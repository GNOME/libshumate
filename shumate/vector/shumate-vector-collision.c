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
#define TILE_SIZE 256

#define ZOOM_LAYERS 15

#define LEN_SQ(x, y) (x*x + y*y)
#define MAX4(a, b, c, d) (MAX (MAX (a, b), MAX (c, d)))
#define MIN4(a, b, c, d) (MIN (MIN (a, b), MIN (c, d)))


struct ShumateVectorCollision {
  /* one set of rows per zoom layer */
  GHashTable *tile_rows[ZOOM_LAYERS];
  guint seq : 1;

  guint dirty : 1;
  float last_rot;
  float last_zoom;

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
} RTreeTileCol;

typedef struct {
  GHashTable *tile_cols;
  ShumateVectorCollisionBBox bbox;
} RTreeTileRow;


static RTreeTileCol *
tile_col_new (ShumateVectorCollisionBBox *bbox)
{
  RTreeTileCol *tile_col = g_new0 (RTreeTileCol, 1);
  tile_col->bbox = *bbox;
  return tile_col;
}


static void
tile_col_free (RTreeTileCol *tile_col)
{
  for (int x = 0; x < NODES; x ++)
    for (int y = 0; y < NODES; y ++)
      g_clear_pointer (&tile_col->rows[y].cols[x].markers, g_ptr_array_unref);

  g_free (tile_col);
}


static RTreeTileRow *
tile_row_new (ShumateVectorCollisionBBox *bbox)
{
  RTreeTileRow *tile_row = g_new0 (RTreeTileRow, 1);
  tile_row->tile_cols = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, (GDestroyNotify) tile_col_free);
  tile_row->bbox = *bbox;
  return tile_row;
}


static void
tile_row_free (RTreeTileRow *tile_row)
{
  g_hash_table_unref (tile_row->tile_cols);
  g_free (tile_row);
}


ShumateVectorCollision *
shumate_vector_collision_new (void)
{
  ShumateVectorCollision *tree = g_new0 (ShumateVectorCollision, 1);

  for (int i = 0; i < ZOOM_LAYERS; i ++)
    tree->tile_rows[i] = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, (GDestroyNotify) tile_row_free);

  tree->dirty = TRUE;

  return tree;
}


void
shumate_vector_collision_free (ShumateVectorCollision *self)
{
  for (int i = 0; i < ZOOM_LAYERS; i ++)
    g_hash_table_unref (self->tile_rows[i]);

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

  if (marker->rotates)
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
  if (marker->rotates)
    bbox->rotation -= rot;
}


static gboolean
markers_intersect (ShumateVectorCollisionMarker *a,
                   ShumateVectorCollisionMarker *b,
                   float                         rot,
                   float                         zoom)
{
  ShumateVectorCollisionBBox a_bbox, b_bbox;

  get_marker_bbox (a, rot, zoom, &a_bbox);
  get_marker_bbox (b, rot, zoom, &b_bbox);

  return rects_intersect (&a_bbox, &b_bbox);
}


static int
positive_mod (int i, int n)
{
  return (i % n + n) % n;
}


static int
row_for_position (float coordinate)
{
  return positive_mod (coordinate, TILE_SIZE) * NODES / TILE_SIZE;
}


