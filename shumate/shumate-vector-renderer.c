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

#ifdef SHUMATE_HAS_VECTOR_RENDERER
#include <json-glib/json-glib.h>
#include <cairo/cairo.h>

#include "vector/shumate-vector-render-scope-private.h"
#include "vector/shumate-vector-utils-private.h"
#include "vector/shumate-vector-layer-private.h"
#endif

struct _ShumateVectorRenderer
{
  ShumateMapSource parent_instance;

  ShumateDataSource *data_source;
  GPtrArray *tiles;

  char *style_json;

  GPtrArray *layers;
};

static void shumate_vector_renderer_initable_iface_init (GInitableIface *iface);

G_DEFINE_TYPE_WITH_CODE (ShumateVectorRenderer, shumate_vector_renderer, SHUMATE_TYPE_MAP_SOURCE,
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, shumate_vector_renderer_initable_iface_init))

enum {
  PROP_0,
  PROP_DATA_SOURCE,
  PROP_STYLE_JSON,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];


/**
 * shumate_vector_renderer_new:
 * @data_source: a [class@DataSource] to provide tile image data
 * @style_json: a vector style
 * @error: return location for a #GError, or %NULL
 *
 * Creates a new [class@VectorRenderer] to render vector tiles from @data_source.
 *
 * Returns: (transfer full): a newly constructed [class@VectorRenderer]
 */
ShumateVectorRenderer *
shumate_vector_renderer_new (ShumateDataSource  *data_source,
                             const char         *style_json,
                             GError            **error)
{
  g_return_val_if_fail (SHUMATE_IS_DATA_SOURCE (data_source), NULL);
  return g_initable_new (SHUMATE_TYPE_VECTOR_RENDERER, NULL, error,
                         "data-source", data_source,
                         "style-json", style_json,
                         NULL);
}


/**
 * shumate_vector_renderer_new_from_url:
 * @url_template: a URL template to fetch tiles from
 * @style_json: a vector style
 * @error: return location for a #GError, or %NULL
 *
 * Creates a new [class@VectorRenderer] that fetches tiles from the given URL
 * using a [class@TileDownloader] data source.
 *
 * Equivalent to:
 *
 * ```c
 * g_autoptr(ShumateTileDownloader) source = shumate_tile_downloader_new (url_template);
 * ShumateVectorRenderer *renderer = shumate_vector_renderer_new (source);
 * ```
 *
 * Returns: (transfer full): a newly constructed [class@VectorRenderer]
 */
ShumateVectorRenderer *
shumate_vector_renderer_new_from_url (const char  *url_template,
                                      const char  *style_json,
                                      GError     **error)
{
  g_autoptr(ShumateDataSource) data_source = NULL;

  g_return_val_if_fail (url_template != NULL, NULL);

  data_source = SHUMATE_DATA_SOURCE (shumate_tile_downloader_new (url_template));
  return shumate_vector_renderer_new (data_source, style_json, error);
}


/**
 * shumate_vector_renderer_new_full:
 * @id: the map source's id
 * @name: the map source's name
 * @license: the map source's license
 * @license_uri: the map source's license URI
 * @min_zoom: the map source's minimum zoom level
 * @max_zoom: the map source's maximum zoom level
 * @tile_size: the map source's tile size (in pixels)
 * @projection: the map source's projection
 * @data_source: a [class@DataSource] to provide tile image data
 * @error: (nullable): a return location for a #GError, or %NULL
 *
 * Creates a new [class@VectorRenderer] to render vector tiles from @data_source.
 *
 * Returns: a newly constructed [class@VectorRenderer] object
 */
ShumateVectorRenderer *
shumate_vector_renderer_new_full (const char            *id,
                                  const char            *name,
                                  const char            *license,
                                  const char            *license_uri,
                                  guint                  min_zoom,
                                  guint                  max_zoom,
                                  guint                  tile_size,
                                  ShumateMapProjection   projection,
                                  ShumateDataSource     *data_source,
                                  const char            *style_json,
                                  GError               **error)
{
  g_return_val_if_fail (SHUMATE_IS_DATA_SOURCE (data_source), NULL);

  return g_initable_new (SHUMATE_TYPE_VECTOR_RENDERER, NULL, error,
                         "id", id,
                         "name", name,
                         "license", license,
                         "license-uri", license_uri,
                         "min-zoom-level", min_zoom,
                         "max-zoom-level", max_zoom,
                         "tile-size", tile_size,
                         "projection", projection,
                         "data-source", data_source,
                         "style-json", style_json,
                         NULL);
}


/**
 * shumate_vector_renderer_new_full_from_url:
 * @id: the map source's id
 * @name: the map source's name
 * @license: the map source's license
 * @license_uri: the map source's license URI
 * @min_zoom: the map source's minimum zoom level
 * @max_zoom: the map source's maximum zoom level
 * @tile_size: the map source's tile size (in pixels)
 * @projection: the map source's projection
 * @url_template: a template for the URL to fetch tiles from
 * @error: (nullable): a return location for a #GError, or %NULL
 *
 * Creates a new [class@VectorRenderer] that fetches tiles from the given URL
 * using a [class@TileDownloader] data source.
 *
 * Returns: a newly constructed [class@VectorRenderer] object
 */
