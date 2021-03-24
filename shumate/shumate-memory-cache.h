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

#ifndef _SHUMATE_MEMORY_CACHE_H_
#define _SHUMATE_MEMORY_CACHE_H_

#include <glib-object.h>
#include <shumate/shumate-tile-cache.h>

G_BEGIN_DECLS

#define SHUMATE_TYPE_MEMORY_CACHE shumate_memory_cache_get_type ()
G_DECLARE_DERIVABLE_TYPE (ShumateMemoryCache, shumate_memory_cache, SHUMATE, MEMORY_CACHE, ShumateTileCache)

/**
 * ShumateMemoryCache:
 *
 * The #ShumateMemoryCache structure contains only private data
 * and should be accessed using the provided API
 */

struct _ShumateMemoryCacheClass
{
  ShumateTileCacheClass parent_class;
};

ShumateMemoryCache *shumate_memory_cache_new_full (guint size_limit);

guint shumate_memory_cache_get_size_limit (ShumateMemoryCache *memory_cache);
void shumate_memory_cache_set_size_limit (ShumateMemoryCache *memory_cache,
    guint size_limit);

void shumate_memory_cache_clean (ShumateMemoryCache *memory_cache);

gboolean shumate_memory_cache_try_fill_tile (ShumateMemoryCache *self,
                                             ShumateTile        *tile,
                                             const char         *source_id);
void shumate_memory_cache_store_texture (ShumateMemoryCache *self,
                                         ShumateTile        *tile,
                                         GdkTexture         *texture,
                                         const char         *source_id);

G_END_DECLS

#endif /* _SHUMATE_MEMORY_CACHE_H_ */
