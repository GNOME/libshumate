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
  ShumateVectorCollisionRect rect;
} RTreeCol;

typedef struct {
  RTreeCol cols[NODES];
  ShumateVectorCollisionRect rect;
} RTreeRow;

typedef struct {
  RTreeRow rows[NODES];
  ShumateVectorCollisionRect rect;
  int n_markers;
} RTreeTileCol;

typedef struct {
  GHashTable *tile_cols;
  ShumateVectorCollisionRect rect;
} RTreeTileRow;


static RTreeTileCol *
tile_col_new (ShumateVectorCollisionRect *rect)
{
  RTreeTileCol *tile_col = g_new0 (RTreeTileCol, 1);
  tile_col->rect = *rect;
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
tile_row_new (ShumateVectorCollisionRect *rect)
{
  RTreeTileRow *tile_row = g_new0 (RTreeTileRow, 1);
  tile_row->tile_cols = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, (GDestroyNotify) tile_col_free);
  tile_row->rect = *rect;
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


static gboolean
rects_intersect (ShumateVectorCollisionRect *a,
                 ShumateVectorCollisionRect *b)
{
  g_assert (a->left <= a->right);
  g_assert (a->top <= a->bottom);
  g_assert (b->left <= b->right);
  g_assert (b->top <= b->bottom);

  return !(a->left >= b->right
           || a->right <= b->left
           || a->top >= b->bottom
           || a->bottom <= b->top);
}


static void
expand_rect (ShumateVectorCollisionRect       *a,
             const ShumateVectorCollisionRect *b)
{
  if (a->left == 0 && a->right == 0 && a->top == 0 && a->bottom == 0)
    *a = *b;
  else
    {
      a->left = MIN (a->left, b->left);
      a->right = MAX (a->right, b->right);
      a->top = MIN (a->top, b->top);
      a->bottom = MAX (a->bottom, b->bottom);
    }
}


static void
rotate_point (ShumateVectorPoint *point,
              float               radians)
{
  float x, y, s, c;

  if (radians == 0)
    return;

  x = point->x;
  y = point->y;
  s = sin (-radians);
  c = cos (-radians);

  point->x = x * c - y * s;
  point->y = x * s + y * c;
}


static void
scale_point (ShumateVectorPoint *point,
             float               scale)
{
  point->x *= scale;
  point->y *= scale;
}


static void
get_marker_bbox (ShumateVectorCollisionMarker *marker,
                 float                         rot,
                 float                         zoom,
                 ShumateVectorCollisionRect   *rect_out)
{
  ShumateVectorPoint top_left = { marker->rect.left, marker->rect.top };
  ShumateVectorPoint top_right = { marker->rect.right, marker->rect.top };
  ShumateVectorPoint bottom_left = { marker->rect.left, marker->rect.bottom };
  ShumateVectorPoint bottom_right = { marker->rect.right, marker->rect.bottom };

  rotate_point (&top_left, rot);
  rotate_point (&top_right, rot);
  rotate_point (&bottom_left, rot);
  rotate_point (&bottom_right, rot);

  rect_out->left = MIN4 (top_left.x, top_right.x, bottom_left.x, bottom_right.x) + marker->center.x;
  rect_out->right = MAX4 (top_left.x, top_right.x, bottom_left.x, bottom_right.x) + marker->center.x;
  rect_out->top = MIN4 (top_left.y, top_right.y, bottom_left.y, bottom_right.y) + marker->center.y;
  rect_out->bottom = MAX4 (top_left.y, top_right.y, bottom_left.y, bottom_right.y) + marker->center.y;
}


static void
get_marker_full_rot_bbox (ShumateVectorCollisionMarker *marker,
                          ShumateVectorCollisionRect   *rect_out)
{
  float top_left = LEN_SQ (marker->rect.top, marker->rect.left);
  float bottom_left = LEN_SQ (marker->rect.bottom, marker->rect.left);
  float top_right = LEN_SQ (marker->rect.top, marker->rect.right);
  float bottom_right = LEN_SQ (marker->rect.bottom, marker->rect.right);

  float radius = sqrt (MAX4 (top_left, bottom_left, top_right, bottom_right));

  rect_out->left = marker->center.x - radius;
  rect_out->right = marker->center.x + radius;
  rect_out->top = marker->center.y - radius;
  rect_out->bottom = marker->center.y + radius;
}


static gboolean
markers_intersect (ShumateVectorCollisionMarker *a,
                   ShumateVectorCollisionMarker *b,
                   float                         rot,
                   float                         zoom)
{
  ShumateVectorPoint a_center = a->center;
  ShumateVectorPoint b_center = b->center;

  g_assert (a->rect.left <= a->rect.right);
  g_assert (a->rect.top <= a->rect.bottom);
  g_assert (b->rect.left <= b->rect.right);
  g_assert (b->rect.top <= b->rect.bottom);

  rotate_point (&a_center, -rot);
  rotate_point (&b_center, -rot);
  scale_point (&a_center, powf (2, zoom - a->zoom));
  scale_point (&b_center, powf (2, zoom - b->zoom));

  return !(a_center.x + a->rect.left >= b_center.x + b->rect.right
           || a_center.x + a->rect.right <= b_center.x + b->rect.left
           || a_center.y + a->rect.top >= b_center.y + b->rect.bottom
           || a_center.y + a->rect.bottom <= b_center.y + b->rect.top);
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
                                 float                   left,
                                 float                   right,
                                 float                   top,
                                 float                   bottom)
{
  RTreeTileRow *tile_row;
  RTreeTileCol *tile_col;
  RTreeRow *row;
  RTreeCol *col;
  ShumateVectorCollisionMarker *marker;
  ShumateVectorCollisionRect bbox;
  gint64 tile_x = floor (x / TILE_SIZE);
  gint64 tile_y = floor (y / TILE_SIZE);

  g_assert (self != NULL);

  zoom = CLAMP (zoom, 0, ZOOM_LAYERS - 1);

  marker = g_new0 (ShumateVectorCollisionMarker, 1);
  marker->center.x = x;
  marker->center.y = y;
  marker->rect.left = left;
  marker->rect.right = right;
  marker->rect.top = top;
  marker->rect.bottom = bottom;
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
  expand_rect (&tile_row->rect, &bbox);
  expand_rect (&tile_col->rect, &bbox);
  expand_rect (&row->rect, &bbox);
  expand_rect (&col->rect, &bbox);

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
  gint64 tile_x = floor (marker->center.x / TILE_SIZE);
  gint64 tile_y = floor (marker->center.y / TILE_SIZE);

  g_assert (self != NULL);
  g_assert (marker != NULL);

  tile_row = g_hash_table_lookup (self->tile_rows[marker->zoom], (gpointer) tile_y);
  g_assert (tile_row != NULL);

  tile_col = g_hash_table_lookup (tile_row->tile_cols, (gpointer) tile_x);
  g_assert (tile_col != NULL);

  row = &tile_col->rows[row_for_position (marker->center.y)];
  col = &row->cols[row_for_position (marker->center.x)];

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
  ShumateVectorCollisionRect bbox;

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

          if (!rects_intersect (&bbox, &tile_row->rect))
            continue;

          g_hash_table_iter_init (&tile_cols, tile_row->tile_cols);

          while (g_hash_table_iter_next (&tile_cols, NULL, (gpointer*) &tile_col))
            {
              if (!rects_intersect (&bbox, &tile_col->rect))
                continue;

              for (int y = 0; y < NODES; y ++)
                {
                  RTreeRow *row = &tile_col->rows[y];

                  if (!rects_intersect (&bbox, &row->rect))
                    continue;

                  for (int x = 0; x < NODES; x ++)
                    {
                      RTreeCol *col = &row->cols[x];

                      if (col->markers == NULL || !rects_intersect (&bbox, &col->rect))
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
