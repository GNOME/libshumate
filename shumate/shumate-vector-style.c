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
#include <cairo/cairo.h>

#include "vector/shumate-vector-render-scope-private.h"
#include "vector/shumate-vector-utils-private.h"
#include "vector/shumate-vector-layer-private.h"
#include "shumate-vector-style.h"

struct _ShumateVectorStyle
{
  GObject parent_instance;

  char *style_json;

  GPtrArray *layers;
};

static void shumate_vector_style_initable_iface_init (GInitableIface *iface);

G_DEFINE_TYPE_WITH_CODE (ShumateVectorStyle, shumate_vector_style, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, shumate_vector_style_initable_iface_init))

enum {
  PROP_0,
  PROP_STYLE_JSON,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];


/**
 * shumate_vector_style_create:
 * @style_json: a JSON string
 * @error: return location for a [class@GLib.Error], or %NULL
 *
 * Creates a vector style from a JSON definition.
 *
 * Returns: (transfer full): a new [class@VectorStyle] object, or %NULL if the
 * style could not be parsed
 */
ShumateVectorStyle *
shumate_vector_style_create (const char *style_json, GError **error)
{
  g_return_val_if_fail (style_json != NULL, NULL);

  return g_initable_new (SHUMATE_TYPE_VECTOR_STYLE, NULL, error,
                         "style-json", style_json,
                         NULL);
}


static void
shumate_vector_style_finalize (GObject *object)
{
  ShumateVectorStyle *self = (ShumateVectorStyle *)object;

  g_clear_pointer (&self->layers, g_ptr_array_unref);
  g_clear_pointer (&self->style_json, g_free);

  G_OBJECT_CLASS (shumate_vector_style_parent_class)->finalize (object);
}

static void
shumate_vector_style_get_property (GObject    *object,
                                   guint       prop_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  ShumateVectorStyle *self = SHUMATE_VECTOR_STYLE (object);

  switch (prop_id)
    {
    case PROP_STYLE_JSON:
      g_value_set_string (value, self->style_json);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
shumate_vector_style_set_property (GObject      *object,
                                   guint         prop_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  ShumateVectorStyle *self = SHUMATE_VECTOR_STYLE (object);

  switch (prop_id)
    {
    case PROP_STYLE_JSON:
      /* Property is construct only, so it should only be set once */
      g_assert (self->style_json == NULL);
      self->style_json = g_strdup (g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
shumate_vector_style_class_init (ShumateVectorStyleClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = shumate_vector_style_finalize;
  object_class->get_property = shumate_vector_style_get_property;
  object_class->set_property = shumate_vector_style_set_property;

  properties[PROP_STYLE_JSON] =
    g_param_spec_string ("style-json",
                         "Style JSON",
                         "Style JSON",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);
}


static gboolean
shumate_vector_style_initable_init (GInitable     *initable,
                                    GCancellable  *cancellable,
                                    GError       **error)
{
  ShumateVectorStyle *self = (ShumateVectorStyle *)initable;
  g_autoptr(JsonNode) node = NULL;
  JsonNode *layers_node;
  JsonObject *object;

  g_return_val_if_fail (SHUMATE_IS_VECTOR_STYLE (self), FALSE);
  g_return_val_if_fail (self->style_json != NULL, FALSE);

  if (!(node = json_from_string (self->style_json, error)))
    return FALSE;

  if (!shumate_vector_json_get_object (node, &object, error))
    return FALSE;

  self->layers = g_ptr_array_new_with_free_func (g_object_unref);
  if ((layers_node = json_object_get_member (object, "layers")))
    {
      JsonArray *layers;

      if (!shumate_vector_json_get_array (layers_node, &layers, error))
        return FALSE;

      for (int i = 0, n = json_array_get_length (layers); i < n; i ++)
        {
          JsonNode *layer_node = json_array_get_element (layers, i);
          JsonObject *layer_obj;
          ShumateVectorLayer *layer;

          if (!shumate_vector_json_get_object (layer_node, &layer_obj, error))
            return FALSE;

          if (!(layer = shumate_vector_layer_create_from_json (layer_obj, error)))
            {
              g_prefix_error (error, "layer '%s': ", json_object_get_string_member (layer_obj, "id"));
              return FALSE;
            }

          g_ptr_array_add (self->layers, layer);
        }
    }

  return TRUE;
}


static void
shumate_vector_style_initable_iface_init (GInitableIface *iface)
{
  iface->init = shumate_vector_style_initable_init;
}


static void
shumate_vector_style_init (ShumateVectorStyle *self)
{
}


/**
 * shumate_vector_style_get_style_json:
 * @self: a [class@VectorStyle]
 *
 * Gets the JSON string from which this vector style was loaded.
 *
 * Returns: (nullable): the style JSON, or %NULL if none is set
 */
const char *
shumate_vector_style_get_style_json (ShumateVectorStyle *self)
{
  g_return_val_if_fail (SHUMATE_IS_VECTOR_STYLE (self), NULL);

  return self->style_json;
}


static GdkTexture *
texture_new_for_surface (cairo_surface_t *surface)
{
  g_autoptr(GBytes) bytes = NULL;
  GdkTexture *texture;

  g_return_val_if_fail (cairo_surface_get_type (surface) == CAIRO_SURFACE_TYPE_IMAGE, NULL);
  g_return_val_if_fail (cairo_image_surface_get_width (surface) > 0, NULL);
  g_return_val_if_fail (cairo_image_surface_get_height (surface) > 0, NULL);

  bytes = g_bytes_new_with_free_func (cairo_image_surface_get_data (surface),
                                      cairo_image_surface_get_height (surface)
                                      * cairo_image_surface_get_stride (surface),
                                      (GDestroyNotify) cairo_surface_destroy,
                                      cairo_surface_reference (surface));

  texture = gdk_memory_texture_new (cairo_image_surface_get_width (surface),
                                    cairo_image_surface_get_height (surface),
                                    GDK_MEMORY_B8G8R8A8_PREMULTIPLIED,
                                    bytes,
                                    cairo_image_surface_get_stride (surface));

  return texture;
}


/**
 * shumate_vector_style_render:
 * @self: a [class@VectorStyle]
 *
 * Renders a tile to a texture using this style.
 *
 * Returns: (transfer full): a [class@Gdk.Texture] containing the rendered tile
 */
GdkTexture *
shumate_vector_style_render (ShumateVectorStyle *self, int texture_size, GBytes *tile_data, double zoom_level)
{
  ShumateVectorRenderScope scope;
  GdkTexture *texture;
  cairo_surface_t *surface;
  gconstpointer data;
  gsize len;

  g_return_val_if_fail (SHUMATE_IS_VECTOR_STYLE (self), NULL);

  scope.target_size = texture_size;
  scope.zoom_level = zoom_level;

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, texture_size, texture_size);
  scope.cr = cairo_create (surface);

  data = g_bytes_get_data (tile_data, &len);
  scope.tile = vector_tile__tile__unpack (NULL, len, data);

  if (scope.tile != NULL)
    for (int i = 0; i < self->layers->len; i ++)
      shumate_vector_layer_render ((ShumateVectorLayer *)self->layers->pdata[i], &scope);

  texture = texture_new_for_surface (surface);

  cairo_destroy (scope.cr);
  cairo_surface_destroy (surface);
  vector_tile__tile__free_unpacked (scope.tile, NULL);

  return texture;
}

/**
 * shumate_style_error_quark:
 *
 * Returns: a #GQuark
 */
G_DEFINE_QUARK (shumate-style-error-quark, shumate_style_error);
