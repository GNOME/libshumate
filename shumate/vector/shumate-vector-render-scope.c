/*
 * Copyright (C) 2021 James Westman <james@jwestman.net>
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

#include "shumate-vector-render-scope-private.h"

/* Sets the current layer by name. */
gboolean
shumate_vector_render_scope_find_layer (ShumateVectorRenderScope *self, const char *layer_name)
{
  for (int i = 0; i < self->tile->n_layers; i ++)
    {
      VectorTile__Tile__Layer *layer = self->tile->layers[i];
      if (g_strcmp0 (layer->name, layer_name) == 0)
        {
          self->layer = layer;
          return TRUE;
        }
    }

  self->layer = NULL;
  return FALSE;
}

static int
zigzag (guint value)
{
  return (value >> 1) ^ (-(value & 1));
}

/* Draws the current feature as a path onto the scope's cairo context. */
void
shumate_vector_render_scope_exec_geometry (ShumateVectorRenderScope *self)
{
  g_return_if_fail (self->feature != NULL);

  cairo_new_path (self->cr);
  cairo_move_to (self->cr, 0, 0);

  for (int i = 0; i < self->feature->n_geometry; i ++)
    {
      int cmd = self->feature->geometry[i];
      double dx, dy;

      /* See https://github.com/mapbox/vector-tile-spec/tree/master/2.1#43-geometry-encoding */
      int op = cmd & 0x7;
      int repeat = cmd >> 3;

      for (int j = 0; j < repeat; j ++)
        {
          switch (op) {
          case 1:
            g_return_if_fail (i + 2 < self->feature->n_geometry);
            dx = zigzag (self->feature->geometry[++i]);
            dy = zigzag (self->feature->geometry[++i]);
            cairo_rel_move_to (self->cr, dx, dy);
            break;
          case 2:
            g_return_if_fail (i + 2 < self->feature->n_geometry);
            dx = zigzag (self->feature->geometry[++i]);
            dy = zigzag (self->feature->geometry[++i]);
            cairo_rel_line_to (self->cr, dx, dy);
            break;
          case 7:
            cairo_get_current_point (self->cr, &dx, &dy);
            cairo_close_path (self->cr);
            cairo_move_to (self->cr, dx, dy);
            break;
          default:
            g_assert_not_reached ();
          }
        }
    }
}


void
shumate_vector_render_scope_get_geometry (ShumateVectorRenderScope *self,
                                          ShumateVectorLineString  *linestring)
{
  ShumateVectorPoint *points = NULL;
  gsize point_index = 0;
  double x = 0, y = 0;

  g_return_if_fail (self->feature != NULL);

  for (int i = 0; i < self->feature->n_geometry; i ++)
    {
      int cmd = self->feature->geometry[i];
      double start_x = 0, start_y = 0;

      int op = cmd & 0x7;
      int repeat = cmd >> 3;

      points = g_realloc (points, (repeat + point_index) * sizeof (ShumateVectorPoint));
      g_return_if_fail (points != NULL);

      for (int j = 0; j < repeat; j ++)
        {
          switch (op) {
          case 1:
            g_return_if_fail (i + 2 < self->feature->n_geometry);
            x += zigzag (self->feature->geometry[++i]);
            y += zigzag (self->feature->geometry[++i]);
            start_x = x;
            start_y = y;
            points[point_index++] = (ShumateVectorPoint) {
              .x = x / self->layer->extent,
              .y = y / self->layer->extent,
            };
            break;
          case 2:
            g_return_if_fail (i + 2 < self->feature->n_geometry);
            x += zigzag (self->feature->geometry[++i]);
            y += zigzag (self->feature->geometry[++i]);
            points[point_index++] = (ShumateVectorPoint) {
              .x = x / self->layer->extent,
              .y = y / self->layer->extent,
            };
            break;
          case 7:
            points[point_index++] = (ShumateVectorPoint) {
              .x = start_x / self->layer->extent,
              .y = start_y / self->layer->extent,
            };
            break;
          default:
            g_assert_not_reached ();
          }
        }
    }

  linestring->n_points = point_index;
  linestring->points = points;
}


void
shumate_vector_render_scope_get_bounds (ShumateVectorRenderScope *self,
                                        double                   *min_x,
                                        double                   *min_y,
                                        double                   *max_x,
                                        double                   *max_y)
{
  double x = 0, y = 0;

  *min_x = self->layer->extent;
  *min_y = self->layer->extent;
  *max_x = 0;
  *max_y = 0;

  g_return_if_fail (self->feature != NULL);

  for (int i = 0; i < self->feature->n_geometry; i ++)
    {
      int cmd = self->feature->geometry[i];

      /* See https://github.com/mapbox/vector-tile-spec/tree/master/2.1#43-geometry-encoding */
      int op = cmd & 0x7;
      int repeat = cmd >> 3;

      for (int j = 0; j < repeat; j ++)
        {
          switch (op) {
          case 1:
            g_return_if_fail (i + 2 < self->feature->n_geometry);
            x += zigzag (self->feature->geometry[++i]);
            y += zigzag (self->feature->geometry[++i]);
            break;
          case 2:
            g_return_if_fail (i + 2 < self->feature->n_geometry);
            x += zigzag (self->feature->geometry[++i]);
            y += zigzag (self->feature->geometry[++i]);
            break;
          case 7:
            break;
          default:
            g_assert_not_reached ();
          }

          *min_x = MIN (*min_x, x);
          *min_y = MIN (*min_y, y);
          *max_x = MAX (*max_x, x);
          *max_y = MAX (*max_y, y);
        }
    }
}


void
shumate_vector_render_scope_get_geometry_center (ShumateVectorRenderScope *self,
                                                 double                   *x,
                                                 double                   *y)
{
  double min_x, min_y, max_x, max_y;
  shumate_vector_render_scope_get_bounds (self, &min_x, &min_y, &max_x, &max_y);
  *x = (min_x + max_x) / 2.0 / self->layer->extent;
  *y = (min_y + max_y) / 2.0 / self->layer->extent;
}


void
shumate_vector_render_scope_get_variable (ShumateVectorRenderScope *self, const char *variable, ShumateVectorValue *value)
{
  shumate_vector_value_unset (value);

  if (g_strcmp0 (variable, "zoom") == 0)
    {
      shumate_vector_value_set_number (value, self->zoom_level);
      return;
    }

  if (self->feature == NULL)
    return;

  if (g_strcmp0 ("$type", variable) == 0)
    {
      switch (self->feature->type)
        {
        case VECTOR_TILE__TILE__GEOM_TYPE__POINT:
          shumate_vector_value_set_string (value, "Point");
          return;
        case VECTOR_TILE__TILE__GEOM_TYPE__LINESTRING:
          shumate_vector_value_set_string (value, "LineString");
          return;
        case VECTOR_TILE__TILE__GEOM_TYPE__POLYGON:
          shumate_vector_value_set_string (value, "Polygon");
          return;
        default:
          shumate_vector_value_unset (value);
          return;
        }
    }

  for (int i = 1; i < self->feature->n_tags; i += 2)
    {
      int key = self->feature->tags[i - 1];
      int val = self->feature->tags[i];

      if (key >= self->layer->n_keys || val >= self->layer->n_values)
        return;

      if (g_strcmp0 (self->layer->keys[key], variable) == 0)
        {
          shumate_vector_value_set_from_feature_value (value, self->layer->values[val]);
          return;
        }
    }
}
