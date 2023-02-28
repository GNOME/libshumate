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
#include "shumate-vector-expression-private.h"
#include "shumate-vector-fill-layer-private.h"
#include "shumate-vector-layer-private.h"
#include "shumate-vector-line-layer-private.h"
#include "shumate-vector-symbol-layer-private.h"

typedef struct
{
  GObject parent_instance;

  char *id;

  double minzoom;
  double maxzoom;
  char *source_layer;
  ShumateVectorExpression *filter;

} ShumateVectorLayerPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (ShumateVectorLayer, shumate_vector_layer, G_TYPE_OBJECT)


ShumateVectorLayer *
shumate_vector_layer_create_from_json (JsonObject *object, GError **error)
{
  ShumateVectorLayer *layer;
  ShumateVectorLayerPrivate *priv;
  JsonNode *filter;
  const char *type = json_object_get_string_member_with_default (object, "type", NULL);

  if (type == NULL)
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "Expected element of \"layer\" to have a string member \"type\"");
      return NULL;
    }

  if (g_strcmp0 (type, "background") == 0)
    layer = shumate_vector_background_layer_create_from_json (object, error);
  else if (g_strcmp0 (type, "fill") == 0)
    layer = shumate_vector_fill_layer_create_from_json (object, error);
  else if (g_strcmp0 (type, "line") == 0)
    layer = shumate_vector_line_layer_create_from_json (object, error);
  else if (g_strcmp0 (type, "symbol") == 0)
    layer = shumate_vector_symbol_layer_create_from_json (object, error);
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
  priv->id = g_strdup (json_object_get_string_member_with_default (object, "id", NULL));
  priv->minzoom = json_object_get_double_member_with_default (object, "minzoom", 0.0);
  priv->maxzoom = json_object_get_double_member_with_default (object, "maxzoom", 1000000000.0);
  priv->source_layer = g_strdup (json_object_get_string_member_with_default (object, "source-layer", NULL));

  filter = json_object_get_member (object, "filter");
  if (filter != NULL)
    {
      if (!(priv->filter = shumate_vector_expression_from_json (filter, NULL, error)))
        return NULL;
    }

  return layer;
}


static void
shumate_vector_layer_finalize (GObject *object)
{
  ShumateVectorLayer *self = (ShumateVectorLayer *)object;
  ShumateVectorLayerPrivate *priv = shumate_vector_layer_get_instance_private (self);

  g_clear_pointer (&priv->id, g_free);
  g_clear_pointer (&priv->source_layer, g_free);
  g_clear_object (&priv->filter);

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
 * Renders the layer.
 */
void
shumate_vector_layer_render (ShumateVectorLayer *self, ShumateVectorRenderScope *scope)
{
  ShumateVectorLayerPrivate *priv = shumate_vector_layer_get_instance_private (self);

  g_return_if_fail (SHUMATE_IS_VECTOR_LAYER (self));

  if (scope->zoom_level < priv->minzoom || scope->zoom_level > priv->maxzoom)
    return;

  scope->feature = NULL;

  if (priv->source_layer == NULL)
    /* Style layers with no source layer are rendered once */
    SHUMATE_VECTOR_LAYER_GET_CLASS (self)->render (self, scope);
  else if (shumate_vector_render_scope_find_layer (scope, priv->source_layer))
    {
      /* Style layers with a source layer are rendered once for each feature
       * in that layer, if it exists */

      cairo_save (scope->cr);

      scope->scale = (double) scope->layer->extent / scope->target_size / scope->overzoom_scale;
      cairo_scale (scope->cr, 1.0 / scope->scale, 1.0 / scope->scale);

      cairo_translate (scope->cr, -scope->overzoom_x * scope->layer->extent, -scope->overzoom_y * scope->layer->extent);

      for (int j = 0; j < scope->layer->n_features; j ++)
        {
          scope->feature = scope->layer->features[j];
          if (priv->filter == NULL || shumate_vector_expression_eval_boolean (priv->filter, scope, FALSE))
            SHUMATE_VECTOR_LAYER_GET_CLASS (self)->render (self, scope);
        }

      cairo_restore (scope->cr);
    }
}

const char *
shumate_vector_layer_get_id (ShumateVectorLayer *self)
{
  ShumateVectorLayerPrivate *priv = shumate_vector_layer_get_instance_private (self);
  g_return_val_if_fail (SHUMATE_IS_VECTOR_LAYER (self), NULL);
  return priv->id;
}

const char *
shumate_vector_layer_get_source_layer (ShumateVectorLayer *self)
{
  ShumateVectorLayerPrivate *priv = shumate_vector_layer_get_instance_private (self);
  g_return_val_if_fail (SHUMATE_IS_VECTOR_LAYER (self), NULL);
  return priv->source_layer;
}
