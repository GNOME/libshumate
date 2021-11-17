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

G_DEFINE_TYPE (ShumateDataSource, shumate_data_source, G_TYPE_OBJECT)


enum
{
  RECEIVED_DATA,
  LAST_SIGNAL
};
static guint signals[LAST_SIGNAL] = { 0, };


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

  klass->get_tile_data_async = NULL;
  klass->get_tile_data_finish = shumate_data_source_real_get_tile_data_finish;

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
 * data from the network. [signal@received-data] is emitted each time
 * data is received, then @callback is called after the last update.
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
 * Returns: (transfer full)(nullable): The requested data, or %NULL if an error occurred
 */
GBytes *
shumate_data_source_get_tile_data_finish (ShumateDataSource  *self,
                                          GAsyncResult       *result,
                                          GError            **error)
{
  g_return_val_if_fail (SHUMATE_IS_DATA_SOURCE (self), FALSE);

  return SHUMATE_DATA_SOURCE_GET_CLASS (self)->get_tile_data_finish (self, result, error);
}
