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

#define ZOOM_LAYERS 15

#define LEN_SQ(x, y) (x*x + y*y)
#define MAX4(a, b, c, d) (MAX (MAX (a, b), MAX (c, d)))
#define MIN4(a, b, c, d) (MIN (MIN (a, b), MIN (c, d)))


struct ShumateVectorCollision {
  /* one set of rows per zoom layer */
  GHashTable *bucket_rows[ZOOM_LAYERS];
  guint seq : 1;

  guint dirty : 1;
  float last_rot;
  float last_zoom;

  int tile_size;

  GList *markers;
};

typedef struct {
  GPtrArray *markers;
  ShumateVectorCollisionBBox bbox;
} RTreeCol;

typedef struct {
  RTreeCol cols[NODES];
  ShumateVectorCollisionBBox bbox;
} RTreeRow;

typedef struct {
  RTreeRow rows[NODES];
  ShumateVectorCollisionBBox bbox;
  int n_markers;
} RTreeBucketCol;

typedef struct {
  GHashTable *bucket_cols;
  ShumateVectorCollisionBBox bbox;
} RTreeBucketRow;


static RTreeBucketCol *
bucket_col_new (ShumateVectorCollisionBBox *bbox)
{
  RTreeBucketCol *bucket_col = g_new0 (RTreeBucketCol, 1);
  bucket_col->bbox = *bbox;
  return bucket_col;
}


static void
bucket_col_free (RTreeBucketCol *tile_col)
{
  for (int x = 0; x < NODES; x ++)
    for (int y = 0; y < NODES; y ++)
      g_clear_pointer (&tile_col->rows[y].cols[x].markers, g_ptr_array_unref);

  g_free (tile_col);
}


static RTreeBucketRow *
tile_row_new (ShumateVectorCollisionBBox *bbox)
{
  RTreeBucketRow *bucket_row = g_new0 (RTreeBucketRow, 1);
  bucket_row->bucket_cols = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, (GDestroyNotify) bucket_col_free);
  bucket_row->bbox = *bbox;
  return bucket_row;
}


static void
bucket_row_free (RTreeBucketRow *bucket_row)
{
  g_hash_table_unref (bucket_row->bucket_cols);
  g_free (bucket_row);
}


ShumateVectorCollision *
shumate_vector_collision_new (int tile_size)
{
  ShumateVectorCollision *self = g_new0 (ShumateVectorCollision, 1);
  self->tile_size = tile_size;

  for (int i = 0; i < ZOOM_LAYERS; i ++)
    self->bucket_rows[i] = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, (GDestroyNotify) bucket_row_free);

  self->dirty = TRUE;

  return self;
}


void
shumate_vector_collision_free (ShumateVectorCollision *self)
{
  for (int i = 0; i < ZOOM_LAYERS; i ++)
    g_hash_table_unref (self->bucket_rows[i]);

  g_list_free (self->markers);

  g_free (self);
}


static float
dot (ShumateVectorPoint *a,
     ShumateVectorPoint *b)
{
  return a->x * b->x + a->y * b->y;
}


static float
project (ShumateVectorPoint *point,
         ShumateVectorPoint *axis)
{
  ShumateVectorPoint tmp;
  float s = dot (point, axis) / LEN_SQ (axis->x, axis->y);
  tmp = *axis;
  tmp.x *= s;
  tmp.y *= s;
  return dot (&tmp, axis);
}


static ShumateVectorPoint
corner (float x,
        float y,
        float xextent,
        float yextent,
        float rot_cos,
        float rot_sin)
{
  return (ShumateVectorPoint) {
    .x = xextent * rot_cos - yextent * rot_sin + x,
    .y = xextent * rot_sin + yextent * rot_cos + y,
  };
}