ShumateVectorCollisionMarker *
shumate_vector_collision_insert (ShumateVectorCollision *self,
                                 int                     zoom,
                                 float                   x,
                                 float                   y,
                                 float                   xextent,
                                 float                   yextent,
                                 guint                   rotates)
{
  RTreeTileRow *tile_row;
  RTreeTileCol *tile_col;
  RTreeRow *row;
  RTreeCol *col;
  ShumateVectorCollisionBBox bbox;
  ShumateVectorCollisionMarker *marker;
  gint64 tile_x = floor (x / TILE_SIZE);
  gint64 tile_y = floor (y / TILE_SIZE);

  g_assert (self != NULL);

  zoom = CLAMP (zoom, 0, ZOOM_LAYERS - 1);

  marker = g_new0 (ShumateVectorCollisionMarker, 1);
  marker->bbox.x = x;
  marker->bbox.y = y;
  marker->bbox.xextent = xextent;
  marker->bbox.yextent = yextent;
  marker->rotates = !!rotates;
  marker->zoom = zoom;
  marker->seq = self->seq;

  self->markers = g_list_prepend (self->markers, marker);
  marker->list_link = g_list_first (self->markers);

  get_marker_full_rot_bbox (marker, &bbox);

  tile_row = g_hash_table_lookup (self->tile_rows[zoom], (gpointer) tile_y);
  if (tile_row == NULL)
    {
      tile_row = tile_row_new (&bbox);
      g_hash_table_insert (self->tile_rows[zoom], (gpointer) tile_y, tile_row);
    }

  tile_col = g_hash_table_lookup (tile_row->tile_cols, (gpointer) tile_x);
  if (tile_col == NULL)
    {
      tile_col = tile_col_new (&bbox);
      g_hash_table_insert (tile_row->tile_cols, (gpointer) tile_x, tile_col);
    }

  row = &tile_col->rows[row_for_position (y)];
  col = &row->cols[row_for_position (x)];

  if (col->markers == NULL)
    col->markers = g_ptr_array_new_full (8, g_free);
  g_ptr_array_add (col->markers, marker);

  tile_col->n_markers ++;

  /* Expand the parents to fit the new marker */
  expand_rect (&tile_row->bbox, &bbox);
  expand_rect (&tile_col->bbox, &bbox);
  expand_rect (&row->bbox, &bbox);
  expand_rect (&col->bbox, &bbox);

  self->dirty = TRUE;
  return marker;
}


void
shumate_vector_collision_remove (ShumateVectorCollision       *self,
                                 ShumateVectorCollisionMarker *marker)
{
  RTreeTileRow *tile_row;
  RTreeTileCol *tile_col;
  RTreeRow *row;
  RTreeCol *col;
  gint64 tile_x = floor (marker->bbox.x / TILE_SIZE);
  gint64 tile_y = floor (marker->bbox.y / TILE_SIZE);

  g_assert (self != NULL);
  g_assert (marker != NULL);

  tile_row = g_hash_table_lookup (self->tile_rows[marker->zoom], (gpointer) tile_y);
  g_assert (tile_row != NULL);

  tile_col = g_hash_table_lookup (tile_row->tile_cols, (gpointer) tile_x);
  g_assert (tile_col != NULL);

  row = &tile_col->rows[row_for_position (marker->bbox.y)];
  col = &row->cols[row_for_position (marker->bbox.x)];

  self->markers = g_list_remove_link (self->markers, marker->list_link);
  g_list_free (marker->list_link);
  g_ptr_array_remove_fast (col->markers, marker);

  g_assert (tile_col->n_markers > 0);
  tile_col->n_markers --;

  if (tile_col->n_markers == 0)
    g_hash_table_remove (tile_row->tile_cols, (gpointer) tile_x);

  if (g_hash_table_size (tile_row->tile_cols) == 0)
    g_hash_table_remove (self->tile_rows[marker->zoom], (gpointer) tile_y);

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
      GHashTableIter tile_rows;
      RTreeTileRow *tile_row;

      get_marker_bbox (marker, rot, z, &bbox);

      g_hash_table_iter_init (&tile_rows, self->tile_rows[z]);

      while (g_hash_table_iter_next (&tile_rows, NULL, (gpointer*) &tile_row))
        {
          GHashTableIter tile_cols;
          RTreeTileCol *tile_col;

          if (!rects_intersect (&bbox, &tile_row->bbox))
            continue;

          g_hash_table_iter_init (&tile_cols, tile_row->tile_cols);

          while (g_hash_table_iter_next (&tile_cols, NULL, (gpointer*) &tile_col))
            {
              if (!rects_intersect (&bbox, &tile_col->bbox))
                continue;

              for (int y = 0; y < NODES; y ++)
                {
                  RTreeRow *row = &tile_col->rows[y];

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

                          if (markers_intersect (m, marker, rot, zoom))
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
