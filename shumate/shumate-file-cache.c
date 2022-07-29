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

/**
 * ShumateFileCache:
 *
 * A cache that stores and retrieves tiles from the file system. It is mainly
 * used by [class@TileDownloader], but can also be used by custom data
 * sources.
 *
 * The cache will be filled up to a certain size limit. When this limit is
 * reached, the cache can be purged, and the tiles that are accessed least are
 * deleted.
 *
 * ## ETags
 *
 * The cache can optionally store an ETag string with each tile. This is
 * useful to avoid redownloading old tiles that haven't changed (for example,
 * using the HTTP If-None-Match header).
 */

#include "shumate-file-cache.h"

#include <sqlite3.h>
#include <errno.h>
#include <glib.h>
#include <gio/gio.h>
#include <string.h>
#include <stdlib.h>

enum
{
  PROP_0,
  PROP_SIZE_LIMIT,
  PROP_CACHE_DIR,
  PROP_CACHE_KEY,
  N_PROPS
};

static GParamSpec *properties[N_PROPS];

struct _ShumateFileCache
{
  GObject parent_instance;

  guint size_limit;
  char *cache_dir;
  char *cache_key;

  sqlite3 *db;
  sqlite3_stmt *stmt_select;
  sqlite3_stmt *stmt_update;

  int size_estimate;
  gboolean have_size_estimate;
  gboolean purge_in_progress;
};

G_DEFINE_TYPE (ShumateFileCache, shumate_file_cache, G_TYPE_OBJECT);


typedef char sqlite_str;
G_DEFINE_AUTOPTR_CLEANUP_FUNC (sqlite_str, sqlite3_free);
G_DEFINE_AUTOPTR_CLEANUP_FUNC (sqlite3_stmt, sqlite3_finalize);


static void finalize_sql (ShumateFileCache *file_cache);
static void init_cache (ShumateFileCache *file_cache);
static char *get_filename (ShumateFileCache *file_cache,
                           int               x,
                           int               y,
                           int               zoom_level);
static void delete_tile (ShumateFileCache *file_cache,
                         const char       *filename);
static gboolean create_cache_dir (const char *dir_name);

static void on_tile_filled (ShumateFileCache *self,
                            int               x,
                            int               y,
                            int               zoom_level);