static gboolean
rects_intersect (ShumateVectorCollisionBBox *a,
                 ShumateVectorCollisionBBox *b)
{
  float cos_a, sin_a, cos_b, sin_b;
  ShumateVectorPoint axes[4], corners_a[4], corners_b[4];

  if (a->rotation == 0 && b->rotation == 0)
    {
      return !((a->x + a->xextent < b->x - b->xextent)
                || (b->x + b->xextent < a->x - a->xextent)
                || (a->y + a->yextent < b->y - b->yextent)
                || (b->y + b->yextent < a->y - a->yextent));
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

      float proj_a[4], proj_b[4];

      /* Project the corners of the rectangles onto the axis */
      for (int j = 0; j < 4; j ++)
        {
          proj_a[j] = project (&corners_a[j], axis);
          proj_b[j] = project (&corners_b[j], axis);
        }

      /* If the projected points don't overlap, the rectangles don't overlap
       * (i.e. either every item in proj_a is greater than or is less than every
       * item in proj_b). */
      float min_a = MIN4 (proj_a[0], proj_a[1], proj_a[2], proj_a[3]);
      float max_a = MAX4 (proj_a[0], proj_a[1], proj_a[2], proj_a[3]);
      float min_b = MIN4 (proj_b[0], proj_b[1], proj_b[2], proj_b[3]);
      float max_b = MAX4 (proj_b[0], proj_b[1], proj_b[2], proj_b[3]);

      if (min_a >= max_b || min_b >= max_a)
        return FALSE;
    }

  return TRUE;
}


static void
expand_rect (ShumateVectorCollisionBBox       *a,
             const ShumateVectorCollisionBBox *b)
{
  g_assert (a->rotation == 0);
  g_assert (b->rotation == 0);

  if (a->x == 0 && a->y == 0 && a->xextent == 0 && a->yextent == 0)
    *a = *b;
  else
    {
      float left = MIN (a->x - a->xextent, b->x - b->xextent);
      float right = MAX (a->x + a->xextent, b->x + b->xextent);
      float top = MIN (a->y - a->yextent, b->y - b->yextent);
      float bottom = MAX (a->y + a->yextent, b->y + b->yextent);
      a->x = (left + right) / 2.0;
      a->y = (top + bottom) / 2.0;
      a->xextent = (right - left) / 2.0;
      a->yextent = (bottom - top) / 2.0;
    }
}


static void
get_marker_full_rot_bbox (ShumateVectorCollisionMarker *marker,
                          ShumateVectorCollisionBBox   *bbox_out)
{
  float radius = sqrt (LEN_SQ (marker->bbox.xextent, marker->bbox.yextent));

  if (!marker->symbol_info->line_placement)
    {
      bbox_out->x = marker->bbox.x;
      bbox_out->y = marker->bbox.y;
      bbox_out->xextent = radius;
      bbox_out->yextent = radius;
      bbox_out->rotation = 0;
    }
  else
    *bbox_out = marker->bbox;
}


static void
get_marker_bbox (ShumateVectorCollisionMarker *marker,
                 float                         rot,
                 float                         zoom,
                 ShumateVectorCollisionBBox   *bbox)
{
  float scale = powf (2, zoom - marker->zoom);

  *bbox = marker->bbox;
  bbox->x *= scale;
  bbox->y *= scale;
  if (!marker->symbol_info->line_placement)
    bbox->rotation -= rot;
}


