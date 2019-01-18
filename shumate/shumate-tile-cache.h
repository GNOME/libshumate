/*
 * Copyright (C) 2010-2013 Jiri Techet <techet@gmail.com>
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

#include <shumate/shumate-defines.h>
#include <shumate/shumate-map-source.h>

G_BEGIN_DECLS

#define SHUMATE_TYPE_TILE_CACHE shumate_tile_cache_get_type ()

#define SHUMATE_TILE_CACHE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SHUMATE_TYPE_TILE_CACHE, ShumateTileCache))

#define SHUMATE_TILE_CACHE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SHUMATE_TYPE_TILE_CACHE, ShumateTileCacheClass))

#define SHUMATE_IS_TILE_CACHE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SHUMATE_TYPE_TILE_CACHE))

#define SHUMATE_IS_TILE_CACHE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SHUMATE_TYPE_TILE_CACHE))

#define SHUMATE_TILE_CACHE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), SHUMATE_TYPE_TILE_CACHE, ShumateTileCacheClass))

typedef struct _ShumateTileCachePrivate ShumateTileCachePrivate;

typedef struct _ShumateTileCache ShumateTileCache;
typedef struct _ShumateTileCacheClass ShumateTileCacheClass;

/**
 * ShumateTileCache:
 *
 * The #ShumateTileCache structure contains only private data
 * and should be accessed using the provided API
 *
 * Since: 0.6
 */
struct _ShumateTileCache
{
  ShumateMapSource parent_instance;

  ShumateTileCachePrivate *priv;
};

struct _ShumateTileCacheClass
{
  ShumateMapSourceClass parent_class;

  void (*store_tile)(ShumateTileCache *tile_cache,
      ShumateTile *tile,
      const gchar *contents,
      gsize size);
  void (*refresh_tile_time)(ShumateTileCache *tile_cache,
      ShumateTile *tile);
  void (*on_tile_filled)(ShumateTileCache *tile_cache,
      ShumateTile *tile);
};

GType shumate_tile_cache_get_type (void);

void shumate_tile_cache_store_tile (ShumateTileCache *tile_cache,
    ShumateTile *tile,
    const gchar *contents,
    gsize size);
void shumate_tile_cache_refresh_tile_time (ShumateTileCache *tile_cache,
    ShumateTile *tile);
void shumate_tile_cache_on_tile_filled (ShumateTileCache *tile_cache,
    ShumateTile *tile);

G_END_DECLS

#endif /* _SHUMATE_TILE_CACHE_H_ */