static void
shumate_file_cache_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  ShumateFileCache *self = SHUMATE_FILE_CACHE (object);

  switch (property_id)
    {
    case PROP_SIZE_LIMIT:
      g_value_set_uint (value, shumate_file_cache_get_size_limit (self));
      break;

    case PROP_CACHE_DIR:
      g_value_set_string (value, shumate_file_cache_get_cache_dir (self));
      break;

    case PROP_CACHE_KEY:
      g_value_set_string (value, shumate_file_cache_get_cache_key (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}


static void
shumate_file_cache_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  ShumateFileCache *self = SHUMATE_FILE_CACHE (object);

  switch (property_id)
    {
    case PROP_SIZE_LIMIT:
      shumate_file_cache_set_size_limit (self, g_value_get_uint (value));
      break;

    case PROP_CACHE_DIR:
      g_free (self->cache_dir);
      self->cache_dir = g_strdup (g_value_get_string (value));
      break;

    case PROP_CACHE_KEY:
      g_free (self->cache_key);
      self->cache_key = g_strdup (g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
finalize_sql (ShumateFileCache *self)
{
  g_clear_pointer (&self->stmt_select, sqlite3_finalize);
  g_clear_pointer (&self->stmt_update, sqlite3_finalize);

  if (self->db)
    {
      int error = sqlite3_close (self->db);
      if (error != SQLITE_OK)
        g_debug ("Sqlite returned error %d when closing cache.db", error);
      self->db = NULL;
    }
}


static void
shumate_file_cache_finalize (GObject *object)
{
  ShumateFileCache *self = SHUMATE_FILE_CACHE (object);

  finalize_sql (self);

  g_clear_pointer (&self->cache_dir, g_free);
  g_clear_pointer (&self->cache_key, g_free);

  G_OBJECT_CLASS (shumate_file_cache_parent_class)->finalize (object);
}


static gboolean
create_cache_dir (const char *dir_name)
{
  /* If needed, create the cache's dirs */
  if (dir_name)
    {
      if (g_mkdir_with_parents (dir_name, 0700) == -1 && errno != EEXIST)
        {
          g_warning ("Unable to create the image cache path '%s': %s",
              dir_name, g_strerror (errno));
          return FALSE;
        }
    }
  return TRUE;
}


static void
init_cache (ShumateFileCache *self)
{
  char *filename = NULL;
  char *error_msg = NULL;
  gint error;

  g_return_if_fail (create_cache_dir (self->cache_dir));

  filename = g_build_filename (self->cache_dir,
        "cache.db", NULL);

  /* Make sure the database is opened in serialized mode (OPEN_FULLMUTEX)
   * because shumate_file_cache_purge_cache_async() runs in a separate thread.
   * See <https://sqlite.org/threadsafe.html> */
  error = sqlite3_open_v2 (filename, &self->db,
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, NULL);
  g_free (filename);

  if (error == SQLITE_ERROR)
    {
      g_debug ("Sqlite returned error %d when opening cache.db", error);
      return;
    }

  sqlite3_exec (self->db,
      "PRAGMA synchronous=OFF;"
      "PRAGMA auto_vacuum=INCREMENTAL;",
      NULL, NULL, &error_msg);
  if (error_msg != NULL)
    {
      g_debug ("Set PRAGMA: %s", error_msg);
      sqlite3_free (error_msg);
      return;
    }

  sqlite3_exec (self->db,
      "CREATE TABLE IF NOT EXISTS tiles ("
      "filename TEXT PRIMARY KEY, "
      "etag TEXT, "
      "popularity INT DEFAULT 1, "
      "size INT DEFAULT 0)",
      NULL, NULL, &error_msg);
  if (error_msg != NULL)
    {
      g_debug ("Creating table 'tiles' failed: %s", error_msg);
      sqlite3_free (error_msg);
      return;
    }

  error = sqlite3_prepare_v2 (self->db,
        "SELECT etag FROM tiles WHERE filename = ?", -1,
        &self->stmt_select, NULL);
  if (error != SQLITE_OK)
    {
      self->stmt_select = NULL;
      g_debug ("Failed to prepare the select Etag statement, error:%d: %s",
          error, sqlite3_errmsg (self->db));
      return;
    }

  error = sqlite3_prepare_v2 (self->db,
        "UPDATE tiles SET popularity = popularity + 1 WHERE filename = ?", -1,
        &self->stmt_update, NULL);
  if (error != SQLITE_OK)
    {
      self->stmt_update = NULL;
      g_debug ("Failed to prepare the update popularity statement, error: %s",
          sqlite3_errmsg (self->db));
      return;
    }

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_CACHE_DIR]);
}


static void
shumate_file_cache_constructed (GObject *object)
{
  ShumateFileCache *self = SHUMATE_FILE_CACHE (object);

  if (!self->cache_dir)
    {
      self->cache_dir = g_build_path (G_DIR_SEPARATOR_S,
            g_get_user_cache_dir (),
            "shumate", NULL);
    }

  init_cache (self);

  G_OBJECT_CLASS (shumate_file_cache_parent_class)->constructed (object);
}


static void
shumate_file_cache_class_init (ShumateFileCacheClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = shumate_file_cache_finalize;
  object_class->get_property = shumate_file_cache_get_property;
  object_class->set_property = shumate_file_cache_set_property;
  object_class->constructed = shumate_file_cache_constructed;

  /**
   * ShumateFileCache:size-limit:
   *
   * The cache size limit in bytes.
   *
   * Note: this new value will not be applied until you call shumate_file_cache_purge()
   */
  properties[PROP_SIZE_LIMIT] =
    g_param_spec_uint ("size-limit",
                       "Size Limit",
                       "The cache's size limit (Mb)",
                       1,
                       G_MAXINT,
                       100000000,
                       G_PARAM_CONSTRUCT | G_PARAM_READWRITE);

  /**
   * ShumateFileCache:cache-dir:
   *
   * The directory where the tile database is stored.
   */
  properties[PROP_CACHE_DIR] =
    g_param_spec_string ("cache-dir",
                         "Cache Directory",
                         "The directory of the cache",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);

  /**
   * ShumateFileCache:cache-key:
   *
   * The key used to store and retrieve tiles from the cache. Different keys
   * can be used to store multiple tilesets in the same cache directory.
   */
  properties[PROP_CACHE_KEY] =
    g_param_spec_string ("cache-key",
                         "Cache Key",
                         "The key used when storing and retrieving tiles",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);
}


static void
shumate_file_cache_init (ShumateFileCache *self)
{
  self->cache_dir = NULL;
  self->size_limit = 100000000;
  self->size_estimate = 0;
  self->have_size_estimate = FALSE;
  self->cache_dir = NULL;
  self->db = NULL;
  self->stmt_select = NULL;
  self->stmt_update = NULL;
}


/**
 * shumate_file_cache_new_full:
 * @size_limit: maximum size of the cache in bytes
 * @cache_key: an ID for the tileset to store/retrieve
 * @cache_dir: (allow-none): the directory where the cache is created. When cache_dir == NULL,
 * a cache in ~/.cache/shumate is used.
 *
 * Constructor of #ShumateFileCache.
 *
 * Returns: a constructed #ShumateFileCache
 */
ShumateFileCache *
shumate_file_cache_new_full (guint       size_limit,
                             const char *cache_key,
                             const char *cache_dir)
{
  ShumateFileCache *cache;

  g_return_val_if_fail (cache_key != NULL, NULL);

  cache = g_object_new (SHUMATE_TYPE_FILE_CACHE,
        "size-limit", size_limit,
        "cache-key", cache_key,
        "cache-dir", cache_dir,
        NULL);
  return cache;
}


/**
 * shumate_file_cache_get_size_limit:
 * @self: a #ShumateFileCache
 *
 * Gets the cache size limit in bytes.
 *
 * Returns: size limit
 */
guint
shumate_file_cache_get_size_limit (ShumateFileCache *self)
{
  g_return_val_if_fail (SHUMATE_IS_FILE_CACHE (self), 0);

  return self->size_limit;
}


/**
 * shumate_file_cache_get_cache_dir:
 * @self: a #ShumateFileCache
 *
 * Gets the directory where the cache database is stored.
 *
 * Returns: the directory
 */
const char *
shumate_file_cache_get_cache_dir (ShumateFileCache *self)
{
  g_return_val_if_fail (SHUMATE_IS_FILE_CACHE (self), NULL);

  return self->cache_dir;
}


/**
 * shumate_file_cache_get_cache_key:
 * @self: a #ShumateFileCache
 *
 * Gets the key used to store and retrieve tiles from the cache. Different keys
 * can be used to store multiple tilesets in the same cache directory.
 *
 * Returns: the cache key
 */
const char *
shumate_file_cache_get_cache_key (ShumateFileCache *self)
{
  g_return_val_if_fail (SHUMATE_IS_FILE_CACHE (self), NULL);

  return self->cache_key;
}


/**
 * shumate_file_cache_set_size_limit:
 * @self: a #ShumateFileCache
 * @size_limit: the cache limit in bytes
 *
 * Sets the cache size limit in bytes.
 */
void
shumate_file_cache_set_size_limit (ShumateFileCache *self,
                                   guint             size_limit)
{
  g_return_if_fail (SHUMATE_IS_FILE_CACHE (self));

  self->size_limit = size_limit;
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SIZE_LIMIT]);
}


