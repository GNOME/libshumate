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


#include "shumate-data-source.h"
#include "shumate-raster-renderer.h"
#include "shumate-tile-downloader.h"


struct _ShumateRasterRenderer
{
  ShumateMapSource parent_instance;

  ShumateDataSource *data_source;
};

G_DEFINE_TYPE (ShumateRasterRenderer, shumate_raster_renderer, SHUMATE_TYPE_MAP_SOURCE)

enum {
  PROP_0,
  PROP_DATA_SOURCE,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];


/**
 * shumate_raster_renderer_new:
 * @data_source: a [class@DataSource] to provide tile image data
 *
 * Creates a new [class@RasterRenderer] that uses the given data source.
 *
 * Returns: (transfer full): a newly constructed [class@RasterRenderer]
 */
ShumateRasterRenderer *
shumate_raster_renderer_new (ShumateDataSource *data_source)
{
  g_return_val_if_fail (SHUMATE_IS_DATA_SOURCE (data_source), NULL);
  return g_object_new (SHUMATE_TYPE_RASTER_RENDERER,
                       "data-source", data_source,
                       NULL);
}

/**
 * shumate_raster_renderer_new_from_url:
 * @url_template: a URL template to fetch tiles from
 *
 * Creates a new [class@RasterRenderer] that fetches tiles from the given URL
 * using a [class@TileDownloader] data source.
 *
 * Equivalent to:
 *
 * ```c
 * g_autoptr(ShumateTileDownloader) source = shumate_tile_downloader_new (url_template);
 * ShumateRasterRenderer *renderer = shumate_raster_renderer_new (source);
 * ```
 *
 * Returns: (transfer full): a newly constructed [class@RasterRenderer]
 */
ShumateRasterRenderer *
shumate_raster_renderer_new_from_url (const char *url_template)
{
  g_autoptr(ShumateDataSource) data_source = NULL;

  g_return_val_if_fail (url_template != NULL, NULL);

  data_source = SHUMATE_DATA_SOURCE (shumate_tile_downloader_new (url_template));
  return shumate_raster_renderer_new (data_source);
}


/**
 * shumate_raster_renderer_new_full:
 * @id: the map source's id
 * @name: the map source's name
 * @license: the map source's license
 * @license_uri: the map source's license URI
 * @min_zoom: the map source's minimum zoom level
 * @max_zoom: the map source's maximum zoom level
 * @tile_size: the map source's tile size (in pixels)
 * @projection: the map source's projection
 * @data_source: a [class@DataSource] to provide tile image data
 *
 * Creates a new [class@RasterRenderer] with the given details and a data
 * source.
 *
 * Returns: a newly constructed [class@RasterRenderer]
 */
ShumateRasterRenderer *
shumate_raster_renderer_new_full (const char           *id,
                                  const char           *name,
                                  const char           *license,
                                  const char           *license_uri,
                                  guint                 min_zoom,
                                  guint                 max_zoom,
                                  guint                 tile_size,
                                  ShumateMapProjection  projection,
                                  ShumateDataSource    *data_source)
{
  g_return_val_if_fail (SHUMATE_IS_DATA_SOURCE (data_source), NULL);

  return g_object_new (SHUMATE_TYPE_RASTER_RENDERER,
                       "id", id,
                       "name", name,
                       "license", license,
                       "license-uri", license_uri,
                       "min-zoom-level", min_zoom,
                       "max-zoom-level", max_zoom,
                       "tile-size", tile_size,
                       "projection", projection,
                       "data-source", data_source,
                       NULL);
}


/**
 * shumate_raster_renderer_new_full_from_url:
 * @id: the map source's id
 * @name: the map source's name
 * @license: the map source's license
 * @license_uri: the map source's license URI
 * @min_zoom: the map source's minimum zoom level
 * @max_zoom: the map source's maximum zoom level
 * @tile_size: the map source's tile size (in pixels)
 * @projection: the map source's projection
 * @url_template: a URL template to fetch tiles from
 *
 * Creates a new [class@RasterRenderer] with the given details and a data
 * source.
 *
 * Returns: a newly constructed [class@RasterRenderer]
 */
ShumateRasterRenderer *
shumate_raster_renderer_new_full_from_url (const char           *id,
                                           const char           *name,
                                           const char           *license,
                                           const char           *license_uri,
                                           guint                 min_zoom,
                                           guint                 max_zoom,
                                           guint                 tile_size,
                                           ShumateMapProjection  projection,
                                           const char           *url_template)
{
  g_autoptr(ShumateTileDownloader) data_source = NULL;

  g_return_val_if_fail (url_template != NULL, NULL);

  data_source = shumate_tile_downloader_new (url_template);

  return g_object_new (SHUMATE_TYPE_RASTER_RENDERER,
                       "id", id,
                       "name", name,
                       "license", license,
                       "license-uri", license_uri,
                       "min-zoom-level", min_zoom,
                       "max-zoom-level", max_zoom,
                       "tile-size", tile_size,
                       "projection", projection,
                       "data-source", data_source,
                       NULL);
}

static void
shumate_raster_renderer_finalize (GObject *object)
{
  ShumateRasterRenderer *self = (ShumateRasterRenderer *)object;

  g_clear_object (&self->data_source);

  G_OBJECT_CLASS (shumate_raster_renderer_parent_class)->finalize (object);
}

