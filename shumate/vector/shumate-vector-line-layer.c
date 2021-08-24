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
#include "shumate-vector-line-layer-private.h"
#include "shumate-vector-utils-private.h"

struct _ShumateVectorLineLayer
{
  ShumateVectorLayer parent_instance;

  GdkRGBA color;
  double opacity;
  double width;
};

G_DEFINE_TYPE (ShumateVectorLineLayer, shumate_vector_line_layer, SHUMATE_TYPE_VECTOR_LAYER)


ShumateVectorLayer *
shumate_vector_line_layer_create_from_json (JsonObject *object, GError **error)
{
  ShumateVectorLineLayer *layer = g_object_new (SHUMATE_TYPE_VECTOR_LINE_LAYER, NULL);
  JsonNode *paint_node;

  if ((paint_node = json_object_get_member (object, "paint")))
    {
      JsonObject *paint;

      if (!shumate_vector_json_get_object (paint_node, &paint, error))
        return NULL;

      gdk_rgba_parse (&layer->color, json_object_get_string_member_with_default (paint, "line-color", "#000000"));
      layer->opacity = json_object_get_double_member_with_default (paint, "line-opacity", 1.0);
      layer->width = json_object_get_double_member_with_default (paint, "line-width", 1.0);
    }

  return (ShumateVectorLayer *)layer;
}


static void
shumate_vector_line_layer_render (ShumateVectorLayer *layer, ShumateVectorRenderScope *scope)
{
  ShumateVectorLineLayer *self = SHUMATE_VECTOR_LINE_LAYER (layer);

  shumate_vector_render_scope_exec_geometry (scope);

  cairo_set_source_rgba (scope->cr, self->color.red, self->color.green, self->color.blue, self->opacity);
  cairo_set_line_width (scope->cr, self->width * scope->scale);
  cairo_stroke (scope->cr);
}


static void
shumate_vector_line_layer_class_init (ShumateVectorLineLayerClass *klass)
{
  ShumateVectorLayerClass *layer_class = SHUMATE_VECTOR_LAYER_CLASS (klass);

  layer_class->render = shumate_vector_line_layer_render;
}


static void
shumate_vector_line_layer_init (ShumateVectorLineLayer *self)
{
}