static gboolean
markers_intersect (ShumateVectorCollision       *self,
                   ShumateVectorCollisionMarker *a,
                   ShumateVectorCollisionMarker *b,
                   float                         rot,
                   float                         zoom)
{
  ShumateVectorCollisionBBox a_bbox, b_bbox;

  get_marker_bbox (a, rot, zoom, &a_bbox);
  get_marker_bbox (b, rot, zoom, &b_bbox);

  if (!rects_intersect (&a_bbox, &b_bbox))
    return FALSE;

  /* If neither marker is a line label, there's no further checks to do */
  if (!a->symbol_info->line_placement && !b->symbol_info->line_placement)
    return TRUE;

  /* If one marker is a line label and the other is not, make sure 'a' is the
   * line label */
  if (!a->symbol_info->line_placement && b->symbol_info->line_placement)
    {
      ShumateVectorCollisionMarker *tmp = a;
      ShumateVectorCollisionBBox tmp_bbox = a_bbox;
      a = b;
      a_bbox = b_bbox;
      b = tmp;
      b_bbox = tmp_bbox;
    }

  if (a->symbol_info->line_placement && !b->symbol_info->line_placement)
    {
      ShumateVectorPointIter iter;
      float scale = powf (2, zoom - a->zoom) * self->tile_size;
      float text_length = MIN (a->symbol_info->line_length, a->text_length / (float) self->tile_size);

      shumate_vector_point_iter_init (&iter, &a->symbol_info->line);
      shumate_vector_point_iter_advance (&iter, (a->symbol_info->line_length - text_length) / 2.0);

      do
        {
          ShumateVectorPoint current_point;
          shumate_vector_point_iter_get_segment_center (&iter, text_length, &current_point);
          a_bbox.x = (current_point.x + a->tile_x) * scale;
          a_bbox.y = (current_point.y + a->tile_y) * scale;
          a_bbox.xextent = MIN (text_length, shumate_vector_point_iter_get_segment_length (&iter)) * scale / 2;
          a_bbox.yextent = a->symbol_info->text_size / 2.0;
          a_bbox.rotation = shumate_vector_point_iter_get_current_angle (&iter);

          if (rects_intersect (&a_bbox, &b_bbox))
            return TRUE;
        }
      while ((text_length -= shumate_vector_point_iter_next_segment (&iter)) > 0);

      return FALSE;
    }
  else
    {
      /* Both are line labels */
      ShumateVectorPointIter a_iter;
      float a_scale = powf (2, zoom - a->zoom) * self->tile_size;
      float b_scale = powf (2, zoom - b->zoom) * self->tile_size;
      float a_text_length = MIN (a->symbol_info->line_length, a->text_length / (float) self->tile_size);

      shumate_vector_point_iter_init (&a_iter, &a->symbol_info->line);
      shumate_vector_point_iter_advance (&a_iter, (a->symbol_info->line_length - a_text_length) / 2.0);

      do
        {
          ShumateVectorPointIter b_iter;
          ShumateVectorPoint a_current_point;
          ShumateVectorCollisionBBox a_segment_bbox;
          float b_text_length = MIN (b->symbol_info->line_length, b->text_length / (float) self->tile_size);

          shumate_vector_point_iter_get_segment_center (&a_iter, a_text_length, &a_current_point);
          a_segment_bbox.x = (a->tile_x + a_current_point.x) * a_scale;
          a_segment_bbox.y = (a->tile_y + a_current_point.y) * a_scale;
          a_segment_bbox.xextent = MIN (b_text_length, shumate_vector_point_iter_get_segment_length (&a_iter)) * a_scale / 2;
          a_segment_bbox.yextent = a->symbol_info->text_size / 2.0;
          a_segment_bbox.rotation = shumate_vector_point_iter_get_current_angle (&a_iter);

          shumate_vector_point_iter_init (&b_iter, &b->symbol_info->line);
          shumate_vector_point_iter_advance (&b_iter, (b->symbol_info->line_length - b_text_length) / 2.0);

          if (!rects_intersect (&a_segment_bbox, &b_bbox))
            continue;

          do
            {
              ShumateVectorPoint b_current_point;
              ShumateVectorCollisionBBox b_segment_bbox;

              shumate_vector_point_iter_get_segment_center (&b_iter, b_text_length, &b_current_point);
              b_segment_bbox.x = (b->tile_x + b_current_point.x) * b_scale;
              b_segment_bbox.y = (b->tile_y + b_current_point.y) * b_scale;
              b_segment_bbox.xextent = MIN (b_text_length, shumate_vector_point_iter_get_segment_length (&b_iter)) * b_scale / 2;
              b_segment_bbox.yextent = b->symbol_info->text_size / 2.0;
              b_segment_bbox.rotation = shumate_vector_point_iter_get_current_angle (&b_iter);

              if (rects_intersect (&a_segment_bbox, &b_segment_bbox))
                return TRUE;
            }
          while ((b_text_length -= shumate_vector_point_iter_next_segment (&b_iter)) > 0);
        }
      while ((a_text_length -= shumate_vector_point_iter_next_segment (&a_iter)) > 0);

      return FALSE;
    }
}