static char *
get_filename (ShumateFileCache *self,
              int               x,
              int               y,
              int               zoom_level)
{
  const char *cache_key;

  g_return_val_if_fail (SHUMATE_IS_FILE_CACHE (self), NULL);
  g_return_val_if_fail (self->cache_dir, NULL);

  cache_key = shumate_file_cache_get_cache_key (self);

  char *filename = g_strdup_printf ("%s" G_DIR_SEPARATOR_S
                                    "%s" G_DIR_SEPARATOR_S
                                    "%d" G_DIR_SEPARATOR_S
                                    "%d" G_DIR_SEPARATOR_S "%d.png",
                                    self->cache_dir,
                                    cache_key,
                                    zoom_level,
                                    x,
                                    y);
  return filename;
}


static char *
db_get_etag (ShumateFileCache *self,
             int               x,
             int               y,
             int               zoom_level)
{
  int sql_rc = SQLITE_OK;
  g_autofree char *filename = get_filename (self, x, y, zoom_level);

  sqlite3_reset (self->stmt_select);
  sql_rc = sqlite3_bind_text (self->stmt_select, 1, filename, -1, SQLITE_STATIC);
  if (sql_rc == SQLITE_ERROR)
    {
      g_debug ("Failed to prepare the SQL query for finding the Etag of '%s', error: %s",
          filename, sqlite3_errmsg (self->db));
      return NULL;
    }

  sql_rc = sqlite3_step (self->stmt_select);
  if (sql_rc == SQLITE_ROW)
    {
      const char *etag = (const char *) sqlite3_column_text (self->stmt_select, 0);
      return g_strdup (etag);
    }
  else if (sql_rc == SQLITE_DONE)
    {
      g_debug ("'%s' doesn't have an etag",
          filename);
    }
  else if (sql_rc == SQLITE_ERROR)
    {
      g_debug ("Failed to finding the Etag of '%s', %d error: %s",
          filename, sql_rc, sqlite3_errmsg (self->db));
    }

  return NULL;
}


