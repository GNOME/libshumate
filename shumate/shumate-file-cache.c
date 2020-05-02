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
 * SECTION:shumate-file-cache
 * @short_description: Stores and loads cached tiles from the file system
 *
 * #ShumateFileCache is a cache that stores and retrieves tiles from the
 * file system. Tiles most frequently loaded gain in "popularity". This popularity
 * is taken into account when purging the cache.
 */

#define DEBUG_FLAG SHUMATE_DEBUG_CACHE
#include "shumate-debug.h"

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
  PROP_CACHE_DIR
};

typedef struct
{
  guint size_limit;
  gchar *cache_dir;

  sqlite3 *db;
  sqlite3_stmt *stmt_select;
  sqlite3_stmt *stmt_update;
} ShumateFileCachePrivate;

G_DEFINE_TYPE_WITH_PRIVATE (ShumateFileCache, shumate_file_cache, SHUMATE_TYPE_TILE_CACHE);

static void finalize_sql (ShumateFileCache *file_cache);
static void init_cache (ShumateFileCache *file_cache);
static gchar *get_filename (ShumateFileCache *file_cache,
    ShumateTile *tile);
static gboolean tile_is_expired (ShumateFileCache *file_cache,
    ShumateTile *tile);
static void delete_tile (ShumateFileCache *file_cache,
    const gchar *filename);
static gboolean create_cache_dir (const gchar *dir_name);

static void fill_tile (ShumateMapSource *map_source,
    ShumateTile *tile);

static void store_tile (ShumateTileCache *tile_cache,
    ShumateTile *tile,
    const gchar *contents,
    gsize size);
static void refresh_tile_time (ShumateTileCache *tile_cache,
    ShumateTile *tile);
static void on_tile_filled (ShumateTileCache *tile_cache,
    ShumateTile *tile);

