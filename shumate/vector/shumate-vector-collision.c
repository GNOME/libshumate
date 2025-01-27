/*
 * Copyright (C) 2022 James Westman <james@jwestman.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <https://www.gnu.org/licenses/>.
 */

/* This is a simple implementation of an R-tree, used to detect overlapping
 * markers. An R-tree is a spatial data structure which stores nodes by their
 * bounding boxes. There are lots of fancy heuristics for R-trees to efficiently
 * insert new nodes into the tree, but this implementation uses a fixed
 * structure for simplicity.
 *
 * See <https://en.wikipedia.org/wiki/R-tree>. */

#include <math.h>

#include "shumate-vector-collision-private.h"

#define NODES 4
/* Doesn't need to match the actual tile size */
#define BUCKET_SIZE 256

#define LEN_SQ(x, y) (x*x + y*y)
#define MAX4(a, b, c, d) (MAX (MAX (a, b), MAX (c, d)))
#define MIN4(a, b, c, d) (MIN (MIN (a, b), MIN (c, d)))

#define EMPTY_BOX ((Box){0, 0, 0, 0, 0, 0, 0})

typedef struct {
  gpointer tag;
  double x;
  double y;
  double xextent;
  double yextent;
  double rotation;
  double aaxextent;
  double aayextent;
  uint8_t overlap_never : 1;
} Box;

typedef struct {
  GArray *boxes;
  Box bbox;
} RTreeCol;

typedef struct {
  RTreeCol cols[NODES];
  Box bbox;
} RTreeRow;

typedef struct {
  RTreeRow rows[NODES];
  Box bbox;
  int n_boxes;
} RTreeBucketCol;

typedef struct {
  GHashTable *bucket_cols;
  GPtrArray *bucket_cols_array;
  Box bbox;
} RTreeBucketRow;


static RTreeBucketCol *
bucket_col_new ()
{
  RTreeBucketCol *bucket_col = g_new0 (RTreeBucketCol, 1);
  bucket_col->bbox = EMPTY_BOX;
  return bucket_col;
}


static void
bucket_col_free (RTreeBucketCol *tile_col)
{
  for (int x = 0; x < NODES; x ++)
    for (int y = 0; y < NODES; y ++)
      g_clear_pointer (&tile_col->rows[y].cols[x].boxes, g_array_unref);

  g_free (tile_col);
}


static RTreeBucketRow *
bucket_row_new ()
{
  RTreeBucketRow *bucket_row = g_new0 (RTreeBucketRow, 1);
  bucket_row->bucket_cols = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, (GDestroyNotify) bucket_col_free);
  bucket_row->bucket_cols_array = g_ptr_array_new ();
  bucket_row->bbox = EMPTY_BOX;
  return bucket_row;
}


static void
bucket_row_free (RTreeBucketRow *bucket_row)
{
  g_hash_table_unref (bucket_row->bucket_cols);
  g_ptr_array_unref (bucket_row->bucket_cols_array);
  g_free (bucket_row);
}


ShumateVectorCollision *
shumate_vector_collision_new (void)
{
  ShumateVectorCollision *self = g_new0 (ShumateVectorCollision, 1);

  self->bucket_rows = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, (GDestroyNotify) bucket_row_free);
  self->bucket_rows_array = g_ptr_array_new ();
  self->pending_boxes = g_array_new (FALSE, FALSE, sizeof (Box));

  return self;
}


void
shumate_vector_collision_free (ShumateVectorCollision *self)
{
  g_hash_table_unref (self->bucket_rows);
  g_array_unref (self->pending_boxes);
  g_ptr_array_unref (self->bucket_rows_array);
  g_free (self);
}


static double
dot (ShumateVectorPoint *a,
     ShumateVectorPoint *b)
{
  return a->x * b->x + a->y * b->y;
}


static double
project (ShumateVectorPoint *point,
         ShumateVectorPoint *axis)
{
  ShumateVectorPoint tmp;
  double s = dot (point, axis) / LEN_SQ (axis->x, axis->y);
  tmp = *axis;
  tmp.x *= s;
  tmp.y *= s;
  return dot (&tmp, axis);
}


static ShumateVectorPoint
corner (double x,
        double y,
        double xextent,
        double yextent,
        double rot_cos,
        double rot_sin)
{
  return (ShumateVectorPoint) {
    .x = xextent * rot_cos - yextent * rot_sin + x,
    .y = xextent * rot_sin + yextent * rot_cos + y,
  };
}