static int
positive_mod (int i, int n)
{
  return (i % n + n) % n;
}


static int
row_for_position (float coordinate)
{
  return positive_mod (coordinate, BUCKET_SIZE) * NODES / BUCKET_SIZE;
}


ShumateVectorCollisionMarker *
shumate_vector_collision_insert (ShumateVectorCollision  *self,
                                 int                      zoom,
                                 ShumateVectorSymbolInfo *symbol_info,
                                 float                    text_length,
                                 int                      tile_x,
                                 int                      tile_y)
{
  RTreeBucketRow *bucket_row;
  RTreeBucketCol *bucket_col;
  RTreeRow *row;
  RTreeCol *col;
  ShumateVectorCollisionBBox bbox;
  ShumateVectorCollisionMarker *marker;
  float x = (tile_x + symbol_info->x) * self->tile_size;
  float y = (tile_y + symbol_info->y) * self->tile_size;
  gint64 bucket_x = floor (x / BUCKET_SIZE);
  gint64 bucket_y = floor (y / BUCKET_SIZE);

  g_assert (self != NULL);

  zoom = CLAMP (zoom, 0, ZOOM_LAYERS - 1);

  marker = g_new0 (ShumateVectorCollisionMarker, 1);
  marker->symbol_info = symbol_info;
  marker->tile_x = tile_x;
  marker->tile_y = tile_y;
  marker->text_length = text_length;
  marker->bbox.x = x;
  marker->bbox.y = y;
  marker->zoom = zoom;
  marker->seq = self->seq;

  if (symbol_info->line_placement)
    {
      marker->bbox.xextent = symbol_info->line_size.x * self->tile_size + symbol_info->text_size;
      marker->bbox.yextent = symbol_info->line_size.y * self->tile_size + symbol_info->text_size;
    }
  else
    {
      marker->bbox.xextent = text_length / 2;
      marker->bbox.yextent = symbol_info->text_size / 2;
    }

  self->markers = g_list_prepend (self->markers, marker);
  marker->list_link = g_list_first (self->markers);

  get_marker_full_rot_bbox (marker, &bbox);

  bucket_row = g_hash_table_lookup (self->bucket_rows[zoom], (gpointer) bucket_y);
  if (bucket_row == NULL)
    {
      bucket_row = tile_row_new (&bbox);
      g_hash_table_insert (self->bucket_rows[zoom], (gpointer) bucket_y, bucket_row);
    }

  bucket_col = g_hash_table_lookup (bucket_row->bucket_cols, (gpointer) bucket_x);
  if (bucket_col == NULL)
    {
      bucket_col = bucket_col_new (&bbox);
      g_hash_table_insert (bucket_row->bucket_cols, (gpointer) bucket_x, bucket_col);
    }

  row = &bucket_col->rows[row_for_position (y)];
  col = &row->cols[row_for_position (x)];

  if (col->markers == NULL)
    col->markers = g_ptr_array_new_full (8, g_free);
  g_ptr_array_add (col->markers, marker);

  bucket_col->n_markers ++;

  /* Expand the parents to fit the new marker */
  expand_rect (&bucket_row->bbox, &bbox);
  expand_rect (&bucket_col->bbox, &bbox);
  expand_rect (&row->bbox, &bbox);
  expand_rect (&col->bbox, &bbox);

  self->dirty = TRUE;
  return marker;
}


