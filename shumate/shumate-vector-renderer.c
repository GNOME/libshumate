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

#include "shumate-vector-renderer.h"
#include "shumate-tile-downloader.h"
#include "shumate-tile-private.h"

/**
 * ShumateVectorRenderer:
 *
 * A [class@MapSource] that renders tiles from a given vector data source.
 */

#ifdef SHUMATE_HAS_VECTOR_RENDERER
#include <json-glib/json-glib.h>
#include <cairo/cairo.h>

#include "vector/shumate-vector-render-scope-private.h"
#include "vector/shumate-vector-symbol-info-private.h"
#include "vector/shumate-vector-utils-private.h"
#include "vector/shumate-vector-layer-private.h"
#endif

struct _ShumateVectorRenderer
{
  ShumateMapSource parent_instance;

  char *source_name;
  ShumateDataSource *data_source;
  GPtrArray *tiles;

#ifdef SHUMATE_HAS_VECTOR_RENDERER
  ShumateVectorSpriteSheet *sprites;
#endif

  char *style_json;

  GPtrArray *layers;
};

static void shumate_vector_renderer_initable_iface_init (GInitableIface *iface);

G_DEFINE_TYPE_WITH_CODE (ShumateVectorRenderer, shumate_vector_renderer, SHUMATE_TYPE_MAP_SOURCE,
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, shumate_vector_renderer_initable_iface_init))

enum {
  PROP_0,
  PROP_STYLE_JSON,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];


/**
 * shumate_vector_renderer_new:
 * @id: an ID for the map source
 * @style_json: a vector style
 * @error: return location for a #GError, or %NULL
 *
 * Creates a new [class@VectorRenderer] from the given JSON style.
 *
 * The stylesheet should contain a list of tile sources. Tiles will be
 * downloaded using [class@TileDownloader]s.
 *
 * See the [MapLibre Style Specification](https://maplibre.org/maplibre-gl-js-docs/style-spec/)
 * for details on @style_json, but be aware that libshumate does not support
 * every feature of the specification.
 *
 * Returns: (transfer full): a newly constructed [class@VectorRenderer], or %NULL if @error is set
 */
ShumateVectorRenderer *
shumate_vector_renderer_new (const char  *id,
                             const char  *style_json,
                             GError     **error)
{
  g_return_val_if_fail (id != NULL, NULL);
  g_return_val_if_fail (style_json != NULL, NULL);

  return g_initable_new (SHUMATE_TYPE_VECTOR_RENDERER, NULL, error,
                         "id", id,
                         "style-json", style_json,
                         NULL);
}


/**
 * shumate_vector_renderer_is_supported:
 *
 * Checks whether libshumate was compiled with vector tile support. If it was
 * not, vector renderers cannot be created or used.
 *
 * Returns: %TRUE if libshumate was compiled with `-Dvector_renderer=true` or
 * %FALSE if it was not
 */
gboolean
shumate_vector_renderer_is_supported (void)
{
#ifdef SHUMATE_HAS_VECTOR_RENDERER
  return TRUE;
#else
  return FALSE;
#endif
}


static void
on_data_source_received_data (ShumateVectorRenderer *self,
                              int                    x,
                              int                    y,
                              int                    zoom_level,
                              GBytes                *bytes,
                              ShumateDataSource     *data_source);


static void
shumate_vector_renderer_finalize (GObject *object)
{
  ShumateVectorRenderer *self = (ShumateVectorRenderer *)object;

  g_clear_pointer (&self->layers, g_ptr_array_unref);
  g_clear_pointer (&self->style_json, g_free);
  g_clear_pointer (&self->source_name, g_free);
  g_clear_object (&self->data_source);
  g_clear_pointer (&self->tiles, g_ptr_array_unref);
#ifdef SHUMATE_HAS_VECTOR_RENDERER
  g_clear_object (&self->sprites);
#endif

  G_OBJECT_CLASS (shumate_vector_renderer_parent_class)->finalize (object);
}

