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

enum {
  MOVE_TO = 1,
  LINE_TO = 2,
  CLOSE_PATH = 7,
};

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

static void
apply_transforms (ShumateVectorRenderScope *self,
                  float                    *x,
                  float                    *y)
{
  *x = ((*x / self->layer->extent) - self->overzoom_x) * self->overzoom_scale;
  *y = ((*y / self->layer->extent) - self->overzoom_y) * self->overzoom_scale;
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
          case MOVE_TO:
            g_return_if_fail (i + 2 < self->feature->n_geometry);
            dx = zigzag (self->feature->geometry[++i]);
            dy = zigzag (self->feature->geometry[++i]);
            cairo_rel_move_to (self->cr, dx, dy);
            break;
          case LINE_TO:
            g_return_if_fail (i + 2 < self->feature->n_geometry);
            dx = zigzag (self->feature->geometry[++i]);
            dy = zigzag (self->feature->geometry[++i]);
            cairo_rel_line_to (self->cr, dx, dy);
            break;
          case CLOSE_PATH:
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


GPtrArray *
shumate_vector_render_scope_get_geometry (ShumateVectorRenderScope *self)
{
  GPtrArray *lines = g_ptr_array_new ();
  ShumateVectorLineString *current_line = NULL;
  float x = 0, y = 0;
  float x_tf, y_tf;

  g_return_val_if_fail (self->feature != NULL, NULL);

  for (int i = 0; i < self->feature->n_geometry; i ++)
    {
      int cmd = self->feature->geometry[i];
      double start_x = 0, start_y = 0;

      int op = cmd & 0x7;
      int repeat = cmd >> 3;

      if (current_line != NULL)
        current_line->points = g_realloc (current_line->points, (repeat + current_line->n_points) * sizeof (ShumateVectorPoint));

      for (int j = 0; j < repeat; j ++)
        {
          switch (op) {
          case MOVE_TO:
            g_return_val_if_fail (i + 2 < self->feature->n_geometry, NULL);

            if (current_line != NULL)
              g_ptr_array_add (lines, current_line);

            current_line = g_new (ShumateVectorLineString, 1);
            current_line->points = g_new (ShumateVectorPoint, 1);
            current_line->n_points = 1;

            x += zigzag (self->feature->geometry[++i]);
            y += zigzag (self->feature->geometry[++i]);

            x_tf = x;
            y_tf = y;
            apply_transforms (self, &x_tf, &y_tf);

            start_x = x_tf;
            start_y = y_tf;

            current_line->points[0] = (ShumateVectorPoint) {
              .x = x_tf,
              .y = y_tf,
            };
            break;
          case LINE_TO:
            g_return_val_if_fail (i + 2 < self->feature->n_geometry, NULL);
            g_return_val_if_fail (current_line != NULL, NULL);

            x += zigzag (self->feature->geometry[++i]);
            y += zigzag (self->feature->geometry[++i]);

            x_tf = x;
            y_tf = y;
            apply_transforms (self, &x_tf, &y_tf);

            current_line->points[current_line->n_points++] = (ShumateVectorPoint) {
              .x = x_tf,
              .y = y_tf,
            };
            break;
          case CLOSE_PATH:
            g_return_val_if_fail (current_line != NULL, NULL);

            current_line->points[current_line->n_points++] = (ShumateVectorPoint) {
              .x = start_x,
              .y = start_y,
            };
            break;
          default:
            g_assert_not_reached ();
          }
        }
    }

  if (current_line != NULL)
    g_ptr_array_add (lines, current_line);

  return lines;
}


void
shumate_vector_render_scope_get_bounds (ShumateVectorRenderScope *self,
                                        float                    *min_x,
                                        float                    *min_y,
                                        float                    *max_x,
                                        float                    *max_y)
{
  double x = 0, y = 0;

  *min_x = G_MAXFLOAT;
  *min_y = G_MAXFLOAT;
  *max_x = G_MINFLOAT;
  *max_y = G_MINFLOAT;

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

  apply_transforms (self, min_x, min_y);
  apply_transforms (self, max_x, max_y);
}


ShumateVectorGeometryType
shumate_vector_render_scope_get_geometry_type (ShumateVectorRenderScope *self)
{
  g_return_val_if_fail (self->feature != NULL, 0);
  return (ShumateVectorGeometryType) self->feature->type;
}

void
shumate_vector_render_scope_get_geometry_center (ShumateVectorRenderScope *self,
                                                 double                   *x,
                                                 double                   *y)
{
  float min_x, min_y, max_x, max_y;
  shumate_vector_render_scope_get_bounds (self, &min_x, &min_y, &max_x, &max_y);
  *x = (min_x + max_x) / 2.0;
  *y = (min_y + max_y) / 2.0;
}


void
shumate_vector_render_scope_get_variable (ShumateVectorRenderScope *self, const char *variable, ShumateVectorValue *value)
{
  shumate_vector_value_unset (value);

  if (self->feature == NULL)
    return;

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


GHashTable *
shumate_vector_render_scope_create_tag_table (ShumateVectorRenderScope *self)
{
  g_autoptr(GHashTable) tags = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
  g_auto(ShumateVectorValue) value = SHUMATE_VECTOR_VALUE_INIT;

  for (int i = 1; i < self->feature->n_tags; i += 2)
    {
      int key = self->feature->tags[i - 1];
      int val = self->feature->tags[i];

      if (key >= self->layer->n_keys || val >= self->layer->n_values)
        continue;

      shumate_vector_value_set_from_feature_value (&value, self->layer->values[val]);
      g_hash_table_insert (tags, g_strdup (self->layer->keys[key]), shumate_vector_value_as_string (&value));
    }

  return g_steal_pointer (&tags);
}