/**
 * shumate_file_cache_mark_up_to_date:
 * @self: a #ShumateFileCache
 * @x: the X coordinate of the tile
 * @y: the Y coordinate of the tile
 * @zoom_level: the zoom level of the tile
 *
 * Marks a tile in the cache as being up to date, without changing its data.
 *
 * For example, a network source might call this function when it gets an HTTP
 * 304 Not Modified response.
 */
void
shumate_file_cache_mark_up_to_date (ShumateFileCache *self,
                                    int               x,
                                    int               y,
                                    int               zoom_level)
{
  g_autofree char *filename = NULL;
  g_autoptr(GFile) file = NULL;
  g_autoptr(GFileInfo) info = NULL;

  g_return_if_fail (SHUMATE_IS_FILE_CACHE (self));

  filename = get_filename (self, x, y, zoom_level);
  file = g_file_new_for_path (filename);

  info = g_file_query_info (file, G_FILE_ATTRIBUTE_TIME_MODIFIED,
        G_FILE_QUERY_INFO_NONE, NULL, NULL);

  if (info)
    {
      g_autoptr(GDateTime) now = g_date_time_new_now_utc ();

      g_file_info_set_modification_date_time (info, now);
      g_file_set_attributes_from_info (file, info, G_FILE_QUERY_INFO_NONE, NULL, NULL);
    }
}


static void
on_tile_filled (ShumateFileCache *self,
                int               x,
                int               y,
                int               zoom_level)
{
  int sql_rc = SQLITE_OK;
  g_autofree char *filename = NULL;

  filename = get_filename (self, x, y, zoom_level);

  g_debug ("popularity of %s", filename);

  sqlite3_reset (self->stmt_update);
  sql_rc = sqlite3_bind_text (self->stmt_update, 1, filename, -1, SQLITE_STATIC);
  if (sql_rc != SQLITE_OK)
    {
      g_debug ("Failed to set values to the popularity query of '%s', error: %s",
          filename, sqlite3_errmsg (self->db));
      return;
    }

  sql_rc = sqlite3_step (self->stmt_update);
  if (sql_rc != SQLITE_DONE)
    {
      /* may not be present in this cache */
      return;
    }
}


static void
delete_tile (ShumateFileCache *self,
             const char       *filename)
{
  g_return_if_fail (SHUMATE_IS_FILE_CACHE (self));
  g_autoptr(sqlite_str) query = NULL;
  g_autoptr(sqlite_str) sql_error = NULL;
  g_autoptr(GError) gerror = NULL;
  g_autoptr(GFile) file = NULL;

  file = g_file_new_for_path (filename);
  if (!g_file_delete (file, NULL, &gerror))
    {
      g_debug ("Deleting tile from disk failed: %s", gerror->message);
    }

  query = sqlite3_mprintf ("DELETE FROM tiles WHERE filename = %Q", filename);
  sqlite3_exec (self->db, query, NULL, NULL, &sql_error);
  if (sql_error != NULL)
    {
      g_debug ("Deleting tile from db failed: %s", sql_error);
    }
}


