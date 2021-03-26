/*
 * Copyright (C) 2010-2013 Jiri Techet <techet@gmail.com>
 * Copyright (C) 2019 Marcus Lundblad <ml@update.uu.se>
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/**
 * SECTION:shumate-error-tile-source
 * @short_description: A tile source that doesn't load map data from anywhere
 */

#include "shumate-error-tile-source.h"

#include "shumate-debug.h"
#include "shumate-enum-types.h"
#include "shumate-tile.h"
#include "shumate-network-tile-source.h"


G_DEFINE_TYPE (ShumateErrorTileSource, shumate_error_tile_source, SHUMATE_TYPE_TILE_SOURCE)


static void
fill_tile_async (ShumateMapSource *self,
                 ShumateTile *tile,
                 GCancellable *cancellable,
                 GAsyncReadyCallback callback,
                 gpointer user_data)
{
  g_return_if_fail (SHUMATE_IS_ERROR_TILE_SOURCE (self));
  g_return_if_fail (SHUMATE_IS_TILE (tile));

  g_task_report_new_error (self, callback, user_data, fill_tile_async,
                           SHUMATE_NETWORK_SOURCE_ERROR, SHUMATE_NETWORK_SOURCE_ERROR_FAILED,
                           "No tile found.");
}

static void
shumate_error_tile_source_class_init (ShumateErrorTileSourceClass *klass)
{
  ShumateMapSourceClass *map_source_class = SHUMATE_MAP_SOURCE_CLASS (klass);

  map_source_class->fill_tile_async = fill_tile_async;
}


static void
shumate_error_tile_source_init (ShumateErrorTileSource *self)
{
}


/**
 * shumate_error_tile_source_new_full:
 *
 * Constructor of #ShumateErrorTileSource.
 *
 * Returns: a constructed #ShumateErrorTileSource object
 */
ShumateErrorTileSource *
shumate_error_tile_source_new_full (void)
{
  return g_object_new (SHUMATE_TYPE_ERROR_TILE_SOURCE, NULL);
}
