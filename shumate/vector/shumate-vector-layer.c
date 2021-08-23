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

#include <json-glib/json-glib.h>
#include "shumate-vector-background-layer-private.h"
#include "shumate-vector-layer-private.h"

typedef struct
{
  GObject parent_instance;

  double minzoom;
  double maxzoom;
  char *source_layer;
} ShumateVectorLayerPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (ShumateVectorLayer, shumate_vector_layer, G_TYPE_OBJECT)


ShumateVectorLayer *
shumate_vector_layer_create_from_json (JsonObject *object, GError **error)
{
  ShumateVectorLayer *layer;
  ShumateVectorLayerPrivate *priv;
  const char *type = json_object_get_string_member_with_default (object, "type", NULL);

  if (type == NULL)
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "Expected element of \"layer\" to have a string member \"type\"");
      return NULL;
    }

  if (g_strcmp0 (type, "background") == 0)
    layer = shumate_vector_background_layer_create_from_json (object, error);
  else
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "Unsupported layer type \"%s\"", type);
      return NULL;
    }

  if (layer == NULL)
    /* A problem occurred in one of the constructors above, and error is already
     * set */
    return NULL;

  priv = shumate_vector_layer_get_instance_private (layer);
  priv->minzoom = json_object_get_double_member_with_default (object, "minzoom", 0.0);
  priv->maxzoom = json_object_get_double_member_with_default (object, "maxzoom", 1000000000.0);
  priv->source_layer = g_strdup (json_object_get_string_member_with_default (object, "source-layer", NULL));

  return layer;
}


static void
shumate_vector_layer_finalize (GObject *object)
{
  ShumateVectorLayer *self = (ShumateVectorLayer *)object;
  ShumateVectorLayerPrivate *priv = shumate_vector_layer_get_instance_private (self);

  g_clear_pointer (&priv->source_layer, g_free);

  G_OBJECT_CLASS (shumate_vector_layer_parent_class)->finalize (object);
}


static void
shumate_vector_layer_class_init (ShumateVectorLayerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ShumateVectorLayerClass *layer_class = SHUMATE_VECTOR_LAYER_CLASS (klass);

  object_class->finalize = shumate_vector_layer_finalize;

  layer_class->render = NULL;
}

static void
shumate_vector_layer_init (ShumateVectorLayer *self)
{
}


/**
 * shumate_vector_layer_render:
 * @self: a [class@VectorLayer]
 *
 * Renders the layer by calling the [vfunc@VectorLayer.render] virtual method.
 */
void
shumate_vector_layer_render (ShumateVectorLayer *self, cairo_t *cr)
{
  g_return_if_fail (SHUMATE_IS_VECTOR_LAYER (self));

  SHUMATE_VECTOR_LAYER_GET_CLASS (self)->render (self, cr);
}