static void
shumate_file_cache_get_property (GObject *object,
    guint property_id,
    GValue *value,
    GParamSpec *pspec)
{
  ShumateFileCache *file_cache = SHUMATE_FILE_CACHE (object);

  switch (property_id)
    {
    case PROP_SIZE_LIMIT:
      g_value_set_uint (value, shumate_file_cache_get_size_limit (file_cache));
      break;

    case PROP_CACHE_DIR:
      g_value_set_string (value, shumate_file_cache_get_cache_dir (file_cache));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}


static void
shumate_file_cache_set_property (GObject *object,
    guint property_id,
    const GValue *value,
    GParamSpec *pspec)
{
  ShumateFileCache *file_cache = SHUMATE_FILE_CACHE (object);
  ShumateFileCachePrivate *priv = shumate_file_cache_get_instance_private (file_cache);

  switch (property_id)
    {
    case PROP_SIZE_LIMIT:
      shumate_file_cache_set_size_limit (file_cache, g_value_get_uint (value));
      break;

    case PROP_CACHE_DIR:
      if (priv->cache_dir)
        g_free (priv->cache_dir);
      priv->cache_dir = g_strdup (g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
finalize_sql (ShumateFileCache *file_cache)
{
  ShumateFileCachePrivate *priv = shumate_file_cache_get_instance_private (file_cache);

  g_clear_pointer (&priv->stmt_select, sqlite3_finalize);
  g_clear_pointer (&priv->stmt_update, sqlite3_finalize);

  if (priv->db)
    {
      gint error = sqlite3_close (priv->db);
      if (error != SQLITE_OK)
        DEBUG ("Sqlite returned error %d when closing cache.db", error);
      priv->db = NULL;
    }
}


static void
shumate_file_cache_finalize (GObject *object)
{
  ShumateFileCache *file_cache = SHUMATE_FILE_CACHE (object);
  ShumateFileCachePrivate *priv = shumate_file_cache_get_instance_private (file_cache);

  finalize_sql (file_cache);

  g_clear_pointer (&priv->cache_dir, g_free);

  G_OBJECT_CLASS (shumate_file_cache_parent_class)->finalize (object);
}


static gboolean
create_cache_dir (const gchar *dir_name)
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
init_cache (ShumateFileCache *file_cache)
{
  ShumateFileCachePrivate *priv = shumate_file_cache_get_instance_private (file_cache);
  gchar *filename = NULL;
  gchar *error_msg = NULL;
  gint error;

  g_return_if_fail (create_cache_dir (priv->cache_dir));

  filename = g_build_filename (priv->cache_dir,
        "cache.db", NULL);
  error = sqlite3_open_v2 (filename, &priv->db,
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
  g_free (filename);

  if (error == SQLITE_ERROR)
    {
      DEBUG ("Sqlite returned error %d when opening cache.db", error);
      return;
    }

  sqlite3_exec (priv->db,
      "PRAGMA synchronous=OFF;"
      "PRAGMA auto_vacuum=INCREMENTAL;",
      NULL, NULL, &error_msg);
  if (error_msg != NULL)
    {
      DEBUG ("Set PRAGMA: %s", error_msg);
      sqlite3_free (error_msg);
      return;
    }

  sqlite3_exec (priv->db,
      "CREATE TABLE IF NOT EXISTS tiles ("
      "filename TEXT PRIMARY KEY, "
      "etag TEXT, "
      "popularity INT DEFAULT 1, "
      "size INT DEFAULT 0)",
      NULL, NULL, &error_msg);
  if (error_msg != NULL)
    {
      DEBUG ("Creating table 'tiles' failed: %s", error_msg);
      sqlite3_free (error_msg);
      return;
    }

  error = sqlite3_prepare_v2 (priv->db,
        "SELECT etag FROM tiles WHERE filename = ?", -1,
        &priv->stmt_select, NULL);
  if (error != SQLITE_OK)
    {
      priv->stmt_select = NULL;
      DEBUG ("Failed to prepare the select Etag statement, error:%d: %s",
          error, sqlite3_errmsg (priv->db));
      return;
    }

  error = sqlite3_prepare_v2 (priv->db,
        "UPDATE tiles SET popularity = popularity + 1 WHERE filename = ?", -1,
        &priv->stmt_update, NULL);
  if (error != SQLITE_OK)
    {
      priv->stmt_update = NULL;
      DEBUG ("Failed to prepare the update popularity statement, error: %s",
          sqlite3_errmsg (priv->db));
      return;
    }

  g_object_notify (G_OBJECT (file_cache), "cache-dir");
}


static void
shumate_file_cache_constructed (GObject *object)
{
  ShumateFileCache *file_cache = SHUMATE_FILE_CACHE (object);
  ShumateFileCachePrivate *priv = shumate_file_cache_get_instance_private (file_cache);

  if (!priv->cache_dir)
    {
      priv->cache_dir = g_build_path (G_DIR_SEPARATOR_S,
            g_get_user_cache_dir (),
            "shumate", NULL);
    }

  init_cache (file_cache);

  G_OBJECT_CLASS (shumate_file_cache_parent_class)->constructed (object);
}


static void
shumate_file_cache_class_init (ShumateFileCacheClass *klass)
{
  ShumateMapSourceClass *map_source_class = SHUMATE_MAP_SOURCE_CLASS (klass);
  ShumateTileCacheClass *tile_cache_class = SHUMATE_TILE_CACHE_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

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
  pspec = g_param_spec_uint ("size-limit",
        "Size Limit",
        "The cache's size limit (Mb)",
        1,
        G_MAXINT,
        100000000,
        G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_SIZE_LIMIT, pspec);

  /**
   * ShumateFileCache:cache-dir:
   *
   * The directory where the tile database is stored.
   */
  pspec = g_param_spec_string ("cache-dir",
        "Cache Directory",
        "The directory of the cache",
        NULL,
        G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_CACHE_DIR, pspec);

  tile_cache_class->store_tile = store_tile;
  tile_cache_class->refresh_tile_time = refresh_tile_time;
  tile_cache_class->on_tile_filled = on_tile_filled;

  map_source_class->fill_tile = fill_tile;
}


static void
shumate_file_cache_init (ShumateFileCache *file_cache)
{
  ShumateFileCachePrivate *priv = shumate_file_cache_get_instance_private (file_cache);

  priv->cache_dir = NULL;
  priv->size_limit = 100000000;
  priv->cache_dir = NULL;
  priv->db = NULL;
  priv->stmt_select = NULL;
  priv->stmt_update = NULL;
}


/**
 * shumate_file_cache_new_full:
 * @size_limit: maximum size of the cache in bytes
 * @cache_dir: (allow-none): the directory where the cache is created. When cache_dir == NULL,
 * a cache in ~/.cache/shumate is used.
 * @renderer: the #ShumateRenderer used for tiles rendering
 *
 * Constructor of #ShumateFileCache.
 *
 * Returns: a constructed #ShumateFileCache
 */
ShumateFileCache *
shumate_file_cache_new_full (guint size_limit,
    const gchar *cache_dir,
    ShumateRenderer *renderer)
{
  ShumateFileCache *cache;

  cache = g_object_new (SHUMATE_TYPE_FILE_CACHE,
        "size-limit", size_limit,
        "cache-dir", cache_dir,
        "renderer", renderer,
        NULL);
  return cache;
}


/**
 * shumate_file_cache_get_size_limit:
 * @file_cache: a #ShumateFileCache
 *
 * Gets the cache size limit in bytes.
 *
 * Returns: size limit
 */
guint
shumate_file_cache_get_size_limit (ShumateFileCache *file_cache)
{
  ShumateFileCachePrivate *priv = shumate_file_cache_get_instance_private (file_cache);

  g_return_val_if_fail (SHUMATE_IS_FILE_CACHE (file_cache), 0);

  return priv->size_limit;
}


/**
 * shumate_file_cache_get_cache_dir:
 * @file_cache: a #ShumateFileCache
 *
 * Gets the directory where the cache database is stored.
 *
 * Returns: the directory
 */
const gchar *
shumate_file_cache_get_cache_dir (ShumateFileCache *file_cache)
{
  ShumateFileCachePrivate *priv = shumate_file_cache_get_instance_private (file_cache);

  g_return_val_if_fail (SHUMATE_IS_FILE_CACHE (file_cache), NULL);

  return priv->cache_dir;
}


/**
 * shumate_file_cache_set_size_limit:
 * @file_cache: a #ShumateFileCache
 * @size_limit: the cache limit in bytes
 *
 * Sets the cache size limit in bytes.
 */
void
shumate_file_cache_set_size_limit (ShumateFileCache *file_cache,
    guint size_limit)
{
  ShumateFileCachePrivate *priv = shumate_file_cache_get_instance_private (file_cache);

  g_return_if_fail (SHUMATE_IS_FILE_CACHE (file_cache));

  priv->size_limit = size_limit;
  g_object_notify (G_OBJECT (file_cache), "size-limit");
}


static gchar *
get_filename (ShumateFileCache *file_cache,
    ShumateTile *tile)
{
  ShumateFileCachePrivate *priv = shumate_file_cache_get_instance_private (file_cache);

  g_return_val_if_fail (SHUMATE_IS_FILE_CACHE (file_cache), NULL);
  g_return_val_if_fail (SHUMATE_IS_TILE (tile), NULL);
  g_return_val_if_fail (priv->cache_dir, NULL);

  ShumateMapSource *map_source = SHUMATE_MAP_SOURCE (file_cache);

  gchar *filename = g_strdup_printf ("%s" G_DIR_SEPARATOR_S
        "%s" G_DIR_SEPARATOR_S
        "%d" G_DIR_SEPARATOR_S
        "%d" G_DIR_SEPARATOR_S "%d.png",
        priv->cache_dir,
        shumate_map_source_get_id (map_source),
        shumate_tile_get_zoom_level (tile),
        shumate_tile_get_x (tile),
        shumate_tile_get_y (tile));
  return filename;
}


static gboolean
tile_is_expired (ShumateFileCache *file_cache,
    ShumateTile *tile)
{
  GDateTime *modified_time;
  gboolean validate_cache = TRUE;

  g_return_val_if_fail (SHUMATE_FILE_CACHE (file_cache), FALSE);
  g_return_val_if_fail (SHUMATE_TILE (tile), FALSE);

  modified_time = shumate_tile_get_modified_time (tile);
  if (modified_time)
    {
      GDateTime *now = g_date_time_new_now_utc ();
      GTimeSpan diff = g_date_time_difference (now, modified_time);

      validate_cache = (diff > 7 * G_TIME_SPAN_DAY); /* Cache expires in 7 days */
      g_date_time_unref (now);
    }

  DEBUG ("%p is %s expired", tile, (validate_cache ? "" : "not"));

  return validate_cache;
}


typedef struct
{
  ShumateMapSource *map_source;
  ShumateTile *tile;
} FileLoadedData;

static void
tile_rendered_cb (ShumateTile *tile,
    gpointer data,
    guint size,
    gboolean error,
    FileLoadedData *user_data)
{
  ShumateMapSource *map_source = user_data->map_source;
  GFile *file;
  ShumateFileCache *file_cache;
  ShumateMapSource *next_source;
  ShumateFileCachePrivate *priv;
  GFileInfo *info = NULL;
  GDateTime *modified_time;
  gchar *filename = NULL;

  g_signal_handlers_disconnect_by_func (tile, tile_rendered_cb, user_data);
  g_slice_free (FileLoadedData, user_data);

  next_source = shumate_map_source_get_next_source (map_source);
  file_cache = SHUMATE_FILE_CACHE (map_source);
  priv = shumate_file_cache_get_instance_private (file_cache);

  if (error)
    {
      DEBUG ("Tile rendering failed");
      goto load_next;
    }

  shumate_tile_set_state (tile, SHUMATE_STATE_LOADED);

  filename = get_filename (file_cache, tile);
  file = g_file_new_for_path (filename);

  /* Retrieve modification time */
  info = g_file_query_info (file,
        G_FILE_ATTRIBUTE_TIME_MODIFIED,
        G_FILE_QUERY_INFO_NONE, NULL, NULL);
  if (info)
    {
      modified_time = g_file_info_get_modification_date_time (info);
      shumate_tile_set_modified_time (tile, modified_time);

      g_date_time_unref (modified_time);
      g_object_unref (info);
    }
  g_object_unref (file);

  /* Notify other caches that the tile has been filled */
  if (SHUMATE_IS_TILE_CACHE (next_source))
    shumate_tile_cache_on_tile_filled (SHUMATE_TILE_CACHE (next_source), tile);

  if (tile_is_expired (file_cache, tile))
    {
      int sql_rc = SQLITE_OK;

      /* Retrieve etag */
      sqlite3_reset (priv->stmt_select);
      sql_rc = sqlite3_bind_text (priv->stmt_select, 1, filename, -1, SQLITE_STATIC);
      if (sql_rc == SQLITE_ERROR)
        {
          DEBUG ("Failed to prepare the SQL query for finding the Etag of '%s', error: %s",
              filename, sqlite3_errmsg (priv->db));
          goto load_next;
        }

      sql_rc = sqlite3_step (priv->stmt_select);
      if (sql_rc == SQLITE_ROW)
        {
          const gchar *etag = (const gchar *) sqlite3_column_text (priv->stmt_select, 0);
          shumate_tile_set_etag (SHUMATE_TILE (tile), etag);
        }
      else if (sql_rc == SQLITE_DONE)
        {
          DEBUG ("'%s' does't have an etag",
              filename);
          goto load_next;
        }
      else if (sql_rc == SQLITE_ERROR)
        {
          DEBUG ("Failed to finding the Etag of '%s', %d error: %s",
              filename, sql_rc, sqlite3_errmsg (priv->db));
          goto load_next;
        }

      /* Validate the tile */
      /* goto load_next; */
    }
  else
    {
      /* Tile loaded and no validation needed - done */
      shumate_tile_set_fade_in (tile, FALSE);
      shumate_tile_set_state (tile, SHUMATE_STATE_DONE);
      shumate_tile_display_content (tile);
      goto cleanup;
    }

load_next:
  if (SHUMATE_IS_MAP_SOURCE (next_source))
    shumate_map_source_fill_tile (next_source, tile);
  else if (shumate_tile_get_state (tile) == SHUMATE_STATE_LOADED)
    {
      /* if we have some content, use the tile even if it wasn't validated */
      shumate_tile_set_state (tile, SHUMATE_STATE_DONE);
      shumate_tile_display_content (tile);
    }

cleanup:
  g_free (filename);
  g_object_unref (tile);
  g_object_unref (map_source);
}


static void
file_loaded_cb (GFile *file,
    GAsyncResult *res,
    FileLoadedData *user_data)
{
  gboolean ok;
  gchar *contents;
  gsize length;
  GError *error = NULL;
  ShumateTile *tile = user_data->tile;
  ShumateMapSource *map_source = user_data->map_source;
  ShumateRenderer *renderer;

  ok = g_file_load_contents_finish (file, res, &contents, &length, NULL, &error);

  if (!ok)
    {
      gchar *path;

      path = g_file_get_path (file);
      DEBUG ("Failed to load tile %s, error: %s", path, error->message);
      g_free (path);
      contents = NULL;
      length = 0;
      g_error_free (error);
    }

  g_object_unref (file);

  renderer = shumate_map_source_get_renderer (map_source);

  g_return_if_fail (SHUMATE_IS_RENDERER (renderer));

  g_signal_connect (tile, "render-complete", G_CALLBACK (tile_rendered_cb), user_data);

  shumate_renderer_set_data (renderer, (guint8*) contents, length);
  g_free (contents);
  shumate_renderer_render (renderer, tile);
}


static void
fill_tile (ShumateMapSource *map_source,
    ShumateTile *tile)
{
  g_return_if_fail (SHUMATE_IS_FILE_CACHE (map_source));
  g_return_if_fail (SHUMATE_IS_TILE (tile));

  ShumateMapSource *next_source = shumate_map_source_get_next_source (map_source);

  if (shumate_tile_get_state (tile) == SHUMATE_STATE_DONE)
    return;

  if (shumate_tile_get_state (tile) != SHUMATE_STATE_LOADED)
    {
      FileLoadedData *user_data;
      gchar *filename;
      GFile *file;

      filename = get_filename (SHUMATE_FILE_CACHE (map_source), tile);
      file = g_file_new_for_path (filename);
      g_free (filename);

      user_data = g_slice_new (FileLoadedData);
      user_data->tile = tile;
      user_data->map_source = map_source;

      g_object_ref (tile);
      g_object_ref (map_source);

      DEBUG ("fill of %s", filename);

      g_file_load_contents_async (file, NULL, (GAsyncReadyCallback) file_loaded_cb, user_data);
    }
  else if (SHUMATE_IS_MAP_SOURCE (next_source))
    shumate_map_source_fill_tile (next_source, tile);
  else if (shumate_tile_get_state (tile) == SHUMATE_STATE_LOADED)
    {
      /* if we have some content, use the tile even if it wasn't validated */
      shumate_tile_set_state (tile, SHUMATE_STATE_DONE);
      shumate_tile_display_content (tile);
    }
}


static void
refresh_tile_time (ShumateTileCache *tile_cache,
    ShumateTile *tile)
{
  g_return_if_fail (SHUMATE_IS_FILE_CACHE (tile_cache));

  ShumateMapSource *map_source = SHUMATE_MAP_SOURCE (tile_cache);
  ShumateMapSource *next_source = shumate_map_source_get_next_source (map_source);
  ShumateFileCache *file_cache = SHUMATE_FILE_CACHE (tile_cache);
  gchar *filename = NULL;
  GFile *file;
  GFileInfo *info;

  filename = get_filename (file_cache, tile);
  file = g_file_new_for_path (filename);
  g_free (filename);

  info = g_file_query_info (file, G_FILE_ATTRIBUTE_TIME_MODIFIED,
        G_FILE_QUERY_INFO_NONE, NULL, NULL);

  if (info)
    {
      GDateTime *now = g_date_time_new_now_utc ();

      g_file_info_set_modification_date_time (info, now);
      g_file_set_attributes_from_info (file, info, G_FILE_QUERY_INFO_NONE, NULL, NULL);

      g_date_time_unref (now);
      g_object_unref (info);
    }

  g_object_unref (file);

  if (SHUMATE_IS_TILE_CACHE (next_source))
    shumate_tile_cache_refresh_tile_time (SHUMATE_TILE_CACHE (next_source), tile);
}


static void
store_tile (ShumateTileCache *tile_cache,
    ShumateTile *tile,
    const gchar *contents,
    gsize size)
{
  g_return_if_fail (SHUMATE_IS_FILE_CACHE (tile_cache));

  ShumateMapSource *map_source = SHUMATE_MAP_SOURCE (tile_cache);
  ShumateMapSource *next_source = shumate_map_source_get_next_source (map_source);
  ShumateFileCache *file_cache = SHUMATE_FILE_CACHE (tile_cache);
  ShumateFileCachePrivate *priv = shumate_file_cache_get_instance_private (file_cache);
  gchar *query = NULL;
  gchar *error = NULL;
  gchar *path = NULL;
  gchar *filename = NULL;
  GError *gerror = NULL;
  GFile *file;
  GFileOutputStream *ostream;
  gsize bytes_written;

  DEBUG ("Update of %p", tile);

  filename = get_filename (file_cache, tile);
  file = g_file_new_for_path (filename);

  /* If the file exists, delete it */
  g_file_delete (file, NULL, NULL);

  /* If needed, create the cache's dirs */
  path = g_path_get_dirname (filename);
  if (g_mkdir_with_parents (path, 0700) == -1)
    {
      if (errno != EEXIST)
        {
          g_warning ("Unable to create the image cache path '%s': %s",
              path, g_strerror (errno));
          goto store_next;
        }
    }

  ostream = g_file_create (file, G_FILE_CREATE_PRIVATE, NULL, &gerror);
  if (!ostream)
    {
      DEBUG ("GFileOutputStream creation failed: %s", gerror->message);
      g_error_free (gerror);
      goto store_next;
    }

  /* Write the cache */
  if (!g_output_stream_write_all (G_OUTPUT_STREAM (ostream), contents, size, &bytes_written, NULL, &gerror))
    {
      DEBUG ("Writing file contents failed: %s", gerror->message);
      g_error_free (gerror);
      g_object_unref (ostream);
      goto store_next;
    }

  g_object_unref (ostream);

  query = sqlite3_mprintf ("REPLACE INTO tiles (filename, etag, size) VALUES (%Q, %Q, %d)",
        filename,
        shumate_tile_get_etag (tile),
        size);
  sqlite3_exec (priv->db, query, NULL, NULL, &error);
  if (error != NULL)
    {
      DEBUG ("Saving Etag and size failed: %s", error);
      sqlite3_free (error);
    }
  sqlite3_free (query);

store_next:
  if (SHUMATE_IS_TILE_CACHE (next_source))
    shumate_tile_cache_store_tile (SHUMATE_TILE_CACHE (next_source), tile, contents, size);

  g_free (filename);
  g_free (path);
  g_object_unref (file);
}


static void
on_tile_filled (ShumateTileCache *tile_cache,
    ShumateTile *tile)
{
  g_return_if_fail (SHUMATE_IS_FILE_CACHE (tile_cache));
  g_return_if_fail (SHUMATE_IS_TILE (tile));

  ShumateMapSource *map_source = SHUMATE_MAP_SOURCE (tile_cache);
  ShumateMapSource *next_source = shumate_map_source_get_next_source (map_source);
  ShumateFileCache *file_cache = SHUMATE_FILE_CACHE (tile_cache);
  ShumateFileCachePrivate *priv = shumate_file_cache_get_instance_private (file_cache);
  int sql_rc = SQLITE_OK;
  gchar *filename = NULL;

  filename = get_filename (file_cache, tile);

  DEBUG ("popularity of %s", filename);

  sqlite3_reset (priv->stmt_update);
  sql_rc = sqlite3_bind_text (priv->stmt_update, 1, filename, -1, SQLITE_STATIC);
  if (sql_rc != SQLITE_OK)
    {
      DEBUG ("Failed to set values to the popularity query of '%s', error: %s",
          filename, sqlite3_errmsg (priv->db));
      goto call_next;
    }

  sql_rc = sqlite3_step (priv->stmt_update);
  if (sql_rc != SQLITE_DONE)
    {
      /* may not be present in this cache */
      goto call_next;
    }

call_next:
  g_free (filename);
  if (SHUMATE_IS_TILE_CACHE (next_source))
    shumate_tile_cache_on_tile_filled (SHUMATE_TILE_CACHE (next_source), tile);
}


static void
delete_tile (ShumateFileCache *file_cache, const gchar *filename)
{
  ShumateFileCachePrivate *priv = shumate_file_cache_get_instance_private (file_cache);

  g_return_if_fail (SHUMATE_IS_FILE_CACHE (file_cache));
  gchar *query, *error = NULL;
  GError *gerror = NULL;
  GFile *file;

  query = sqlite3_mprintf ("DELETE FROM tiles WHERE filename = %Q", filename);
  sqlite3_exec (priv->db, query, NULL, NULL, &error);
  if (error != NULL)
    {
      DEBUG ("Deleting tile from db failed: %s", error);
      sqlite3_free (error);
    }
  sqlite3_free (query);

  file = g_file_new_for_path (filename);
  if (!g_file_delete (file, NULL, &gerror))
    {
      DEBUG ("Deleting tile from disk failed: %s", gerror->message);
      g_error_free (gerror);
    }
  g_object_unref (file);
}


static gboolean
purge_on_idle (gpointer data)
{
  shumate_file_cache_purge (SHUMATE_FILE_CACHE (data));
  return FALSE;
}


/**
 * shumate_file_cache_purge_on_idle:
 * @file_cache: a #ShumateFileCache
 *
 * Purge the cache from the less popular tiles until cache's size limit is reached.
 * This is a non blocking call as the purge will happen when the application is idle
 */
void
shumate_file_cache_purge_on_idle (ShumateFileCache *file_cache)
{
  g_return_if_fail (SHUMATE_IS_FILE_CACHE (file_cache));
  g_idle_add_full (G_PRIORITY_HIGH + 50,
      (GSourceFunc) purge_on_idle,
      g_object_ref (file_cache),
      (GDestroyNotify) g_object_unref);
}


/**
 * shumate_file_cache_purge:
 * @file_cache: a #ShumateFileCache
 *
 * Purge the cache from the less popular tiles until cache's size limit is reached.
 */
void
shumate_file_cache_purge (ShumateFileCache *file_cache)
{
  ShumateFileCachePrivate *priv = shumate_file_cache_get_instance_private (file_cache);

  g_return_if_fail (SHUMATE_IS_FILE_CACHE (file_cache));

  gchar *query;
  sqlite3_stmt *stmt;
  int rc = 0;
  guint current_size = 0;
  guint highest_popularity = 0;
  gchar *error;

  query = "SELECT SUM (size) FROM tiles";
  rc = sqlite3_prepare (priv->db, query, strlen (query), &stmt, NULL);
  if (rc != SQLITE_OK)
    {
      DEBUG ("Can't compute cache size %s", sqlite3_errmsg (priv->db));
    }

  rc = sqlite3_step (stmt);
  if (rc != SQLITE_ROW)
    {
      DEBUG ("Failed to count the total cache consumption %s",
          sqlite3_errmsg (priv->db));
      sqlite3_finalize (stmt);
      return;
    }

  current_size = sqlite3_column_int (stmt, 0);
  if (current_size < priv->size_limit)
    {
      DEBUG ("Cache doesn't need to be purged at %d bytes", current_size);
      sqlite3_finalize (stmt);
      return;
    }

  sqlite3_finalize (stmt);

  /* Ok, delete the less popular tiles until size_limit reached */
  query = "SELECT filename, size, popularity FROM tiles ORDER BY popularity";
  rc = sqlite3_prepare (priv->db, query, strlen (query), &stmt, NULL);
  if (rc != SQLITE_OK)
    {
      DEBUG ("Can't fetch tiles to delete: %s", sqlite3_errmsg (priv->db));
    }

  rc = sqlite3_step (stmt);
  while (rc == SQLITE_ROW && current_size > priv->size_limit)
    {
      const char *filename;
      guint size;

      filename = (const char *) sqlite3_column_text (stmt, 0);
      size = sqlite3_column_int (stmt, 1);
      highest_popularity = sqlite3_column_int (stmt, 2);
      DEBUG ("Deleting %s of size %d", filename, size);

      delete_tile (file_cache, filename);

      current_size -= size;

      rc = sqlite3_step (stmt);
    }
  DEBUG ("Cache size is now %d", current_size);

  sqlite3_finalize (stmt);

  query = sqlite3_mprintf ("UPDATE tiles SET popularity = popularity - %d",
        highest_popularity);
  sqlite3_exec (priv->db, query, NULL, NULL, &error);
  if (error != NULL)
    {
      DEBUG ("Updating popularity failed: %s", error);
      sqlite3_free (error);
    }
  sqlite3_free (query);

  sqlite3_exec (priv->db, "PRAGMA incremental_vacuum;", NULL, NULL, &error);
}