static gboolean
rects_intersect_aa (Box *a,
                    Box *b)
{
  /* Test whether the boxes' axis-aligned bounding boxes intersect. */
  return !((a->x + a->aaxextent < b->x - b->aaxextent)
          || (b->x + b->aaxextent < a->x - a->aaxextent)
          || (a->y + a->aayextent < b->y - b->aayextent)
          || (b->y + b->aayextent < a->y - a->aayextent));
}


static gboolean
rects_intersect (Box *a,
                 Box *b)
{
  double cos_a, sin_a, cos_b, sin_b;
  ShumateVectorPoint axes[4], corners_a[4], corners_b[4];

  if (!rects_intersect_aa (a, b))
    return FALSE;

  if (a->rotation == 0 && b->rotation == 0)
    {
      /* If both boxes' rotation is 0, then rects_intersect_aa is equivalent
         to rects_intersect and would have returned FALSE already */
      return TRUE;
    }

  /* See <https://www.gamedev.net/articles/programming/general-and-gameplay-programming/2d-rotated-rectangle-collision-r2604/> */

  cos_a = cosf (a->rotation);
  sin_a = sinf (a->rotation);
  cos_b = cosf (b->rotation);
  sin_b = sinf (b->rotation);

  /* Calculate the four axes of the two rectangles */
  axes[0] = (ShumateVectorPoint) { cos_a, sin_a };
  axes[1] = (ShumateVectorPoint) { -sin_a, cos_a };
  axes[2] = (ShumateVectorPoint) { cos_b, sin_b };
  axes[3] = (ShumateVectorPoint) { -sin_b, cos_b };

  corners_a[0] = corner (a->x, a->y, a->xextent, a->yextent, cos_a, sin_a);
  corners_a[1] = corner (a->x, a->y, -a->xextent, a->yextent, cos_a, sin_a);
  corners_a[2] = corner (a->x, a->y, a->xextent, -a->yextent, cos_a, sin_a);
  corners_a[3] = corner (a->x, a->y, -a->xextent, -a->yextent, cos_a, sin_a);

  corners_b[0] = corner (b->x, b->y, b->xextent, b->yextent, cos_b, sin_b);
  corners_b[1] = corner (b->x, b->y, -b->xextent, b->yextent, cos_b, sin_b);
  corners_b[2] = corner (b->x, b->y, b->xextent, -b->yextent, cos_b, sin_b);
  corners_b[3] = corner (b->x, b->y, -b->xextent, -b->yextent, cos_b, sin_b);

  for (int i = 0; i < 4; i ++)
    {
      ShumateVectorPoint *axis = &axes[i];

      double proj_a[4], proj_b[4];

      /* Project the corners of the rectangles onto the axis */
      for (int j = 0; j < 4; j ++)
        {
          proj_a[j] = project (&corners_a[j], axis);
          proj_b[j] = project (&corners_b[j], axis);
        }

      /* If the projected points don't overlap, the rectangles don't overlap
       * (i.e. either every item in proj_a is greater than or is less than every
       * item in proj_b). */
      double min_a = MIN4 (proj_a[0], proj_a[1], proj_a[2], proj_a[3]);
      double max_a = MAX4 (proj_a[0], proj_a[1], proj_a[2], proj_a[3]);
      double min_b = MIN4 (proj_b[0], proj_b[1], proj_b[2], proj_b[3]);
      double max_b = MAX4 (proj_b[0], proj_b[1], proj_b[2], proj_b[3]);

      if (min_a >= max_b || min_b >= max_a)
        return FALSE;
    }

  return TRUE;
}


static void
expand_rect (Box       *a,
             const Box *b)
{
  if (a->x == 0 && a->y == 0 && a->xextent == 0 && a->yextent == 0)
    *a = *b;
  else
    {
      double left = MIN (a->x - a->aaxextent, b->x - b->aaxextent);
      double right = MAX (a->x + a->aaxextent, b->x + b->aaxextent);
      double top = MIN (a->y - a->aayextent, b->y - b->aayextent);
      double bottom = MAX (a->y + a->aayextent, b->y + b->aayextent);
      a->x = (left + right) / 2.0;
      a->y = (top + bottom) / 2.0;
      a->xextent = (right - left) / 2.0;
      a->yextent = (bottom - top) / 2.0;
      a->aaxextent  = a->xextent;
      a->aayextent  = a->yextent;
    }
}