ShumateVectorRenderer *
shumate_vector_renderer_new_full_from_url (const char            *id,
                                           const char            *name,
                                           const char            *license,
                                           const char            *license_uri,
                                           guint                  min_zoom,
                                           guint                  max_zoom,
                                           guint                  tile_size,
                                           ShumateMapProjection   projection,
                                           const char            *url_template,
                                           const char            *style_json,
                                           GError               **error)
{
  g_autoptr(ShumateTileDownloader) data_source = NULL;

  g_return_val_if_fail (url_template != NULL, NULL);

  data_source = shumate_tile_downloader_new (url_template);

  return g_initable_new (SHUMATE_TYPE_VECTOR_RENDERER, NULL, error,
                         "id", id,
                         "name", name,
                         "license", license,
                         "license-uri", license_uri,
                         "min-zoom-level", min_zoom,
                         "max-zoom-level", max_zoom,
                         "tile-size", tile_size,
                         "projection", projection,
                         "data-source", data_source,
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
shumate_vector_renderer_constructed (GObject *object)
{
  ShumateVectorRenderer *self = SHUMATE_VECTOR_RENDERER (object);

  g_signal_connect_object (self->data_source, "received-data", (GCallback)on_data_source_received_data, self, G_CONNECT_SWAPPED);
}


static void
shumate_vector_renderer_finalize (GObject *object)
{
  ShumateVectorRenderer *self = (ShumateVectorRenderer *)object;

  g_clear_pointer (&self->layers, g_ptr_array_unref);
  g_clear_pointer (&self->style_json, g_free);
  g_clear_object (&self->data_source);
  g_clear_pointer (&self->tiles, g_ptr_array_unref);

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
    case PROP_DATA_SOURCE:
      g_value_set_object (value, self->data_source);
      break;
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
    case PROP_DATA_SOURCE:
      g_set_object (&self->data_source, g_value_get_object (value));
      break;
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

static void
shumate_vector_renderer_class_init (ShumateVectorRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ShumateMapSourceClass *map_source_class = SHUMATE_MAP_SOURCE_CLASS (klass);

  object_class->constructed = shumate_vector_renderer_constructed;
  object_class->finalize = shumate_vector_renderer_finalize;
  object_class->get_property = shumate_vector_renderer_get_property;
  object_class->set_property = shumate_vector_renderer_set_property;

  map_source_class->fill_tile_async = shumate_vector_renderer_fill_tile_async;

  /**
   * ShumateVectorRenderer:data-source:
   *
   * The data source that provides image tiles to display. In most cases,
   * a [class@TileDownloader] is sufficient.
   */
  properties[PROP_DATA_SOURCE] =
    g_param_spec_object ("data-source",
                         "Data source",
                         "Data source",
                         SHUMATE_TYPE_DATA_SOURCE,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

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
  JsonObject *object;

  g_return_val_if_fail (SHUMATE_IS_VECTOR_RENDERER (self), FALSE);
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

typedef struct {
  ShumateVectorRenderer *self;
  ShumateTile *tile;
} FillTileData;

static void
fill_tile_data_free (FillTileData *data)
{
  g_clear_object (&data->self);
  g_clear_object (&data->tile);
  g_free (data);
}

static void
shumate_vector_renderer_fill_tile_async (ShumateMapSource    *map_source,
                                         ShumateTile         *tile,
                                         GCancellable        *cancellable,
                                         GAsyncReadyCallback  callback,
                                         gpointer             user_data)
{
  ShumateVectorRenderer *self = (ShumateVectorRenderer *)map_source;
  g_autoptr(GTask) task = NULL;
  FillTileData *data;

  g_return_if_fail (SHUMATE_IS_VECTOR_RENDERER (self));
  g_return_if_fail (SHUMATE_IS_TILE (tile));
  g_return_if_fail (!cancellable || G_IS_CANCELLABLE (cancellable));

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_source_tag (task, shumate_vector_renderer_fill_tile_async);

  data = g_new0 (FillTileData, 1);
  data->self = g_object_ref (self);
  data->tile = g_object_ref (tile);
  g_task_set_task_data (task, data, (GDestroyNotify)fill_tile_data_free);

  g_ptr_array_add (self->tiles, g_object_ref (tile));

  shumate_data_source_get_tile_data_async (self->data_source,
                                           shumate_tile_get_x (tile),
                                           shumate_tile_get_y (tile),
                                           shumate_tile_get_zoom_level (tile),
                                           cancellable,
                                           on_data_source_done,
                                           g_steal_pointer (&task));
}

static void
on_data_source_done (GObject *object, GAsyncResult *res, gpointer user_data)
{
  ShumateDataSource *data_source = SHUMATE_DATA_SOURCE (object);
  g_autoptr(GTask) task = G_TASK (user_data);
  FillTileData *data = g_task_get_task_data (task);
  GError *error = NULL;

  g_ptr_array_remove_fast (data->self->tiles, data->tile);

  if (!shumate_data_source_get_tile_data_finish (data_source, res, &error))
    g_task_return_error (task, error);
  else
    {
      g_task_return_boolean (task, TRUE);
      shumate_tile_set_state (data->tile, SHUMATE_STATE_DONE);
    }
}

static GdkTexture *
render (ShumateVectorRenderer *self, int texture_size, GBytes *tile_data, double zoom_level)
{
#ifdef SHUMATE_HAS_VECTOR_RENDERER
  ShumateVectorRenderScope scope;
  GdkTexture *texture;
  cairo_surface_t *surface;
  gconstpointer data;
  gsize len;

  g_return_val_if_fail (SHUMATE_IS_VECTOR_RENDERER (self), NULL);

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
#else
  g_return_val_if_reached (NULL);
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
        {
          g_autoptr(GdkTexture) texture = render (self, shumate_tile_get_size (tile), bytes, zoom_level);
          shumate_tile_set_texture (tile, texture);
        }
    }
}

/**
 * shumate_style_error_quark:
 *
 * Returns: a #GQuark
 */
G_DEFINE_QUARK (shumate-style-error-quark, shumate_style_error);