static void
purge_cache (GTask        *task,
             gpointer      source_object,
             gpointer      _task_data,
             GCancellable *cancellable)
{
  ShumateFileCache *self = (ShumateFileCache *) source_object;

  char *query;
  g_autoptr(sqlite3_stmt) stmt = NULL;
  int rc = 0;
  guint original_size, current_size = 0;
  guint highest_popularity = 0;
  g_autoptr(sqlite_str) error = NULL;

  query = "SELECT SUM (size) FROM tiles";
  rc = sqlite3_prepare (self->db, query, strlen (query), &stmt, NULL);
  if (rc != SQLITE_OK)
    {
      g_warning ("Can't compute cache size %s", sqlite3_errmsg (self->db));
      g_task_return_boolean (task, FALSE);
      return;
    }

  rc = sqlite3_step (stmt);
  if (rc != SQLITE_ROW)
    {
      g_warning ("Failed to count the total cache consumption %s",
          sqlite3_errmsg (self->db));
      g_task_return_boolean (task, FALSE);
      return;
    }

  current_size = sqlite3_column_int (stmt, 0);
  original_size = current_size;
  if (current_size < self->size_limit)
    {
      g_debug ("Cache doesn't need to be purged at %d bytes", current_size);
      self->size_estimate = current_size;
      g_task_return_boolean (task, FALSE);
      return;
    }

  sqlite3_finalize (stmt);

  /* Ok, delete the less popular tiles until size_limit reached */
  query = "SELECT filename, size, popularity FROM tiles ORDER BY popularity";
  rc = sqlite3_prepare (self->db, query, strlen (query), &stmt, NULL);
  if (rc != SQLITE_OK)
    {
      g_warning ("Can't fetch tiles to delete: %s", sqlite3_errmsg (self->db));
    }

  rc = sqlite3_step (stmt);
  while (rc == SQLITE_ROW && current_size > self->size_limit)
    {
      const char *filename;
      guint size;

      filename = (const char *) sqlite3_column_text (stmt, 0);
      size = sqlite3_column_int (stmt, 1);
      highest_popularity = sqlite3_column_int (stmt, 2);
      g_debug ("Deleting %s of size %d", filename, size);

      delete_tile (self, filename);

      current_size -= size;

      rc = sqlite3_step (stmt);
    }

  g_debug ("Cache size is now %d bytes (reduced by %d bytes)", current_size, original_size - current_size);
  self->size_estimate = current_size;
  self->have_size_estimate = TRUE;

  query = sqlite3_mprintf ("UPDATE tiles SET popularity = popularity - %d",
        highest_popularity);
  sqlite3_exec (self->db, query, NULL, NULL, &error);
  if (error != NULL)
    {
      g_warning ("Updating popularity failed: %s", error);
      sqlite3_free (error);
    }
  sqlite3_free (query);

  sqlite3_exec (self->db, "PRAGMA incremental_vacuum;", NULL, NULL, &error);

  self->purge_in_progress = FALSE;
  g_task_return_boolean (task, TRUE);
}

/**
 * shumate_file_cache_purge_cache_async:
 * @self: a #ShumateFileCache
 * @cancellable: (nullable): a #GCancellable
 * @callback: a #GAsyncReadyCallback to execute upon completion
 * @user_data: closure data for @callback
 *
 * Removes less used tiles from the cache, if necessary, until it fits in
 * the size limit.
 */
void
shumate_file_cache_purge_cache_async (ShumateFileCache    *self,
                                      GCancellable        *cancellable,
                                      GAsyncReadyCallback  callback,
                                      gpointer             user_data)
{
  g_autoptr(GTask) task = NULL;

  g_return_if_fail (SHUMATE_IS_FILE_CACHE (self));
  g_return_if_fail (!cancellable || G_IS_CANCELLABLE (cancellable));

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_source_tag (task, shumate_file_cache_purge_cache_async);

  if (self->purge_in_progress)
    {
      g_task_return_boolean (task, FALSE);
      return;
    }

  self->purge_in_progress = TRUE;
  g_task_run_in_thread (task, purge_cache);
}