static void
axis_align (Box *box)
{
  if (box->rotation == 0)
    {
      box->aaxextent = box->xextent;
      box->aayextent = box->yextent;
    }
  else
    {
      box->aaxextent = MAX (
        ABS (cos (box->rotation) *  box->xextent - sin (box->rotation) * box->yextent),
        ABS (cos (box->rotation) * -box->xextent - sin (box->rotation) * box->yextent)
      );
      box->aayextent = MAX (
        ABS (sin (box->rotation) *  box->xextent + cos (box->rotation) * box->yextent),
        ABS (sin (box->rotation) * -box->xextent + cos (box->rotation) * box->yextent)
      );
    }
}


static int
positive_mod (int i, int n)
{
  return (i % n + n) % n;
}


static int
row_for_position (double coordinate)
{
  return positive_mod (coordinate, BUCKET_SIZE) * NODES / BUCKET_SIZE;
}


static gboolean
detect_collision (ShumateVectorCollision *self,
                  Box                    *bbox)
{
  for (int i = 0; i < self->bucket_rows_array->len; i ++)
    {
      RTreeBucketRow *bucket_row = g_ptr_array_index (self->bucket_rows_array, i);

      if (!rects_intersect_aa (bbox, &bucket_row->bbox))
        continue;

      for (int j = 0; j < bucket_row->bucket_cols_array->len; j ++)
        {
          RTreeBucketCol *bucket_col = g_ptr_array_index (bucket_row->bucket_cols_array, j);

          if (bucket_col->n_boxes == 0)
            continue;

          if (!rects_intersect_aa (bbox, &bucket_col->bbox))
            continue;

          for (int y = 0; y < NODES; y ++)
            {
              RTreeRow *row = &bucket_col->rows[y];

              if (!rects_intersect_aa (bbox, &row->bbox))
                continue;

              for (int x = 0; x < NODES; x ++)
                {
                  RTreeCol *col = &row->cols[x];

                  if (col->boxes == NULL || !rects_intersect_aa (bbox, &col->bbox))
                    continue;

                  for (int i = 0; i < col->boxes->len; i ++)
                    {
                      Box *b = &((Box*)col->boxes->data)[i];

                      if (b->overlap_never || bbox->overlap_never)
                        {
                          if (rects_intersect (bbox, b))
                            return TRUE;
                        }
                    }
                }
            }
        }
    }

  return FALSE;
}


gboolean
shumate_vector_collision_check (ShumateVectorCollision *self,
                                double                  x,
                                double                  y,
                                double                  xextent,
                                double                  yextent,
                                double                  rotation,
                                ShumateVectorOverlap    overlap,
                                gboolean                ignore_placement,
                                gpointer                tag)
{
  Box new_bbox = {
    tag, x, y, xextent, yextent, rotation,
    .overlap_never = overlap == SHUMATE_VECTOR_OVERLAP_NEVER
  };

  axis_align (&new_bbox);

  if (overlap != SHUMATE_VECTOR_OVERLAP_ALWAYS)
    {
      if (detect_collision (self, &new_bbox))
        return FALSE;
    }

  if (!ignore_placement)
    g_array_append_val (self->pending_boxes, new_bbox);

  return TRUE;
}

int
shumate_vector_collision_save_pending (ShumateVectorCollision *self)
{
  return self->pending_boxes->len;
}

void
shumate_vector_collision_rollback_pending (ShumateVectorCollision *self,
                                           int                     save)
{
  g_array_set_size (self->pending_boxes, save);
}

void
shumate_vector_collision_commit_pending (ShumateVectorCollision *self,
                                         graphene_rect_t        *bounds_out)
{
  int i;

  for (i = 0; i < self->pending_boxes->len; i ++)
    {
      Box bbox = g_array_index (self->pending_boxes, Box, i);
      RTreeBucketRow *bucket_row;
      RTreeBucketCol *bucket_col;
      RTreeRow *row;
      RTreeCol *col;
      gsize bucket_x = floor (bbox.x / BUCKET_SIZE);
      gsize bucket_y = floor (bbox.y / BUCKET_SIZE);
      graphene_rect_t bounds;

      bucket_row = g_hash_table_lookup (self->bucket_rows, (gpointer) bucket_y);
      if (bucket_row == NULL)
        {
          bucket_row = bucket_row_new ();
          g_hash_table_insert (self->bucket_rows, (gpointer) bucket_y, bucket_row);
          g_ptr_array_add (self->bucket_rows_array, bucket_row);
        }

      bucket_col = g_hash_table_lookup (bucket_row->bucket_cols, (gpointer) bucket_x);
      if (bucket_col == NULL)
        {
          bucket_col = bucket_col_new ();
          g_hash_table_insert (bucket_row->bucket_cols, (gpointer) bucket_x, bucket_col);
          g_ptr_array_add (bucket_row->bucket_cols_array, bucket_col);
        }

      row = &bucket_col->rows[row_for_position (bbox.y)];
      col = &row->cols[row_for_position (bbox.x)];

      if (col->boxes == NULL)
        col->boxes = g_array_new (FALSE, FALSE, sizeof (Box));
      g_array_append_val (col->boxes, bbox);

      bucket_col->n_boxes ++;

      /* Expand the parents to fit the new marker */
      expand_rect (&bucket_row->bbox, &bbox);
      expand_rect (&bucket_col->bbox, &bbox);
      expand_rect (&row->bbox, &bbox);
      expand_rect (&col->bbox, &bbox);

      bounds = GRAPHENE_RECT_INIT (bbox.x - bbox.aaxextent,
                                   bbox.y - bbox.aayextent,
                                   bbox.aaxextent * 2,
                                   bbox.aayextent * 2);
      if (i == 0)
        *bounds_out = bounds;
      else
        graphene_rect_union (bounds_out, &bounds, bounds_out);
    }

  /* clear the pending boxes */
  g_array_set_size (self->pending_boxes, 0);
}