static void
shumate_raster_renderer_get_property (GObject    *object,
                                      guint       prop_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  ShumateRasterRenderer *self = SHUMATE_RASTER_RENDERER (object);

  switch (prop_id)
    {
    case PROP_DATA_SOURCE:
      g_value_set_object (value, self->data_source);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
shumate_raster_renderer_set_property (GObject      *object,
                                      guint         prop_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  ShumateRasterRenderer *self = SHUMATE_RASTER_RENDERER (object);

  switch (prop_id)
    {
    case PROP_DATA_SOURCE:
      g_set_object (&self->data_source, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void shumate_raster_renderer_fill_tile_async (ShumateMapSource    *map_source,
                                                     ShumateTile         *tile,
                                                     GCancellable        *cancellable,
                                                     GAsyncReadyCallback  callback,
                                                     gpointer             user_data);

static gboolean shumate_raster_renderer_fill_tile_finish (ShumateMapSource  *map_source,
                                                          GAsyncResult      *result,
                                                          GError           **error);

static void
shumate_raster_renderer_class_init (ShumateRasterRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ShumateMapSourceClass *map_source_class = SHUMATE_MAP_SOURCE_CLASS (klass);

  object_class->finalize = shumate_raster_renderer_finalize;
  object_class->get_property = shumate_raster_renderer_get_property;
  object_class->set_property = shumate_raster_renderer_set_property;

  map_source_class->fill_tile_async = shumate_raster_renderer_fill_tile_async;
  map_source_class->fill_tile_finish = shumate_raster_renderer_fill_tile_finish;

  /**
   * ShumateRasterRenderer:data-source:
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

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
shumate_raster_renderer_init (ShumateRasterRenderer *self)
{
}


static void
on_request_notify (ShumateDataSourceRequest *req,
                   GParamSpec               *pspec,
                   gpointer                  user_data)
{
  GTask *task = user_data;
  GBytes *data;
  ShumateTile *tile = g_task_get_task_data (task);

  if ((data = shumate_data_source_request_get_data (req)))
    {
      g_autoptr(GError) error = NULL;
      g_autoptr(GdkPixbuf) pixbuf = NULL;
      g_autoptr(GInputStream) stream = NULL;
      g_autoptr(GdkTexture) texture = NULL;

      stream = g_memory_input_stream_new_from_bytes (data);
      pixbuf = gdk_pixbuf_new_from_stream (stream, NULL, &error);
      if (error)
        {
          g_warning ("Failed to create texture from tile data (%d, %d @ %d): %s",
                     shumate_tile_get_x (tile),
                     shumate_tile_get_y (tile),
                     shumate_tile_get_zoom_level (tile),
                     error->message);
          return;
        }

      texture = gdk_texture_new_for_pixbuf (pixbuf);
      shumate_tile_set_paintable (tile, GDK_PAINTABLE (texture));
    }
}

static void
on_request_notify_completed (ShumateDataSourceRequest *req,
                             GParamSpec               *pspec,
                             gpointer                  user_data)
{
  g_autoptr(GTask) task = user_data;
  GError *error;
  ShumateTile *tile = g_task_get_task_data (task);

  shumate_tile_set_state (tile, SHUMATE_STATE_DONE);

  if ((error = shumate_data_source_request_get_error (req)))
    g_task_return_error (task, g_error_copy (error));
  else
    g_task_return_boolean (task, TRUE);
}

static void
shumate_raster_renderer_fill_tile_async (ShumateMapSource    *map_source,
                                         ShumateTile         *tile,
                                         GCancellable        *cancellable,
                                         GAsyncReadyCallback  callback,
                                         gpointer             user_data)
{
  ShumateRasterRenderer *self = (ShumateRasterRenderer *)map_source;
  g_autoptr(GTask) task = NULL;
  g_autoptr(ShumateDataSourceRequest) req = NULL;

  g_return_if_fail (SHUMATE_IS_RASTER_RENDERER (self));
  g_return_if_fail (SHUMATE_IS_TILE (tile));
  g_return_if_fail (!cancellable || G_IS_CANCELLABLE (cancellable));

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_source_tag (task, shumate_raster_renderer_fill_tile_async);

  g_task_set_task_data (task, g_object_ref (tile), (GDestroyNotify)g_object_unref);

  req = shumate_data_source_start_request (self->data_source,
                                           shumate_tile_get_x (tile),
                                           shumate_tile_get_y (tile),
                                           shumate_tile_get_zoom_level (tile),
                                           cancellable);

  if (shumate_data_source_request_is_completed (req))
    {
      on_request_notify (req, NULL, task);
      on_request_notify_completed (req, NULL, g_steal_pointer (&task));
    }
  else
    {
      g_signal_connect_object (req, "notify::data", (GCallback)on_request_notify, task, 0);
      g_signal_connect_object (req, "notify::completed", (GCallback)on_request_notify_completed, g_object_ref (task), 0);
    }
}

static gboolean
shumate_raster_renderer_fill_tile_finish (ShumateMapSource  *map_source,
                                          GAsyncResult      *result,
                                          GError           **error)
{
  ShumateRasterRenderer *self = (ShumateRasterRenderer *)map_source;

  g_return_val_if_fail (SHUMATE_IS_RASTER_RENDERER (self), FALSE);
  g_return_val_if_fail (g_task_is_valid (result, self), FALSE);

  return g_task_propagate_boolean (G_TASK (result), error);
}
