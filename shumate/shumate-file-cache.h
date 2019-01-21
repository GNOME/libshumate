/*
 * Copyright (C) 2009 Pierre-Luc Beaudoin <pierre-luc@pierlux.com>
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

#ifndef _SHUMATE_FILE_CACHE_H_
#define _SHUMATE_FILE_CACHE_H_

#include <glib-object.h>
#include <shumate/shumate-tile-cache.h>

G_BEGIN_DECLS

#define SHUMATE_TYPE_FILE_CACHE shumate_file_cache_get_type ()

#define SHUMATE_FILE_CACHE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SHUMATE_TYPE_FILE_CACHE, ShumateFileCache))

#define SHUMATE_FILE_CACHE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SHUMATE_TYPE_FILE_CACHE, ShumateFileCacheClass))

#define SHUMATE_IS_FILE_CACHE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SHUMATE_TYPE_FILE_CACHE))

#define SHUMATE_IS_FILE_CACHE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SHUMATE_TYPE_FILE_CACHE))

#define SHUMATE_FILE_CACHE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), SHUMATE_TYPE_FILE_CACHE, ShumateFileCacheClass))

typedef struct _ShumateFileCachePrivate ShumateFileCachePrivate;

typedef struct _ShumateFileCache ShumateFileCache;
typedef struct _ShumateFileCacheClass ShumateFileCacheClass;

/**
 * ShumateFileCache:
 *
 * The #ShumateFileCache structure contains only private data
 * and should be accessed using the provided API
 *
 * Since: 0.6
 */
struct _ShumateFileCache
{
  ShumateTileCache parent_instance;

  ShumateFileCachePrivate *priv;
};

struct _ShumateFileCacheClass
{
  ShumateTileCacheClass parent_class;
};

GType shumate_file_cache_get_type (void);

ShumateFileCache *shumate_file_cache_new_full (guint size_limit,
    const gchar *cache_dir,
    ShumateRenderer *renderer);

guint shumate_file_cache_get_size_limit (ShumateFileCache *file_cache);
void shumate_file_cache_set_size_limit (ShumateFileCache *file_cache,
    guint size_limit);

const gchar *shumate_file_cache_get_cache_dir (ShumateFileCache *file_cache);

void shumate_file_cache_purge (ShumateFileCache *file_cache);
void shumate_file_cache_purge_on_idle (ShumateFileCache *file_cache);

G_END_DECLS

#endif /* _SHUMATE_FILE_CACHE_H_ */