static gboolean
point_intersects_rect_aa (Box    *box,
                          double  x,
                          double  y)
{
  return (x >= box->x - box->aaxextent &&
          x <= box->x + box->aaxextent &&
          y >= box->y - box->aayextent &&
          y <= box->y + box->aayextent);
}


static gboolean
point_intersects_rect (Box    *box,
                       double  x,
                       double  y)
{
  double x2, y2;
  x -= box->x;
  y -= box->y;

  x2 = cosf (-box->rotation) * x - sinf (-box->rotation) * y;
  y2 = sinf (-box->rotation) * x + cosf (-box->rotation) * y;

  return (x2 >= -box->xextent &&
          x2 <= box->xextent &&
          y2 >= -box->yextent &&
          y2 <= box->yextent);
}


gboolean
shumate_vector_collision_query_point (ShumateVectorCollision *self,
                                      double                 x,
                                      double                 y,
                                      gpointer              tag)
{
  for (int i = 0; i < self->bucket_rows_array->len; i ++)
    {
      RTreeBucketRow *bucket_row = g_ptr_array_index (self->bucket_rows_array, i);

      if (!point_intersects_rect_aa (&bucket_row->bbox, x, y))
        continue;

      for (int j = 0; j < bucket_row->bucket_cols_array->len; j ++)
        {
          RTreeBucketCol *bucket_col = g_ptr_array_index (bucket_row->bucket_cols_array, j);

          if (bucket_col->n_boxes == 0)
            continue;

          if (!point_intersects_rect_aa (&bucket_col->bbox, x, y))
            continue;

          for (int r = 0; r < NODES; r ++)
            {
              RTreeRow *row = &bucket_col->rows[r];

              if (!point_intersects_rect_aa (&row->bbox, x, y))
                continue;

              for (int c = 0; c < NODES; c ++)
                {
                  RTreeCol *col = &row->cols[c];

                  if (col->boxes == NULL || !point_intersects_rect_aa (&col->bbox, x, y))
                    continue;

                  for (int i = 0; i < col->boxes->len; i ++)
                    {
                      Box *b = &((Box*)col->boxes->data)[i];

                      if (point_intersects_rect (b, x, y) && (tag == NULL || tag == b->tag))
                        return TRUE;
                    }
                }
            }
        }
    }

  return FALSE;
}


void
shumate_vector_collision_clear (ShumateVectorCollision *self)
{
  GHashTableIter bucket_rows;
  RTreeBucketRow *bucket_row;

  g_hash_table_iter_init (&bucket_rows, self->bucket_rows);

  while (g_hash_table_iter_next (&bucket_rows, NULL, (gpointer*) &bucket_row))
    {
      GHashTableIter bucket_cols;
      RTreeBucketCol *bucket_col;

      bucket_row->bbox = EMPTY_BOX;

      g_hash_table_iter_init (&bucket_cols, bucket_row->bucket_cols);

      while (g_hash_table_iter_next (&bucket_cols, NULL, (gpointer*) &bucket_col))
        {
          if (bucket_col->n_boxes == 0)
            {
              g_ptr_array_remove_fast (bucket_row->bucket_cols_array, bucket_col);
              g_hash_table_iter_remove (&bucket_cols);
              continue;
            }

          bucket_col->n_boxes = 0;
          bucket_col->bbox = EMPTY_BOX;

          for (int y = 0; y < NODES; y ++)
            {
              RTreeRow *row = &bucket_col->rows[y];
              row->bbox = EMPTY_BOX;

              for (int x = 0; x < NODES; x ++)
                {
                  RTreeCol *col = &row->cols[x];
                  col->bbox = EMPTY_BOX;

                  if (col->boxes != NULL)
                    g_array_set_size (col->boxes, 0);
                }
            }
        }

      if (g_hash_table_size (bucket_row->bucket_cols) == 0)
        {
          g_ptr_array_remove_fast (self->bucket_rows_array, bucket_row);
          g_hash_table_iter_remove (&bucket_rows);
        }
    }
}