/**
 * shumate_file_cache_purge_cache_finish:
 * @self: a #ShumateFileCache
 * @result: a #GAsyncResult provided to callback
 * @error: a location for a #GError, or %NULL
 *
 * Gets the result of an async operation started using
 * shumate_file_cache_purge_cache_async().
 *
 * Returns: %TRUE if any tiles were removed, otherwise %FALSE
 */
gboolean
shumate_file_cache_purge_cache_finish (ShumateFileCache  *self,
                                       GAsyncResult      *result,
                                       GError           **error)
{
  g_return_val_if_fail (SHUMATE_IS_FILE_CACHE (self), FALSE);
  g_return_val_if_fail (g_task_is_valid (result, self), FALSE);

  return g_task_propagate_boolean (G_TASK (result), error);
}


typedef struct {
  char *etag;
  GDateTime *modtime;
} GetTileData;

static void
get_tile_data_free (GetTileData *data)
{
  g_clear_pointer (&data->etag, g_free);
  g_clear_pointer (&data->modtime, g_date_time_unref);
  g_free (data);
}

static void on_get_tile_file_loaded (GObject *source_object, GAsyncResult *res, gpointer user_data);


/**
 * shumate_file_cache_get_tile_async:
 * @self: a #ShumateFileCache
 * @x: the X coordinate of the tile
 * @y: the Y coordinate of the tile
 * @zoom_level: the zoom level of the tile
 * @cancellable: (nullable): a #GCancellable
 * @callback: a #GAsyncReadyCallback to execute upon completion
 * @user_data: closure data for @callback
 *
 * Gets tile data from the cache, if it is available.
 */
void
shumate_file_cache_get_tile_async (ShumateFileCache    *self,
                                   int                  x,
                                   int                  y,
                                   int                  zoom_level,
                                   GCancellable        *cancellable,
                                   GAsyncReadyCallback  callback,
                                   gpointer             user_data)
{
  g_autoptr(GTask) task = NULL;
  g_autoptr(GFile) file = NULL;
  g_autofree char *filename = NULL;
  g_autoptr(GFileInfo) info = NULL;
  g_autoptr(GError) error = NULL;
  GetTileData *task_data = NULL;

  g_return_if_fail (SHUMATE_IS_FILE_CACHE (self));
  g_return_if_fail (!cancellable || G_IS_CANCELLABLE (cancellable));

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_source_tag (task, shumate_file_cache_get_tile_async);

  task_data = g_new0 (GetTileData, 1);
  g_task_set_task_data (task, task_data, (GDestroyNotify) get_tile_data_free);

  filename = get_filename (self, x, y, zoom_level);
  file = g_file_new_for_path (filename);

  /* Retrieve modification time */
  info = g_file_query_info (file,
                            G_FILE_ATTRIBUTE_TIME_MODIFIED,
                            G_FILE_QUERY_INFO_NONE, cancellable, &error);
  if (error)
    {
      if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND))
        g_task_return_pointer (task, NULL, NULL);
      else
        g_task_return_error (task, g_error_copy (error));

      return;
    }

  task_data->modtime = g_file_info_get_modification_date_time (info);
  task_data->etag = db_get_etag (self, x, y, zoom_level);

  /* update tile popularity */
  on_tile_filled (self, x, y, zoom_level);

  g_file_load_bytes_async (file, cancellable, on_get_tile_file_loaded, g_object_ref (task));
}


static void
on_get_tile_file_loaded (GObject      *source_object,
                         GAsyncResult *res,
                         gpointer      user_data)
{
  g_autoptr(GTask) task = user_data;
  GFile *file = G_FILE (source_object);
  g_autoptr(GError) error = NULL;
  GBytes *bytes;

  bytes = g_file_load_bytes_finish (file, res, NULL, &error);

  if (error != NULL)
    {
      /* Return NULL but not an error if the file doesn't exist */
      if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND))
        g_task_return_pointer (task, NULL, NULL);
      else
        g_task_return_error (task, g_error_copy (error));

      return;
    }

  g_task_return_pointer (task, bytes, (GDestroyNotify) g_bytes_unref);
}


