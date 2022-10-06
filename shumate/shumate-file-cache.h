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
#include <gio/gio.h>

G_BEGIN_DECLS

/**
 * SHUMATE_FILE_CACHE_ERROR:
 *
 * Error domain for errors that may occur while storing or retrieving tiles
 * from a #ShumateFileCache. Errors in this domain will be from the
 * #ShumateFileCacheError enum.
 */
#define SHUMATE_FILE_CACHE_ERROR shumate_file_cache_error_quark ()
GQuark shumate_file_cache_error_quark (void);

/**
 * ShumateFileCacheError:
 * @SHUMATE_FILE_CACHE_ERROR_FAILED: An unspecified error occurred during the operation.
 *
 * Error codes in the #SHUMATE_FILE_CACHE_ERROR domain.
 */
typedef enum {
  SHUMATE_FILE_CACHE_ERROR_FAILED,
} ShumateFileCacheError;

#define SHUMATE_TYPE_FILE_CACHE shumate_file_cache_get_type ()
G_DECLARE_FINAL_TYPE (ShumateFileCache, shumate_file_cache, SHUMATE, FILE_CACHE, GObject)

ShumateFileCache *shumate_file_cache_new_full (guint size_limit,
    const char *cache_key,
    const char *cache_dir);

guint shumate_file_cache_get_size_limit (ShumateFileCache *self);
void shumate_file_cache_set_size_limit (ShumateFileCache *self,
    guint size_limit);

const char *shumate_file_cache_get_cache_dir (ShumateFileCache *self);
const char *shumate_file_cache_get_cache_key (ShumateFileCache *self);

void shumate_file_cache_purge_cache_async (ShumateFileCache *self,
                                           GCancellable *cancellable,
                                           GAsyncReadyCallback callback,
                                           gpointer user_data);
gboolean shumate_file_cache_purge_cache_finish (ShumateFileCache *self,
                                                GAsyncResult *result,
                                                GError **error);

void shumate_file_cache_get_tile_async (ShumateFileCache    *self,
                                        int                  x,
                                        int                  y,
                                        int                  zoom_level,
                                        GCancellable        *cancellable,
                                        GAsyncReadyCallback  callback,
                                        gpointer             user_data);
GBytes *shumate_file_cache_get_tile_finish (ShumateFileCache  *self,
                                            char             **etag,
                                            GDateTime        **modtime,
                                            GAsyncResult      *result,
                                            GError           **error);

void shumate_file_cache_store_tile_async (ShumateFileCache    *self,
                                          int                  x,
                                          int                  y,
                                          int                  zoom_level,
                                          GBytes              *bytes,
                                          const char          *etag,
                                          GCancellable        *cancellable,
                                          GAsyncReadyCallback  callback,
                                          gpointer             user_data);
gboolean shumate_file_cache_store_tile_finish (ShumateFileCache *self,
                                               GAsyncResult *result,
                                               GError **error);

void shumate_file_cache_mark_up_to_date (ShumateFileCache *self,
                                         int               x,
                                         int               y,
                                         int               zoom_level);

G_END_DECLS

#endif /* _SHUMATE_FILE_CACHE_H_ */
