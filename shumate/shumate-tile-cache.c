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
 * SECTION:shumate-tile-cache
 * @short_description: A base class of tile caches
 *
 * This class defines properties and methods commons to all caches (that is, map
 * sources that permit storage and retrieval of tiles). Tiles are typically
 * stored by #ShumateTileSource objects.
 */

#include "shumate-tile-cache.h"

G_DEFINE_ABSTRACT_TYPE (ShumateTileCache, shumate_tile_cache, SHUMATE_TYPE_MAP_SOURCE)

static const char *get_id (ShumateMapSource * map_source);
static const char *get_name (ShumateMapSource *map_source);
static const char *get_license (ShumateMapSource *map_source);
static const char *get_license_uri (ShumateMapSource *map_source);
static guint get_min_zoom_level (ShumateMapSource *map_source);
static guint get_max_zoom_level (ShumateMapSource *map_source);
static guint get_tile_size (ShumateMapSource *map_source);
static ShumateMapProjection get_projection (ShumateMapSource *map_source);

static void
shumate_tile_cache_class_init (ShumateTileCacheClass *klass)
{
  ShumateMapSourceClass *map_source_class = SHUMATE_MAP_SOURCE_CLASS (klass);
  ShumateTileCacheClass *tile_cache_class = SHUMATE_TILE_CACHE_CLASS (klass);

  map_source_class->get_id = get_id;
  map_source_class->get_name = get_name;
  map_source_class->get_license = get_license;
  map_source_class->get_license_uri = get_license_uri;
  map_source_class->get_min_zoom_level = get_min_zoom_level;
  map_source_class->get_max_zoom_level = get_max_zoom_level;
  map_source_class->get_tile_size = get_tile_size;
  map_source_class->get_projection = get_projection;

  map_source_class->fill_tile = NULL;

  tile_cache_class->refresh_tile_time = NULL;
  tile_cache_class->on_tile_filled = NULL;
  tile_cache_class->store_tile = NULL;
}


static void
shumate_tile_cache_init (ShumateTileCache *tile_cache)
{
}


/**
 * shumate_tile_cache_store_tile:
 * @tile_cache: a #ShumateTileCache
 * @tile: a #ShumateTile
 * @contents: the tile contents that should be stored
 * @size: size of the contents in bytes
 *
 * Stores the tile including the metadata into the cache.
 */
void
shumate_tile_cache_store_tile (ShumateTileCache *tile_cache,
    ShumateTile *tile,
    const char *contents,
    gsize size)
{
  g_return_if_fail (SHUMATE_IS_TILE_CACHE (tile_cache));

  SHUMATE_TILE_CACHE_GET_CLASS (tile_cache)->store_tile (tile_cache, tile, contents, size);
}


/**
 * shumate_tile_cache_refresh_tile_time:
 * @tile_cache: a #ShumateTileCache
 * @tile: a #ShumateTile
 *
 * Refreshes the tile access time in the cache.
 */
void
shumate_tile_cache_refresh_tile_time (ShumateTileCache *tile_cache,
    ShumateTile *tile)
{
  g_return_if_fail (SHUMATE_IS_TILE_CACHE (tile_cache));

  SHUMATE_TILE_CACHE_GET_CLASS (tile_cache)->refresh_tile_time (tile_cache, tile);
}


/**
 * shumate_tile_cache_on_tile_filled:
 * @tile_cache: a #ShumateTileCache
 * @tile: a #ShumateTile
 *
 * When a cache fills a tile and the next source in the chain is a tile cache,
 * it should call this function on the next source. This way all the caches
 * preceding a tile source in the chain get informed that the tile was used and
 * can modify their metadata accordingly in the implementation of this function.
 * In addition, the call of this function should be chained so within the
 * implementation of this function it should be called on the next source
 * in the chain when next source is a tile cache.
 */
void
shumate_tile_cache_on_tile_filled (ShumateTileCache *tile_cache,
    ShumateTile *tile)
{
  g_return_if_fail (SHUMATE_IS_TILE_CACHE (tile_cache));

  SHUMATE_TILE_CACHE_GET_CLASS (tile_cache)->on_tile_filled (tile_cache, tile);
}


static const char *
get_id (ShumateMapSource *map_source)
{
  g_return_val_if_fail (SHUMATE_IS_TILE_CACHE (map_source), NULL);

  ShumateMapSource *next_source = shumate_map_source_get_next_source (map_source);

  g_return_val_if_fail (SHUMATE_IS_MAP_SOURCE (next_source), NULL);

  return shumate_map_source_get_id (next_source);
}


static const char *
get_name (ShumateMapSource *map_source)
{
  g_return_val_if_fail (SHUMATE_IS_TILE_CACHE (map_source), NULL);

  ShumateMapSource *next_source = shumate_map_source_get_next_source (map_source);

  g_return_val_if_fail (SHUMATE_IS_MAP_SOURCE (next_source), NULL);

  return shumate_map_source_get_name (next_source);
}


static const char *
get_license (ShumateMapSource *map_source)
{
  g_return_val_if_fail (SHUMATE_IS_TILE_CACHE (map_source), NULL);

  ShumateMapSource *next_source = shumate_map_source_get_next_source (map_source);

  g_return_val_if_fail (SHUMATE_IS_MAP_SOURCE (next_source), NULL);

  return shumate_map_source_get_license (next_source);
}


static const char *
get_license_uri (ShumateMapSource *map_source)
{
  g_return_val_if_fail (SHUMATE_IS_TILE_CACHE (map_source), NULL);

  ShumateMapSource *next_source = shumate_map_source_get_next_source (map_source);

  g_return_val_if_fail (SHUMATE_IS_MAP_SOURCE (next_source), NULL);

  return shumate_map_source_get_license_uri (next_source);
}


static guint
get_min_zoom_level (ShumateMapSource *map_source)
{
  g_return_val_if_fail (SHUMATE_IS_TILE_CACHE (map_source), 0);

  ShumateMapSource *next_source = shumate_map_source_get_next_source (map_source);

  g_return_val_if_fail (SHUMATE_IS_MAP_SOURCE (next_source), 0);

  return shumate_map_source_get_min_zoom_level (next_source);
}


static guint
get_max_zoom_level (ShumateMapSource *map_source)
{
  g_return_val_if_fail (SHUMATE_IS_TILE_CACHE (map_source), 0);

  ShumateMapSource *next_source = shumate_map_source_get_next_source (map_source);

  g_return_val_if_fail (SHUMATE_IS_MAP_SOURCE (next_source), 0);

  return shumate_map_source_get_max_zoom_level (next_source);
}


static guint
get_tile_size (ShumateMapSource *map_source)
{
  g_return_val_if_fail (SHUMATE_IS_TILE_CACHE (map_source), 0);

  ShumateMapSource *next_source = shumate_map_source_get_next_source (map_source);

  g_return_val_if_fail (SHUMATE_IS_MAP_SOURCE (next_source), 0);

  return shumate_map_source_get_tile_size (next_source);
}


static ShumateMapProjection
get_projection (ShumateMapSource *map_source)
{
  g_return_val_if_fail (SHUMATE_IS_TILE_CACHE (map_source), SHUMATE_MAP_PROJECTION_MERCATOR);

  ShumateMapSource *next_source = shumate_map_source_get_next_source (map_source);

  g_return_val_if_fail (SHUMATE_IS_MAP_SOURCE (next_source), SHUMATE_MAP_PROJECTION_MERCATOR);

  return shumate_map_source_get_projection (next_source);
}