/**
 * shumate_file_cache_get_tile_finish:
 * @self: a #ShumateFileCache
 * @etag: (nullable) (out) (optional): a location for the data's ETag, or %NULL
 * @modtime: (nullable) (out) (optional): a location to return the tile's last modification time, or %NULL
 * @result: a #GAsyncResult provided to callback
 * @error: a location for a #GError, or %NULL
 *
 * Gets the tile data from a completed shumate_file_cache_get_tile_async()
 * operation.
 *
 * @modtime will be set to the time the tile was added to the cache, or the
 * latest time it was confirmed to be up to date.
 *
 * @etag will be set to the data's ETag, if present.
 *
 * Returns: a #GBytes containing the tile data, or %NULL if the tile was not in
 * the cache or an error occurred
 */
GBytes *
shumate_file_cache_get_tile_finish (ShumateFileCache  *self,
                                    char             **etag,
                                    GDateTime        **modtime,
                                    GAsyncResult      *result,
                                    GError           **error)
{
  GetTileData *data = g_task_get_task_data (G_TASK (result));

  g_return_val_if_fail (SHUMATE_IS_FILE_CACHE (self), NULL);
  g_return_val_if_fail (g_task_is_valid (result, self), NULL);

  if (etag)
    *etag = g_steal_pointer (&data->etag);
  if (modtime)
    *modtime = g_steal_pointer (&data->modtime);

  return g_task_propagate_pointer (G_TASK (result), error);
}


typedef struct {
  ShumateFileCache *self;
  char *etag;
  GBytes *bytes;
  char *filename;
} StoreTileData;

static void
store_tile_data_free (StoreTileData *data)
{
  g_clear_object (&data->self);
  g_clear_pointer (&data->etag, g_free);
  g_clear_pointer (&data->bytes, g_bytes_unref);
  g_clear_pointer (&data->filename, g_free);
  g_free (data);
}
G_DEFINE_AUTOPTR_CLEANUP_FUNC (StoreTileData, store_tile_data_free);

static void on_file_created (GObject *object, GAsyncResult *result, gpointer user_data);
static void on_file_written (GObject *object, GAsyncResult *result, gpointer user_data);

/**
 * shumate_file_cache_store_tile_async:
 * @self: a #ShumateFileCache
 * @x: the X coordinate of the tile
 * @y: the Y coordinate of the tile
 * @zoom_level: the zoom level of the tile
 * @bytes: a #GBytes
 * @etag: (nullable): an ETag string, or %NULL
 * @cancellable: (nullable): a #GCancellable
 * @callback: a #GAsyncReadyCallback to execute upon completion
 * @user_data: closure data for @callback
 *
 * Stores a tile in the cache.
 */
void
shumate_file_cache_store_tile_async (ShumateFileCache    *self,
                                     int                  x,
                                     int                  y,
                                     int                  zoom_level,
                                     GBytes              *bytes,
                                     const char          *etag,
                                     GCancellable        *cancellable,
                                     GAsyncReadyCallback  callback,
                                     gpointer             user_data)
{
  g_autoptr(GTask) task = NULL;
  g_autofree char *filename = NULL;
  g_autoptr(GFile) file = NULL;
  g_autofree char *path = NULL;
  StoreTileData *data;

  g_return_if_fail (SHUMATE_IS_FILE_CACHE (self));
  g_return_if_fail (bytes != NULL);
  g_return_if_fail (!cancellable || G_IS_CANCELLABLE (cancellable));

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_source_tag (task, shumate_file_cache_store_tile_async);

  filename = get_filename (self, x, y, zoom_level);
  file = g_file_new_for_path (filename);

  g_debug ("Update of tile (%d %d zoom %d)", x, y, zoom_level);

  /* If needed, create the cache's dirs */
  path = g_path_get_dirname (filename);
  if (g_mkdir_with_parents (path, 0700) == -1)
    {
      if (errno != EEXIST)
        {
          const char *error_string = g_strerror (errno);
          g_task_return_new_error (task, SHUMATE_FILE_CACHE_ERROR,
                                   SHUMATE_FILE_CACHE_ERROR_FAILED,
                                   "Failed to create cache directory %s: %s", path, error_string);
          return;
        }
    }

  data = g_new0 (StoreTileData, 1);
  data->self = g_object_ref (self);
  data->etag = g_strdup (etag);
  data->bytes = g_bytes_ref (bytes);
  data->filename = g_steal_pointer (&filename);
  g_task_set_task_data (task, data, (GDestroyNotify) store_tile_data_free);

  g_file_replace_async (file, NULL, FALSE, G_FILE_CREATE_PRIVATE, G_PRIORITY_DEFAULT, cancellable, on_file_created, g_object_ref (task));
}