void
shumate_vector_collision_visualize (ShumateVectorCollision *self,
                                    GtkSnapshot            *snapshot)
{
  GHashTableIter bucket_rows;
  RTreeBucketRow *bucket_row;

  float width[4] = { 1, 1, 1, 1 };
  GdkRGBA color[4], color2[4];

  gdk_rgba_parse (&color[0], "#FF0000");
  gdk_rgba_parse (&color[1], "#FF0000");
  gdk_rgba_parse (&color[2], "#FF0000");
  gdk_rgba_parse (&color[3], "#FF0000");

  gdk_rgba_parse (&color2[0], "#00FF00");
  gdk_rgba_parse (&color2[1], "#00FF00");
  gdk_rgba_parse (&color2[2], "#00FF00");
  gdk_rgba_parse (&color2[3], "#00FF00");

  g_hash_table_iter_init (&bucket_rows, self->bucket_rows);

  while (g_hash_table_iter_next (&bucket_rows, NULL, (gpointer*) &bucket_row))
    {
      GHashTableIter tile_cols;
      RTreeBucketCol *bucket_col;

      gtk_snapshot_append_border (snapshot,
                                  &GSK_ROUNDED_RECT_INIT (bucket_row->bbox.x - bucket_row->bbox.aaxextent,
                                                          bucket_row->bbox.y - bucket_row->bbox.aayextent,
                                                          bucket_row->bbox.aaxextent * 2,
                                                          bucket_row->bbox.aayextent),
                                  width,
                                  color);

      g_hash_table_iter_init (&tile_cols, bucket_row->bucket_cols);

      while (g_hash_table_iter_next (&tile_cols, NULL, (gpointer*) &bucket_col))
        {
          if (bucket_col->n_boxes == 0)
            continue;

          gtk_snapshot_append_border (snapshot,
                                      &GSK_ROUNDED_RECT_INIT (bucket_col->bbox.x - bucket_col->bbox.aaxextent,
                                                              bucket_col->bbox.y - bucket_col->bbox.aayextent,
                                                              bucket_col->bbox.aaxextent * 2,
                                                              bucket_col->bbox.aayextent * 2),
                                      width,
                                      color);

          for (int y = 0; y < NODES; y ++)
            {
              RTreeRow *row = &bucket_col->rows[y];

              gtk_snapshot_append_border (snapshot,
                                          &GSK_ROUNDED_RECT_INIT (row->bbox.x - row->bbox.aaxextent,
                                                                  row->bbox.y - row->bbox.aayextent,
                                                                  row->bbox.aaxextent * 2,
                                                                  row->bbox.aayextent * 2),
                                          width,
                                          color);

              for (int x = 0; x < NODES; x ++)
                {
                  RTreeCol *col = &row->cols[x];

                  if (col->boxes != NULL)
                    {

                      gtk_snapshot_append_border (snapshot,
                                                  &GSK_ROUNDED_RECT_INIT (col->bbox.x - col->bbox.aaxextent,
                                                                          col->bbox.y - col->bbox.aayextent,
                                                                          col->bbox.aaxextent * 2,
                                                                          col->bbox.aayextent * 2),
                                                  width,
                                                  color);

                      for (int i = 0; i < col->boxes->len; i ++)
                        {
                          Box *b = &((Box*)col->boxes->data)[i];

                          gtk_snapshot_save (snapshot);
                          gtk_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (b->x, b->y));
                          gtk_snapshot_rotate (snapshot, b->rotation * 180 / G_PI);

                          gtk_snapshot_append_border (snapshot,
                                                      &GSK_ROUNDED_RECT_INIT (-b->xextent,
                                                                              -b->yextent,
                                                                              b->xextent * 2,
                                                                              b->yextent * 2),
                                                      width,
                                                      color2);

                          gtk_snapshot_restore (snapshot);
                        }
                    }
                }
            }
        }
    }
}
