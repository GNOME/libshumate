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

#if !defined (__SHUMATE_SHUMATE_H_INSIDE__) && !defined (SHUMATE_COMPILATION)
#error "Only <shumate/shumate.h> can be included directly."
#endif

#ifndef _SHUMATE_TILE_CACHE_H_
#define _SHUMATE_TILE_CACHE_H_

#include <shumate/shumate-map-source.h>

G_BEGIN_DECLS

#define SHUMATE_TYPE_TILE_CACHE shumate_tile_cache_get_type ()
G_DECLARE_DERIVABLE_TYPE (ShumateTileCache, shumate_tile_cache, SHUMATE, TILE_CACHE, ShumateMapSource)

/**
 * ShumateTileCache:
 *
 * The #ShumateTileCache structure contains only private data
 * and should be accessed using the provided API
 */

struct _ShumateTileCacheClass
{
  ShumateMapSourceClass parent_class;

  void (*store_tile)(ShumateTileCache *tile_cache,
      ShumateTile *tile,
      const char *contents,
      gsize size);
  void (*refresh_tile_time)(ShumateTileCache *tile_cache,
      ShumateTile *tile);
  void (*on_tile_filled)(ShumateTileCache *tile_cache,
      ShumateTile *tile);
};

void shumate_tile_cache_store_tile (ShumateTileCache *tile_cache,
    ShumateTile *tile,
    const char *contents,
    gsize size);
void shumate_tile_cache_refresh_tile_time (ShumateTileCache *tile_cache,
    ShumateTile *tile);
void shumate_tile_cache_on_tile_filled (ShumateTileCache *tile_cache,
    ShumateTile *tile);

G_END_DECLS

#endif /* _SHUMATE_TILE_CACHE_H_ */
