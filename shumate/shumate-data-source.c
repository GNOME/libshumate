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
shumate_data_source_class_init (ShumateDataSourceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = shumate_data_source_get_property;
  object_class->set_property = shumate_data_source_set_property;

  klass->get_tile_data_async = NULL;
  klass->get_tile_data_finish = shumate_data_source_real_get_tile_data_finish;

  /**
   * ShumateMapSource:min-zoom-level:
   *
   * The minimum zoom level
   */
  properties[PROP_MIN_ZOOM_LEVEL] =
    g_param_spec_uint ("min-zoom-level",
                       "Minimum Zoom Level",
                       "The minimum zoom level",
                       0, 30, 0,
                       G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * ShumateMapSource:max-zoom-level:
   *
   * The maximum zoom level
   */
  properties[PROP_MAX_ZOOM_LEVEL] =
    g_param_spec_uint ("max-zoom-level",
                       "Maximum Zoom Level",
                       "The maximum zoom level",
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
   */
  signals[RECEIVED_DATA] =
    g_signal_new ("received-data",
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
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
 * shumate_data_source_get_min_zoom_level:
 * @self: a [class@DataSource]
 *
 * Gets the data source's minimum zoom level.
 *
 * Returns: the minimum zoom level this data source supports
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
 */
void
shumate_data_source_set_min_zoom_level (ShumateDataSource *self,
                                        guint              zoom_level)
{
  ShumateDataSourcePrivate *priv = shumate_data_source_get_instance_private (self);

  g_return_if_fail (SHUMATE_IS_DATA_SOURCE (self));
  g_return_if_fail (zoom_level >= 0 && zoom_level <= 30);

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
 */
void
shumate_data_source_set_max_zoom_level (ShumateDataSource *self,
                                        guint              zoom_level)
{
  ShumateDataSourcePrivate *priv = shumate_data_source_get_instance_private (self);

  g_return_if_fail (SHUMATE_IS_DATA_SOURCE (self));
  g_return_if_fail (zoom_level >= 0 && zoom_level <= 30);

  if (priv->max_zoom_level != zoom_level)
    {
      priv->max_zoom_level = zoom_level;

      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_MAX_ZOOM_LEVEL]);
    }
}

