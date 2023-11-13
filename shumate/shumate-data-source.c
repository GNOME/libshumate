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

typedef struct {
  int min_zoom_level;
  int max_zoom_level;
} ShumateDataSourcePrivate;

#include "shumate-data-source.h"
#include "shumate-data-source-request.h"

/**
 * ShumateDataSource:
 *
 * The base class used to retrieve tiles as [struct@GLib.Bytes].
 */

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (ShumateDataSource, shumate_data_source, G_TYPE_OBJECT)

enum {
  PROP_0,
  PROP_MIN_ZOOM_LEVEL,
  PROP_MAX_ZOOM_LEVEL,
  N_PROPS,
};

static GParamSpec *properties[N_PROPS];

enum
{
  RECEIVED_DATA,
  LAST_SIGNAL
};
static guint signals[LAST_SIGNAL] = { 0, };


static void
shumate_data_source_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  ShumateDataSource *data_source = SHUMATE_DATA_SOURCE (object);

  switch (prop_id)
    {
      case PROP_MIN_ZOOM_LEVEL:
        g_value_set_uint (value, shumate_data_source_get_min_zoom_level (data_source));
        break;

      case PROP_MAX_ZOOM_LEVEL:
        g_value_set_uint (value, shumate_data_source_get_max_zoom_level (data_source));
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
shumate_data_source_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  ShumateDataSource *data_source = SHUMATE_DATA_SOURCE (object);

  switch (prop_id)
    {
      case PROP_MIN_ZOOM_LEVEL:
        shumate_data_source_set_min_zoom_level (data_source,
                                                g_value_get_uint (value));
        break;

      case PROP_MAX_ZOOM_LEVEL:
        shumate_data_source_set_max_zoom_level (data_source,
                                                g_value_get_uint (value));
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static GBytes *
shumate_data_source_real_get_tile_data_finish (ShumateDataSource  *self,
                                               GAsyncResult       *result,
                                               GError            **error)
{
  g_return_val_if_fail (SHUMATE_IS_DATA_SOURCE (self), FALSE);
  g_return_val_if_fail (g_task_is_valid (result, self), FALSE);

  return g_task_propagate_pointer (G_TASK (result), error);
}

static void
on_get_tile_data_done (GObject      *source,
                       GAsyncResult *result,
                       gpointer      user_data)
{
  ShumateDataSource *self = (ShumateDataSource *)source;
  g_autoptr(ShumateDataSourceRequest) req = user_data;
  g_autoptr(GError) error = NULL;
  g_autoptr(GBytes) data = NULL;

  data = shumate_data_source_get_tile_data_finish (self, result, &error);

  if (data)
    shumate_data_source_request_emit_data (req, data, TRUE);
  else
    shumate_data_source_request_emit_error (req, error);
}

static ShumateDataSourceRequest *
shumate_data_source_real_start_request (ShumateDataSource *self,
                                        int                x,
                                        int                y,
                                        int                zoom_level,
                                        GCancellable      *cancellable)
{
  ShumateDataSourceRequest *req = shumate_data_source_request_new (x, y, zoom_level);
  shumate_data_source_get_tile_data_async (self,
                                           x, y, zoom_level,
                                           cancellable,
                                           on_get_tile_data_done,
                                           g_object_ref (req));
  return req;
}

static void
shumate_data_source_class_init (ShumateDataSourceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = shumate_data_source_get_property;
  object_class->set_property = shumate_data_source_set_property;

  klass->get_tile_data_async = NULL;
  klass->get_tile_data_finish = shumate_data_source_real_get_tile_data_finish;
  klass->start_request = shumate_data_source_real_start_request;

  /**
   * ShumateDataSource:min-zoom-level:
   *
   * The minimum zoom level
   *
   * Since: 1.1
   */
  properties[PROP_MIN_ZOOM_LEVEL] =
    g_param_spec_uint ("min-zoom-level",
                       "min-zoom-level",
                       "min-zoom-level",
                       0, 30, 0,
                       G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * ShumateDataSource:max-zoom-level:
   *
   * The maximum zoom level
   *
   * Since: 1.1
   */
  properties[PROP_MAX_ZOOM_LEVEL] =
    g_param_spec_uint ("max-zoom-level",
                       "max-zoom-level",
                       "max-zoom-level",
                       0, 30, 30,
                       G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class,
                                     N_PROPS,
                                     properties);

  /**
   * ShumateDataSource::received-data:
   * @self: the [class@DataSource] emitting the signal
   * @x: the X coordinate of the tile
   * @y: the Y coordinate of the tile
   * @zoom_level: the zoom level of the tile
   * @bytes: the received data
   *
   * Emitted when data is received for any tile. This includes any intermediate
   * steps, such as data from the file cache, as well as the final result.
   *
   * Deprecated: 1.1: Use [method@DataSource.start_request] and connect to the
   * notify signals of the resulting [class@DataSourceRequest].
   */
  signals[RECEIVED_DATA] =
    g_signal_new ("received-data",
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DEPRECATED,
                  0, NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  4, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT, G_TYPE_BYTES);
}

static void
shumate_data_source_init (ShumateDataSource *self)
{
}


/**
 * shumate_data_source_get_tile_data_async:
 * @self: a [class@DataSource]
 * @x: the X coordinate to fetch
 * @y: the Y coordinate to fetch
 * @zoom_level: the Z coordinate to fetch
 * @cancellable: a #GCancellable
 * @callback: a #GAsyncReadyCallback to execute upon completion
 * @user_data: closure data for @callback
 *
 * Gets the data for the tile at the given coordinates.
 *
 * Some data sources may return data multiple times. For example,
 * [class@TileDownloader] may return data from a cache, then return updated
 * data from the network. [signal@ShumateDataSource::received-data] is emitted
 * each time data is received, then @callback is called after the last update.
 */
void
shumate_data_source_get_tile_data_async (ShumateDataSource   *self,
                                         int                  x,
                                         int                  y,
                                         int                  zoom_level,
                                         GCancellable        *cancellable,
                                         GAsyncReadyCallback  callback,
                                         gpointer             user_data)
{
  g_return_if_fail (SHUMATE_IS_DATA_SOURCE (self));
  g_return_if_fail (!cancellable || G_IS_CANCELLABLE (cancellable));

  return SHUMATE_DATA_SOURCE_GET_CLASS (self)->get_tile_data_async (self, x, y, zoom_level, cancellable, callback, user_data);
}


/**
 * shumate_data_source_get_tile_data_finish:
 * @self: a [class@DataSource]
 * @result: a #GAsyncResult provided to callback
 * @error: a location for a #GError, or %NULL
 *
 * Gets the final result of a request started with
 * shumate_data_source_get_tile_data_async().
 *
 * Returns: (transfer full) (nullable): The requested data, or %NULL if an
 * error occurred
 */
GBytes *
shumate_data_source_get_tile_data_finish (ShumateDataSource  *self,
                                          GAsyncResult       *result,
                                          GError            **error)
{
  g_return_val_if_fail (SHUMATE_IS_DATA_SOURCE (self), FALSE);

  return SHUMATE_DATA_SOURCE_GET_CLASS (self)->get_tile_data_finish (self, result, error);
}


/**
 * shumate_data_source_start_request:
 * @self: a [class@DataSource]
 * @x: X coordinate to request
 * @y: Y coordinate to request
 * @zoom_level: zoom level to request
 * @cancellable: for cancelling the request
 *
 * Begins a request for a tile.
 *
 * Returns: (transfer full): a [class@DataSourceRequest] object for tracking
 * the request.
 *
 * Since: 1.1
 */
ShumateDataSourceRequest *
shumate_data_source_start_request (ShumateDataSource         *self,
                                   int                        x,
                                   int                        y,
                                   int                        zoom_level,
                                   GCancellable              *cancellable)
{
  g_return_val_if_fail (SHUMATE_IS_DATA_SOURCE (self), NULL);

  return SHUMATE_DATA_SOURCE_GET_CLASS (self)->start_request (self, x, y, zoom_level, cancellable);
}


/**
 * shumate_data_source_get_min_zoom_level:
 * @self: a [class@DataSource]
 *
 * Gets the data source's minimum zoom level.
 *
 * Returns: the minimum zoom level this data source supports
 *
 * Since: 1.1
 */
guint
shumate_data_source_get_min_zoom_level (ShumateDataSource *self)
{
  ShumateDataSourcePrivate *priv = shumate_data_source_get_instance_private (self);

  g_return_val_if_fail (SHUMATE_IS_DATA_SOURCE (self), 0);

  return priv->min_zoom_level;
}

/**
 * shumate_data_source_set_min_zoom_level:
 * @self: a [class@DataSource]
 * @zoom_level: the minimum zoom level
 *
 * Sets the data source's minimum zoom level.
 *
 * Since: 1.1
 */
void
shumate_data_source_set_min_zoom_level (ShumateDataSource *self,
                                        guint              zoom_level)
{
  ShumateDataSourcePrivate *priv = shumate_data_source_get_instance_private (self);

  g_return_if_fail (SHUMATE_IS_DATA_SOURCE (self));
  g_return_if_fail (zoom_level <= 30u);

  if (priv->min_zoom_level != zoom_level)
    {
      priv->min_zoom_level = zoom_level;

      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_MIN_ZOOM_LEVEL]);
    }
}

/**
 * shumate_data_source_get_max_zoom_level:
 * @self: a [class@DataSource]
 *
 * Gets the data source's maximum zoom level.
 *
 * Returns: the maximum zoom level this data source supports
 *
 * Since: 1.1
 */
guint
shumate_data_source_get_max_zoom_level (ShumateDataSource *self)
{
  ShumateDataSourcePrivate *priv = shumate_data_source_get_instance_private (self);

  g_return_val_if_fail (SHUMATE_IS_DATA_SOURCE (self), 0);

  return priv->max_zoom_level;
}

/**
 * shumate_data_source_set_max_zoom_level:
 * @self: a [class@DataSource]
 * @zoom_level: the maximum zoom level
 *
 * Sets the data source's maximum zoom level.
 *
 * Since: 1.1
 */
void
shumate_data_source_set_max_zoom_level (ShumateDataSource *self,
                                        guint              zoom_level)
{
  ShumateDataSourcePrivate *priv = shumate_data_source_get_instance_private (self);

  g_return_if_fail (SHUMATE_IS_DATA_SOURCE (self));
  g_return_if_fail (zoom_level <= 30u);

  if (priv->max_zoom_level != zoom_level)
    {
      priv->max_zoom_level = zoom_level;

      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_MAX_ZOOM_LEVEL]);
    }
}