static void
on_file_created (GObject      *object,
                 GAsyncResult *res,
                 gpointer      user_data)
{
  g_autoptr(GTask) task = user_data;
  StoreTileData *data = g_task_get_task_data (task);
  GCancellable *cancellable = g_task_get_cancellable (task);
  GError *error = NULL;
  g_autoptr(GFileOutputStream) ostream = NULL;
  gconstpointer contents;
  gsize size;

  ostream = g_file_create_finish (G_FILE (object), res, &error);
  if (error != NULL)
    {
      g_task_return_error (task, error);
      return;
    }

  contents = g_bytes_get_data (data->bytes, &size);

  g_output_stream_write_all_async (G_OUTPUT_STREAM (ostream),
                                   contents, size, G_PRIORITY_DEFAULT,
                                   cancellable, on_file_written, g_object_ref (task));
}

static void
on_file_written (GObject      *object,
                 GAsyncResult *res,
                 gpointer      user_data)
{
  g_autoptr(GTask) task = user_data;
  StoreTileData *data = g_task_get_task_data (task);
  g_autoptr(sqlite_str) query = NULL;
  g_autoptr(sqlite_str) sql_error = NULL;
  GError *error = NULL;
  guint tile_size = g_bytes_get_size (data->bytes);

  g_output_stream_write_all_finish (G_OUTPUT_STREAM (object), res, NULL, &error);
  if (error != NULL)
    {
      g_task_return_error (task, error);
      return;
    }

  query = sqlite3_mprintf ("REPLACE INTO tiles (filename, etag, size) VALUES (%Q, %Q, %d)",
                           data->filename, data->etag, tile_size);
  sqlite3_exec (data->self->db, query, NULL, NULL, &sql_error);
  if (sql_error != NULL)
    {
      g_task_return_new_error (task, SHUMATE_FILE_CACHE_ERROR, SHUMATE_FILE_CACHE_ERROR_FAILED,
                               "Failed to insert tile into SQLite database: %s", sql_error);
      return;
    }

  data->self->size_estimate += tile_size;
  if (!data->self->have_size_estimate || data->self->size_estimate > data->self->size_limit + 5000000)
    {
      /* automatically purge the cache if the size estimate is 5MB over
       * the limit, or if there is no estimate of the cache size yet */

      shumate_file_cache_purge_cache_async (data->self, NULL, NULL, NULL);
    }


  g_task_return_boolean (task, TRUE);
}


/**
 * shumate_file_cache_store_tile_finish:
 * @self: an #ShumateFileCache
 * @result: a #GAsyncResult provided to callback
 * @error: a location for a #GError, or %NULL
 *
 * Gets the success value of a completed shumate_file_cache_store_tile_async()
 * operation.
 *
 * Returns: %TRUE if the operation was successful, otherwise %FALSE
 */
gboolean
shumate_file_cache_store_tile_finish (ShumateFileCache  *self,
                                      GAsyncResult      *result,
                                      GError           **error)
{
  g_return_val_if_fail (SHUMATE_IS_FILE_CACHE (self), FALSE);
  g_return_val_if_fail (g_task_is_valid (result, self), FALSE);

  return g_task_propagate_boolean (G_TASK (result), error);
}

/**
 * shumate_file_cache_error_quark:
 *
 * Gets the #ShumateFileCache error quark.
 *
 * Returns: a #GQuark
 */
G_DEFINE_QUARK (shumate-file-cache-error-quark, shumate_file_cache_error);