static void
shumate_vector_renderer_get_property (GObject    *object,
                                      guint       prop_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  ShumateVectorRenderer *self = SHUMATE_VECTOR_RENDERER (object);

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
shumate_vector_renderer_set_property (GObject      *object,
                                      guint         prop_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  ShumateVectorRenderer *self = SHUMATE_VECTOR_RENDERER (object);

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

static void shumate_vector_renderer_fill_tile_async (ShumateMapSource    *map_source,
                                                     ShumateTile         *tile,
                                                     GCancellable        *cancellable,
                                                     GAsyncReadyCallback  callback,
                                                     gpointer             user_data);

static gboolean shumate_vector_renderer_fill_tile_finish (ShumateMapSource  *map_source,
                                                          GAsyncResult      *result,
                                                          GError           **error);

static void
shumate_vector_renderer_class_init (ShumateVectorRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ShumateMapSourceClass *map_source_class = SHUMATE_MAP_SOURCE_CLASS (klass);

  object_class->finalize = shumate_vector_renderer_finalize;
  object_class->get_property = shumate_vector_renderer_get_property;
  object_class->set_property = shumate_vector_renderer_set_property;

  map_source_class->fill_tile_async = shumate_vector_renderer_fill_tile_async;
  map_source_class->fill_tile_finish = shumate_vector_renderer_fill_tile_finish;

  /**
   * ShumateVectorRenderer:style-json:
   *
   * A map style, in [Mapbox Style Specification](https://docs.mapbox.com/mapbox-gl-js/style-spec/)
   * format.
   *
   * Note that not all features of the specification are supported.
   */
  properties[PROP_STYLE_JSON] =
    g_param_spec_string ("style-json",
                         "Style JSON",
                         "Style JSON",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);
}


static gboolean
shumate_vector_renderer_initable_init (GInitable     *initable,
                                       GCancellable  *cancellable,
                                       GError       **error)
{
#ifdef SHUMATE_HAS_VECTOR_RENDERER
  ShumateVectorRenderer *self = (ShumateVectorRenderer *)initable;
  g_autoptr(JsonNode) node = NULL;
  JsonNode *layers_node;
  JsonNode *sources_node;
  JsonObject *object;
  const char *style_name;

  g_return_val_if_fail (SHUMATE_IS_VECTOR_RENDERER (self), FALSE);
  g_return_val_if_fail (self->style_json != NULL, FALSE);

  if (!(node = json_from_string (self->style_json, error)))
    return FALSE;

  if (!shumate_vector_json_get_object (node, &object, error))
    return FALSE;

  if (!shumate_vector_json_get_string_member (object, "name", &style_name, error))
    return FALSE;
  if (style_name != NULL)
    shumate_map_source_set_name (SHUMATE_MAP_SOURCE (self), style_name);

  if ((sources_node = json_object_get_member (object, "sources")) == NULL)
    {
      g_set_error (error,
                   SHUMATE_STYLE_ERROR,
                   SHUMATE_STYLE_ERROR_UNSUPPORTED,
                   "a data source is required");
      return FALSE;
    }
  else
    {
      JsonObject *sources;
      JsonObjectIter iter;
      const char *source_name;
      JsonNode *source_node;
      int minzoom = 30, maxzoom = 0;

      if (!shumate_vector_json_get_object (sources_node, &sources, error))
        return FALSE;

      if (json_object_get_size (sources) > 1)
        {
          /* TODO: Support multiple data sources */
          g_set_error (error,
                       SHUMATE_STYLE_ERROR,
                       SHUMATE_STYLE_ERROR_UNSUPPORTED,
                       "ShumateVectorRenderer does not currently support multiple data sources");
          return FALSE;
        }
      else if (json_object_get_size (object) == 0)
        {
          g_set_error (error,
                       SHUMATE_STYLE_ERROR,
                       SHUMATE_STYLE_ERROR_UNSUPPORTED,
                       "a data source is required");
          return FALSE;
        }

      json_object_iter_init (&iter, sources);
      while (json_object_iter_next (&iter, &source_name, &source_node))
        {
          JsonObject *source_object;
          const char *source_type;
          const char *url;
          JsonArray *tiles;
          const char *url_template;
          ShumateDataSource *data_source;

          if (!shumate_vector_json_get_object (source_node, &source_object, error))
            return FALSE;

          if (!shumate_vector_json_get_string_member (source_object, "type", &source_type, error)
              || !shumate_vector_json_get_string_member (source_object, "url", &url, error)
              || !shumate_vector_json_get_array_member (source_object, "tiles", &tiles, error))
            return FALSE;

          if (g_strcmp0 (source_type, "vector") != 0)
            {
              g_set_error (error,
                           SHUMATE_STYLE_ERROR,
                           SHUMATE_STYLE_ERROR_UNSUPPORTED,
                           "ShumateVectorRenderer currently only supports vector sources.");
              return FALSE;
            }

          if (url != NULL)
            {
              g_set_error (error,
                           SHUMATE_STYLE_ERROR,
                           SHUMATE_STYLE_ERROR_UNSUPPORTED,
                           "ShumateVectorRenderer does not currently support TileJSON links. "
                           "Please embed the TileJSON data directly into the style.");
              return FALSE;
            }

          if (tiles == NULL || json_array_get_length (tiles) == 0)
            {
              g_set_error (error,
                           SHUMATE_STYLE_ERROR,
                           SHUMATE_STYLE_ERROR_MALFORMED_STYLE,
                           "Expected 'tiles' array to contain at least one element");
              return FALSE;
            }

          if (!shumate_vector_json_get_string (json_array_get_element (tiles, 0),
                                               &url_template,
                                               error))
            return FALSE;

          minzoom = MIN (minzoom, json_object_get_int_member_with_default (source_object, "minzoom", 0));
          maxzoom = MAX (maxzoom, json_object_get_int_member_with_default (source_object, "maxzoom", 30));

          data_source = SHUMATE_DATA_SOURCE (shumate_tile_downloader_new (url_template));
          g_signal_connect_object (data_source, "received-data", (GCallback)on_data_source_received_data, self, G_CONNECT_SWAPPED);

          self->source_name = g_strdup (source_name);
          self->data_source = data_source;
        }

      if (minzoom < maxzoom)
        {
          shumate_map_source_set_min_zoom_level (SHUMATE_MAP_SOURCE (self), minzoom);
          shumate_map_source_set_max_zoom_level (SHUMATE_MAP_SOURCE (self), maxzoom);
        }
    }

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

  /* According to the style spec, this is not configurable for vector tiles */
  shumate_map_source_set_tile_size (SHUMATE_MAP_SOURCE (self), 512);

  return TRUE;
#else
  g_set_error (error,
               SHUMATE_STYLE_ERROR,
               SHUMATE_STYLE_ERROR_SUPPORT_OMITTED,
               "Libshumate was compiled without support for vector tiles, so a "
               "ShumateVectorRenderer may not be constructed. You can fix this "
               "by compiling libshumate with `-Dvector_renderer=true` or by "
               "checking `shumate_vector_renderer_is_supported ()` before trying "
               "to construct a ShumateVectorRenderer.");
  return FALSE;
#endif
}


static void
shumate_vector_renderer_initable_iface_init (GInitableIface *iface)
{
  iface->init = shumate_vector_renderer_initable_init;
}


static void
shumate_vector_renderer_init (ShumateVectorRenderer *self)
{
  self->tiles = g_ptr_array_new_full (0, g_object_unref);
}


/**
 * shumate_vector_renderer_get_style_json:
 * @self: a [class@VectorStyle]
 *
 * Gets the JSON string from which this vector style was loaded.
 *
 * Returns: (nullable): the style JSON, or %NULL if none is set
 */
const char *
shumate_vector_renderer_get_style_json (ShumateVectorRenderer *self)
{
  g_return_val_if_fail (SHUMATE_IS_VECTOR_RENDERER (self), NULL);

  return self->style_json;
}


/**
 * shumate_vector_renderer_set_sprite_sheet_data:
 * @self: a [class@VectorRenderer]
 * @sprites_pixbuf: a [class@GdkPixbuf.Pixbuf]
 * @sprites_json: a JSON string
 * @error: return location for a #GError, or %NULL
 *
 * Sets the sprite sheet used by the style JSON to render icons and textures.
 *
 * See <https://maplibre.org/maplibre-gl-js-docs/style-spec/sprite/> for
 * details about the spritesheet format. Most stylesheets provide these files
 * along with the main style JSON.
 *
 * Returns: whether the sprite sheet was loaded successfully
 */
gboolean
shumate_vector_renderer_set_sprite_sheet_data (ShumateVectorRenderer  *self,
                                               GdkPixbuf              *sprites_pixbuf,
                                               const char             *sprites_json,
                                               GError                **error)
{
  g_return_val_if_fail (SHUMATE_IS_VECTOR_RENDERER (self), FALSE);
  g_return_val_if_fail (GDK_IS_PIXBUF (sprites_pixbuf), FALSE);
  g_return_val_if_fail (sprites_json != NULL, FALSE);

#ifdef SHUMATE_HAS_VECTOR_RENDERER
  g_clear_object (&self->sprites);
  self->sprites = shumate_vector_sprite_sheet_new (sprites_pixbuf, sprites_json, NULL, error);
  return self->sprites != NULL;
#else
  g_set_error (error,
               SHUMATE_STYLE_ERROR,
               SHUMATE_STYLE_ERROR_SUPPORT_OMITTED,
               "Libshumate was compiled without support for vector tiles.");
  return FALSE;
#endif
}


#ifdef SHUMATE_HAS_VECTOR_RENDERER
static GdkTexture *
texture_new_for_surface (cairo_surface_t *surface)
{
  g_autoptr(GBytes) bytes = NULL;
  GdkTexture *texture;

  g_return_val_if_fail (cairo_surface_get_type (surface) == CAIRO_SURFACE_TYPE_IMAGE, NULL);
  g_return_val_if_fail (cairo_image_surface_get_width (surface) > 0, NULL);
  g_return_val_if_fail (cairo_image_surface_get_height (surface) > 0, NULL);

  bytes = g_bytes_new_with_free_func (cairo_image_surface_get_data (surface),
                                      (gsize) cairo_image_surface_get_height (surface)
                                      * (gsize) cairo_image_surface_get_stride (surface),
                                      (GDestroyNotify) cairo_surface_destroy,
                                      cairo_surface_reference (surface));

  texture = gdk_memory_texture_new (cairo_image_surface_get_width (surface),
                                    cairo_image_surface_get_height (surface),
                                    GDK_MEMORY_B8G8R8A8_PREMULTIPLIED,
                                    bytes,
                                    cairo_image_surface_get_stride (surface));

  return texture;
}
#endif


static void on_data_source_done (GObject      *object,
                                 GAsyncResult *res,
                                 gpointer      user_data);

static void
shumate_vector_renderer_fill_tile_async (ShumateMapSource    *map_source,
                                         ShumateTile         *tile,
                                         GCancellable        *cancellable,
                                         GAsyncReadyCallback  callback,
                                         gpointer             user_data)
{
  ShumateVectorRenderer *self = (ShumateVectorRenderer *)map_source;
  g_autoptr(GTask) task = NULL;

  g_return_if_fail (SHUMATE_IS_VECTOR_RENDERER (self));
  g_return_if_fail (SHUMATE_IS_TILE (tile));
  g_return_if_fail (!cancellable || G_IS_CANCELLABLE (cancellable));

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_source_tag (task, shumate_vector_renderer_fill_tile_async);
  g_task_set_task_data (task, g_object_ref (tile), (GDestroyNotify)g_object_unref);

  g_ptr_array_add (self->tiles, g_object_ref (tile));

  shumate_data_source_get_tile_data_async (self->data_source,
                                           shumate_tile_get_x (tile),
                                           shumate_tile_get_y (tile),
                                           shumate_tile_get_zoom_level (tile),
                                           cancellable,
                                           on_data_source_done,
                                           g_steal_pointer (&task));
}

static gboolean
shumate_vector_renderer_fill_tile_finish (ShumateMapSource  *map_source,
                                          GAsyncResult      *result,
                                          GError           **error)
{
  ShumateVectorRenderer *self = (ShumateVectorRenderer *)map_source;

  g_return_val_if_fail (SHUMATE_IS_VECTOR_RENDERER (self), FALSE);
  g_return_val_if_fail (g_task_is_valid (result, self), FALSE);

  return g_task_propagate_boolean (G_TASK (result), error);
}

static void
on_data_source_done (GObject *object, GAsyncResult *res, gpointer user_data)
{
  ShumateDataSource *data_source = SHUMATE_DATA_SOURCE (object);
  g_autoptr(GTask) task = G_TASK (user_data);
  ShumateVectorRenderer *self = g_task_get_source_object (task);
  ShumateTile *tile = g_task_get_task_data (task);
  GError *error = NULL;
  g_autoptr(GBytes) bytes = NULL;

  g_ptr_array_remove_fast (self->tiles, tile);

  bytes = shumate_data_source_get_tile_data_finish (data_source, res, &error);

  if (bytes == NULL)
    g_task_return_error (task, error);
  else
    {
      shumate_tile_set_state (tile, SHUMATE_STATE_DONE);
      g_task_return_boolean (task, TRUE);
    }
}

static void
render (ShumateVectorRenderer *self,
        ShumateTile           *tile,
        GBytes                *tile_data,
        double                 zoom_level)
{
#ifdef SHUMATE_HAS_VECTOR_RENDERER
  ShumateVectorRenderScope scope;
  g_autoptr(GdkTexture) texture = NULL;
  cairo_surface_t *surface;
  gconstpointer data;
  gsize len;
  g_autoptr(GPtrArray) symbols = g_ptr_array_new_with_free_func (shumate_vector_symbol_info_unref);
  int texture_size;

  g_assert (SHUMATE_IS_VECTOR_RENDERER (self));
  g_assert (SHUMATE_IS_TILE (tile));

  texture_size = shumate_tile_get_size (tile);
  scope.target_size = texture_size;
  scope.tile_x = shumate_tile_get_x (tile);
  scope.tile_y = shumate_tile_get_y (tile);
  scope.zoom_level = zoom_level;
  scope.symbols = symbols;
  scope.sprites = self->sprites;

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, texture_size, texture_size);
  scope.cr = cairo_create (surface);

  data = g_bytes_get_data (tile_data, &len);
  scope.tile = vector_tile__tile__unpack (NULL, len, data);

  if (scope.tile != NULL)
    for (int i = 0; i < self->layers->len; i ++)
      shumate_vector_layer_render ((ShumateVectorLayer *)self->layers->pdata[i], &scope);

  texture = texture_new_for_surface (surface);
  shumate_tile_set_paintable (tile, GDK_PAINTABLE (texture));
  shumate_tile_set_symbols (tile, symbols);

  cairo_destroy (scope.cr);
  cairo_surface_destroy (surface);
  vector_tile__tile__free_unpacked (scope.tile, NULL);
#else
  g_return_if_reached ();
#endif
}

static void
on_data_source_received_data (ShumateVectorRenderer *self,
                              int                    x,
                              int                    y,
                              int                    zoom_level,
                              GBytes                *bytes,
                              ShumateDataSource     *data_source)
{
  int i;
  ShumateTile *tile;

  g_assert (SHUMATE_IS_VECTOR_RENDERER (self));
  g_assert (SHUMATE_IS_DATA_SOURCE (data_source));

  for (i = 0; i < self->tiles->len; i ++)
    {
      tile = self->tiles->pdata[i];

      if (shumate_tile_get_x (tile) == x
          && shumate_tile_get_y (tile) == y
          && shumate_tile_get_zoom_level (tile) == zoom_level)
          render (self, tile, bytes, zoom_level);
    }
}

/**
 * shumate_style_error_quark:
 *
 * Returns: a #GQuark
 */
G_DEFINE_QUARK (shumate-style-error-quark, shumate_style_error);
