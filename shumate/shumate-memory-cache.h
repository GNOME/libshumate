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

#ifndef _SHUMATE_MEMORY_CACHE_H_
#define _SHUMATE_MEMORY_CACHE_H_

#include <glib-object.h>
#include <shumate/shumate-tile-cache.h>

G_BEGIN_DECLS

#define SHUMATE_TYPE_MEMORY_CACHE shumate_memory_cache_get_type ()

#define SHUMATE_MEMORY_CACHE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SHUMATE_TYPE_MEMORY_CACHE, ShumateMemoryCache))

#define SHUMATE_MEMORY_CACHE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SHUMATE_TYPE_MEMORY_CACHE, ShumateMemoryCacheClass))

#define SHUMATE_IS_MEMORY_CACHE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SHUMATE_TYPE_MEMORY_CACHE))

#define SHUMATE_IS_MEMORY_CACHE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SHUMATE_TYPE_MEMORY_CACHE))

#define SHUMATE_MEMORY_CACHE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), SHUMATE_TYPE_MEMORY_CACHE, ShumateMemoryCacheClass))

typedef struct _ShumateMemoryCachePrivate ShumateMemoryCachePrivate;

typedef struct _ShumateMemoryCache ShumateMemoryCache;
typedef struct _ShumateMemoryCacheClass ShumateMemoryCacheClass;

/**
 * ShumateMemoryCache:
 *
 * The #ShumateMemoryCache structure contains only private data
 * and should be accessed using the provided API
 *
 * Since: 0.8
 */
struct _ShumateMemoryCache
{
  ShumateTileCache parent_instance;

  ShumateMemoryCachePrivate *priv;
};

struct _ShumateMemoryCacheClass
{
  ShumateTileCacheClass parent_class;
};

GType shumate_memory_cache_get_type (void);

ShumateMemoryCache *shumate_memory_cache_new_full (guint size_limit,
    ShumateRenderer *renderer);

guint shumate_memory_cache_get_size_limit (ShumateMemoryCache *memory_cache);
void shumate_memory_cache_set_size_limit (ShumateMemoryCache *memory_cache,
    guint size_limit);

void shumate_memory_cache_clean (ShumateMemoryCache *memory_cache);

G_END_DECLS

#endif /* _SHUMATE_MEMORY_CACHE_H_ */