void
shumate_vector_collision_remove (ShumateVectorCollision       *self,
                                 ShumateVectorCollisionMarker *marker)
{
  RTreeBucketRow *bucket_row;
  RTreeBucketCol *bucket_col;
  RTreeRow *row;
  RTreeCol *col;
  gint64 tile_x = floor (marker->bbox.x / BUCKET_SIZE);
  gint64 tile_y = floor (marker->bbox.y / BUCKET_SIZE);

  g_assert (self != NULL);
  g_assert (marker != NULL);

  bucket_row = g_hash_table_lookup (self->bucket_rows[marker->zoom], (gpointer) tile_y);
  g_assert (bucket_row != NULL);

  bucket_col = g_hash_table_lookup (bucket_row->bucket_cols, (gpointer) tile_x);
  g_assert (bucket_col != NULL);

  row = &bucket_col->rows[row_for_position (marker->bbox.y)];
  col = &row->cols[row_for_position (marker->bbox.x)];

  self->markers = g_list_remove_link (self->markers, marker->list_link);
  g_list_free (marker->list_link);
  g_ptr_array_remove_fast (col->markers, marker);

  g_assert (bucket_col->n_markers > 0);
  bucket_col->n_markers --;

  if (bucket_col->n_markers == 0)
    g_hash_table_remove (bucket_row->bucket_cols, (gpointer) tile_x);

  if (g_hash_table_size (bucket_row->bucket_cols) == 0)
    g_hash_table_remove (self->bucket_rows[marker->zoom], (gpointer) tile_y);

  self->dirty = TRUE;
}


static gboolean
detect_collision (ShumateVectorCollision       *self,
                  ShumateVectorCollisionMarker *marker,
                  float                         rot,
                  float                         zoom)
{
  ShumateVectorCollisionBBox bbox;

  for (int z = 0; z < ZOOM_LAYERS; z ++)
    {
      GHashTableIter bucket_rows;
      RTreeBucketRow *bucket_row;

      get_marker_bbox (marker, rot, z, &bbox);

      g_hash_table_iter_init (&bucket_rows, self->bucket_rows[z]);

      while (g_hash_table_iter_next (&bucket_rows, NULL, (gpointer*) &bucket_row))
        {
          GHashTableIter tile_cols;
          RTreeBucketCol *bucket_col;

          if (!rects_intersect (&bbox, &bucket_row->bbox))
            continue;

          g_hash_table_iter_init (&tile_cols, bucket_row->bucket_cols);

          while (g_hash_table_iter_next (&tile_cols, NULL, (gpointer*) &bucket_col))
            {
              if (!rects_intersect (&bbox, &bucket_col->bbox))
                continue;

              for (int y = 0; y < NODES; y ++)
                {
                  RTreeRow *row = &bucket_col->rows[y];

                  if (!rects_intersect (&bbox, &row->bbox))
                    continue;

                  for (int x = 0; x < NODES; x ++)
                    {
                      RTreeCol *col = &row->cols[x];

                      if (col->markers == NULL || !rects_intersect (&bbox, &col->bbox))
                        continue;

                      for (int i = 0; i < col->markers->len; i ++)
                        {
                          ShumateVectorCollisionMarker *m = col->markers->pdata[i];

                          if (m == marker || !m->visible || m->seq != self->seq)
                            continue;

                          if (markers_intersect (self, m, marker, rot, zoom))
                            return TRUE;
                        }
                    }
                }
            }
        }
    }

  return FALSE;
}


void
shumate_vector_collision_recalc (ShumateVectorCollision *self,
                                 float                   rot,
                                 float                   zoom)
{
  if (!self->dirty && rot == self->last_rot && zoom == self->last_zoom)
    return;

  self->seq = !self->seq;

  for (GList *l = self->markers; l != NULL; l = l->next)
    {
      ShumateVectorCollisionMarker *marker = (ShumateVectorCollisionMarker *)l->data;
      marker->visible = !detect_collision (self, marker, rot, zoom);
      marker->seq = self->seq;
    }

  self->dirty = FALSE;
  self->last_rot = rot;
  self->last_zoom = zoom;
}
