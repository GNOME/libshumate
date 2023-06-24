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

#include <gtk/gtk.h>
#include "shumate-vector-expression-private.h"
#include "shumate-vector-line-layer-private.h"
#include "shumate-vector-utils-private.h"
#include "shumate-vector-value-private.h"

struct _ShumateVectorLineLayer
{
  ShumateVectorLayer parent_instance;

  ShumateVectorExpression *color;
  ShumateVectorExpression *opacity;
  ShumateVectorExpression *width;
  ShumateVectorExpression *cap;
  ShumateVectorExpression *join;

  double *dashes;
  int num_dashes;
};

G_DEFINE_TYPE (ShumateVectorLineLayer, shumate_vector_line_layer, SHUMATE_TYPE_VECTOR_LAYER)


ShumateVectorLayer *
shumate_vector_line_layer_create_from_json (JsonObject *object, GError **error)
{
  ShumateVectorLineLayer *layer = g_object_new (SHUMATE_TYPE_VECTOR_LINE_LAYER, NULL);
  JsonNode *paint_node, *layout_node;

  if ((paint_node = json_object_get_member (object, "paint")))
    {
      JsonObject *paint;

      if (!shumate_vector_json_get_object (paint_node, &paint, error))
        return NULL;

      if (!(layer->color = shumate_vector_expression_from_json (json_object_get_member (paint, "line-color"), error)))
        return NULL;

      if (!(layer->opacity = shumate_vector_expression_from_json (json_object_get_member (paint, "line-opacity"), error)))
        return NULL;

      if (!(layer->width = shumate_vector_expression_from_json (json_object_get_member (paint, "line-width"), error)))
        return NULL;

      {
        JsonArray *dasharray;

        if (!shumate_vector_json_get_array_member (paint, "line-dasharray", &dasharray, error))
          return NULL;

        if (dasharray != NULL)
          {
            int i;
            layer->num_dashes = json_array_get_length (dasharray);
            layer->dashes = g_new (double, layer->num_dashes);
            for (i = 0; i < layer->num_dashes; i ++)
              layer->dashes[i] = json_node_get_double (json_array_get_element (dasharray, i));
          }
      }
    }

  if ((layout_node = json_object_get_member (object, "layout")))
    {
      JsonObject *layout;

      if (!shumate_vector_json_get_object (layout_node, &layout, error))
        return NULL;

      if (!(layer->cap = shumate_vector_expression_from_json (json_object_get_member (layout, "line-cap"), error)))
        return NULL;

      if (!(layer->join = shumate_vector_expression_from_json (json_object_get_member (layout, "line-join"), error)))
        return NULL;
    }

  return (ShumateVectorLayer *)layer;
}


static void
shumate_vector_line_layer_finalize (GObject *object)
{
  ShumateVectorLineLayer *self = SHUMATE_VECTOR_LINE_LAYER (object);

  g_clear_object (&self->color);
  g_clear_object (&self->opacity);
  g_clear_object (&self->width);
  g_clear_object (&self->cap);
  g_clear_object (&self->join);

  g_clear_pointer (&self->dashes, g_free);

  G_OBJECT_CLASS (shumate_vector_line_layer_parent_class)->finalize (object);
}


static void
shumate_vector_line_layer_render (ShumateVectorLayer *layer, ShumateVectorRenderScope *scope)
{
  ShumateVectorLineLayer *self = SHUMATE_VECTOR_LINE_LAYER (layer);
  GdkRGBA color = SHUMATE_VECTOR_COLOR_BLACK;
  double opacity;
  double width;
  g_autofree char *cap = NULL;
  g_autofree char *join = NULL;

  shumate_vector_expression_eval_color (self->color, scope, &color);
  opacity = shumate_vector_expression_eval_number (self->opacity, scope, 1.0);
  width = shumate_vector_expression_eval_number (self->width, scope, 1.0);
  cap = shumate_vector_expression_eval_string (self->cap, scope, NULL);
  join = shumate_vector_expression_eval_string (self->join, scope, NULL);

  shumate_vector_render_scope_exec_geometry (scope);

  cairo_set_source_rgba (scope->cr, color.red, color.green, color.blue, color.alpha * opacity);
  cairo_set_line_width (scope->cr, width * scope->scale);

  if (g_strcmp0 (cap, "round") == 0)
    cairo_set_line_cap (scope->cr, CAIRO_LINE_CAP_ROUND);
  else if (g_strcmp0 (cap, "square") == 0)
    cairo_set_line_cap (scope->cr, CAIRO_LINE_CAP_SQUARE);
  else
    cairo_set_line_cap (scope->cr, CAIRO_LINE_CAP_BUTT);

  if (g_strcmp0 (join, "bevel") == 0)
    cairo_set_line_join (scope->cr, CAIRO_LINE_JOIN_BEVEL);
  else if (g_strcmp0 (join, "round") == 0)
    cairo_set_line_join (scope->cr, CAIRO_LINE_JOIN_ROUND);
  else
    cairo_set_line_join (scope->cr, CAIRO_LINE_JOIN_MITER);

  if (self->dashes == NULL)
    cairo_set_dash (scope->cr, NULL, 0, 0);
  else
    {
      int i;
      g_autofree double *dasharray = g_new (double, self->num_dashes);
      gboolean any_nonzero = FALSE;
      gboolean all_positive = TRUE;

      for (i = 0; i < self->num_dashes; i ++)
        {
          dasharray[i] = self->dashes[i] * width * scope->scale;

          if (dasharray[i] < 0)
            {
              all_positive = FALSE;
              break;
            }
          else if (dasharray[i] != 0)
            any_nonzero = TRUE;
        }

      /* make sure the dasharray is valid */
      if (any_nonzero && all_positive)
        cairo_set_dash (scope->cr, dasharray, self->num_dashes, 0);
      else
        cairo_set_dash (scope->cr, NULL, 0, 0);
    }

  cairo_stroke (scope->cr);
}


static void
shumate_vector_line_layer_class_init (ShumateVectorLineLayerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ShumateVectorLayerClass *layer_class = SHUMATE_VECTOR_LAYER_CLASS (klass);

  object_class->finalize = shumate_vector_line_layer_finalize;
  layer_class->render = shumate_vector_line_layer_render;
}


static void
shumate_vector_line_layer_init (ShumateVectorLineLayer *self)
{
}
