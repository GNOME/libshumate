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
#include "shumate-vector-background-layer-private.h"
#include "shumate-vector-utils-private.h"

struct _ShumateVectorBackgroundLayer
{
  ShumateVectorLayer parent_instance;

  GdkRGBA color;
  double opacity;
};

G_DEFINE_TYPE (ShumateVectorBackgroundLayer, shumate_vector_background_layer, SHUMATE_TYPE_VECTOR_LAYER)


ShumateVectorLayer *
shumate_vector_background_layer_create_from_json (JsonObject *object, GError **error)
{
  ShumateVectorBackgroundLayer *layer = g_object_new (SHUMATE_TYPE_VECTOR_BACKGROUND_LAYER, NULL);
  JsonNode *paint_node;

  if ((paint_node = json_object_get_member (object, "paint")))
    {
      JsonObject *paint;

      if (!shumate_vector_json_get_object (paint_node, &paint, error))
        return NULL;

      gdk_rgba_parse (&layer->color, json_object_get_string_member_with_default (paint, "background-color", "#000000"));
      layer->opacity = json_object_get_double_member_with_default (paint, "background-opacity", 1.0);
    }

  return (ShumateVectorLayer *)layer;
}


static void
shumate_vector_background_layer_render (ShumateVectorLayer *layer, cairo_t *cr)
{
  ShumateVectorBackgroundLayer *self = SHUMATE_VECTOR_BACKGROUND_LAYER (layer);

  gdk_cairo_set_source_rgba (cr, &self->color);
  cairo_paint_with_alpha (cr, self->opacity);
}


static void
shumate_vector_background_layer_class_init (ShumateVectorBackgroundLayerClass *klass)
{
  ShumateVectorLayerClass *layer_class = SHUMATE_VECTOR_LAYER_CLASS (klass);

  layer_class->render = shumate_vector_background_layer_render;
}


static void
shumate_vector_background_layer_init (ShumateVectorBackgroundLayer *self)
{
}
