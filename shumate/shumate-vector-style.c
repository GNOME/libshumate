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

#include <cairo/cairo.h>

#include "shumate-vector-style.h"

struct _ShumateVectorStyle
{
  GObject parent_instance;

  char *style_json;
};

G_DEFINE_TYPE (ShumateVectorStyle, shumate_vector_style, G_TYPE_OBJECT)

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

  return g_object_new (SHUMATE_TYPE_VECTOR_STYLE,
                       "style-json", style_json,
                       NULL);
}


static void
shumate_vector_style_finalize (GObject *object)
{
  ShumateVectorStyle *self = (ShumateVectorStyle *)object;

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
shumate_vector_style_render (ShumateVectorStyle *self, int size)
{
  GdkTexture *texture;
  cairo_t *cr;
  cairo_surface_t *surface;

  g_return_val_if_fail (SHUMATE_IS_VECTOR_STYLE (self), NULL);

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, size, size);
  cr = cairo_create (surface);

  cairo_set_source_rgb (cr, 0, 0, 0);
  cairo_paint (cr);

  texture = texture_new_for_surface (surface);

  cairo_destroy (cr);
  cairo_surface_destroy (surface);

  return texture;
}

