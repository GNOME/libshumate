/*
 * Copyright (C) 2021 James Westman <james@jwestman.net>
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
 * License along with this library; if not, see <https://www.gnu.org/licenses/>.
 */

#include <libsoup/soup.h>
#include "shumate-tile-downloader.h"
#include "shumate-file-cache.h"
#include "shumate-user-agent.h"

/**
 * ShumateTileDownloader:
 *
 * A [class@DataSource] that asynchronously downloads tiles from an online
 * service using a given template.
 *
 * It contains an internal [class@FileCache] to cache the tiles on the system.
 */

struct _ShumateTileDownloader
{
  ShumateDataSource parent_instance;

  char *url_template;

  SoupSession *soup_session;
  ShumateFileCache *cache;
};

G_DEFINE_TYPE (ShumateTileDownloader, shumate_tile_downloader, SHUMATE_TYPE_DATA_SOURCE)

enum {
  PROP_0,
  PROP_URL_TEMPLATE,
  N_PROPS
};
static GParamSpec *properties [N_PROPS];


/* The osm.org tile set require us to use no more than 2 simultaneous
 * connections so let that be the default.
 */
#define MAX_CONNS_DEFAULT 2


/**
 * shumate_tile_downloader_new:
 * @url_template: a URL template to fetch tiles from
 *
 * Creates a new [class@TileDownloader] that fetches tiles from an API and
 * caches them on disk.
 *
 * See [property@TileDownloader:url-template] for the format of the URL template.
 *
 * Returns: (transfer full): a newly constructed [class@TileDownloader]
 */
ShumateTileDownloader *
shumate_tile_downloader_new (const char *url_template)
{
  return g_object_new (SHUMATE_TYPE_TILE_DOWNLOADER,
                       "url-template", url_template,
                       NULL);
}

static void
shumate_tile_downloader_constructed (GObject *object)
{
  ShumateTileDownloader *self = (ShumateTileDownloader *)object;
  g_autofree char *cache_key = NULL;

  cache_key = g_strcanon (g_strdup (self->url_template), "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789", '_');
  self->cache = shumate_file_cache_new_full (100 * 1000 * 1000, cache_key, NULL);

  G_OBJECT_CLASS (shumate_tile_downloader_parent_class)->constructed (object);
}

static void
shumate_tile_downloader_finalize (GObject *object)
{
  ShumateTileDownloader *self = (ShumateTileDownloader *)object;

  g_clear_pointer (&self->url_template, g_free);
  g_clear_object (&self->soup_session);
  g_clear_object (&self->cache);

  G_OBJECT_CLASS (shumate_tile_downloader_parent_class)->finalize (object);
}

static void
shumate_tile_downloader_get_property (GObject    *object,
                                      guint       prop_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  ShumateTileDownloader *self = SHUMATE_TILE_DOWNLOADER (object);

  switch (prop_id)
    {
    case PROP_URL_TEMPLATE:
      g_value_set_string (value, self->url_template);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
shumate_tile_downloader_set_property (GObject      *object,
                                      guint         prop_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  ShumateTileDownloader *self = SHUMATE_TILE_DOWNLOADER (object);

  switch (prop_id)
    {
    case PROP_URL_TEMPLATE:
      self->url_template = g_strdup (g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}


static void
get_tile_data_async (ShumateDataSource   *data_source,
                     int                  x,
                     int                  y,
                     int                  zoom_level,
                     GCancellable        *cancellable,
                     GAsyncReadyCallback  callback,
                     gpointer             user_data);

static void
shumate_tile_downloader_class_init (ShumateTileDownloaderClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ShumateDataSourceClass *source_class = SHUMATE_DATA_SOURCE_CLASS (klass);

  object_class->constructed = shumate_tile_downloader_constructed;
  object_class->finalize = shumate_tile_downloader_finalize;
  object_class->get_property = shumate_tile_downloader_get_property;
  object_class->set_property = shumate_tile_downloader_set_property;

  source_class->get_tile_data_async = get_tile_data_async;

  /**
   * ShumateTileDownloader:url-template:
   *
   * A template for construting the URL to download a tile from.
   *
   * The template has the following replacements:
   * - "{x}": The X coordinate of the tile
   * - "{y}": The Y coordinate of the tile
   * - "{z}": The zoom level of the tile
   * - "{tmsy}": The inverted Y coordinate (i.e. tile numbering starts with 0 at
   * the bottom, rather than top, of the map)
   */
  properties[PROP_URL_TEMPLATE] =
    g_param_spec_string ("url-template", "URL template", "URL template",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
shumate_tile_downloader_init (ShumateTileDownloader *self)
{
}


static void on_file_cache_get_tile (GObject *source_object, GAsyncResult *res, gpointer user_data);
static void fetch_from_network (GTask *task);
static void on_message_sent (GObject *source_object, GAsyncResult *res, gpointer user_data);
static void on_message_read (GObject *source_object, GAsyncResult *res, gpointer user_data);

static void on_file_cache_stored (GObject *source_object, GAsyncResult *res, gpointer user_data);

typedef struct {
  ShumateTileDownloader *self;
  int x;
  int y;
  int z;
  GBytes *bytes;
  char *etag;
  SoupMessage *msg;
  GDateTime *modtime;
} FillTileData;

static void
fill_tile_data_free (FillTileData *data)
{
  g_clear_object (&data->self);
  g_clear_pointer (&data->bytes, g_bytes_unref);
  g_clear_pointer (&data->etag, g_free);
  g_clear_object (&data->msg);
  g_clear_pointer (&data->modtime, g_date_time_unref);
  g_free (data);
}

static gboolean
tile_is_expired (GDateTime *modified_time)
{
  g_autoptr(GDateTime) now = g_date_time_new_now_utc ();
  GTimeSpan diff = g_date_time_difference (now, modified_time);

  return diff > 7 * G_TIME_SPAN_DAY; /* Cache expires in 7 days */
}

static char *
get_modified_time_string (GDateTime *modified_time)
{
  if (modified_time == NULL)
    return NULL;

  return g_date_time_format (modified_time, "%a, %d %b %Y %T %Z");
}


static char *
get_tile_uri (ShumateTileDownloader *self,
              int                    x,
              int                    y,
              int                    z)
{
  GString *string = g_string_new (self->url_template);
  g_autofree char *x_str = g_strdup_printf("%d", x);
  g_autofree char *y_str = g_strdup_printf("%d", y);
  g_autofree char *z_str = g_strdup_printf("%d", z);
  g_autofree char *tmsy_str = g_strdup_printf("%d", (1 << z) - y - 1);

  g_string_replace (string, "{x}", x_str, 0);
  g_string_replace (string, "{y}", y_str, 0);
  g_string_replace (string, "{z}", z_str, 0);
  g_string_replace (string, "{tmsy}", tmsy_str, 0);

  return g_string_free (string, FALSE);
}


static void
get_tile_data_async (ShumateDataSource   *data_source,
                     int                  x,
                     int                  y,
                     int                  zoom_level,
                     GCancellable        *cancellable,
                     GAsyncReadyCallback  callback,
                     gpointer             user_data)
{
  g_autoptr(GTask) task = NULL;
  ShumateTileDownloader *self = (ShumateTileDownloader *)data_source;
  FillTileData *data;

  g_return_if_fail (SHUMATE_IS_TILE_DOWNLOADER (self));
  g_return_if_fail (!cancellable || G_IS_CANCELLABLE (cancellable));

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_source_tag (task, get_tile_data_async);

  data = g_new0 (FillTileData, 1);
  data->self = g_object_ref (self);
  data->x = x;
  data->y = y;
  data->z = zoom_level;
  g_task_set_task_data (task, data, (GDestroyNotify) fill_tile_data_free);

  shumate_file_cache_get_tile_async (self->cache, x, y, zoom_level, cancellable, on_file_cache_get_tile, g_object_ref (task));
}

static void
on_file_cache_get_tile (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  g_autoptr(GTask) task = user_data;
  FillTileData *data = g_task_get_task_data (task);

  data->bytes = shumate_file_cache_get_tile_finish (SHUMATE_FILE_CACHE (source_object),
                                                    &data->etag, &data->modtime, res, NULL);

  if (data->bytes != NULL)
    {
      g_signal_emit_by_name (data->self, "received-data", data->x, data->y, data->z, data->bytes);

      if (!tile_is_expired (data->modtime))
        {
          g_task_return_pointer (task, g_steal_pointer (&data->bytes), (GDestroyNotify)g_bytes_unref);
          return;
        }
    }

  fetch_from_network (task);
}

static void
fetch_from_network (GTask *task)
{
  FillTileData *data = g_task_get_task_data (task);
  GCancellable *cancellable = g_task_get_cancellable (task);
  g_autofree char *uri = NULL;
  g_autofree char *modtime_string = NULL;
  SoupMessageHeaders *headers;

  if (g_task_return_error_if_cancelled (task))
    return;

  uri = get_tile_uri (data->self, data->x, data->y, data->z);

  data->msg = soup_message_new (SOUP_METHOD_GET, uri);

  if (data->msg == NULL)
    {
      g_task_return_new_error (task, SHUMATE_TILE_DOWNLOADER_ERROR,
                               SHUMATE_TILE_DOWNLOADER_ERROR_MALFORMED_URL,
                               "The URL %s is not valid", uri);
      return;
    }

  modtime_string = get_modified_time_string (data->modtime);
#ifdef SHUMATE_LIBSOUP_3
  headers = soup_message_get_request_headers (data->msg);
#else
  headers = data->msg->request_headers;
#endif

  /* If an etag is available, only use it.
   * OSM servers seems to send now as the modified time for all tiles
   * Omarender servers set the modified time correctly
   */
  if (data->etag)
    {
      g_debug ("If-None-Match: %s", data->etag);
      soup_message_headers_append (headers, "If-None-Match", data->etag);
    }
  else if (modtime_string)
    {
      g_debug ("If-Modified-Since %s", modtime_string);
      soup_message_headers_append (headers, "If-Modified-Since", modtime_string);
    }

  if (data->self->soup_session == NULL)
    {
#ifdef SHUMATE_LIBSOUP_3
      data->self->soup_session =
        soup_session_new_with_options ("user-agent", "libshumate/" SHUMATE_VERSION,
                                       "max-conns-per-host", MAX_CONNS_DEFAULT,
                                       "max-conns", MAX_CONNS_DEFAULT,
                                       NULL);
#else
      data->self->soup_session
        = soup_session_new_with_options ("proxy-uri", NULL,
                                         "ssl-strict", FALSE,
                                         SOUP_SESSION_ADD_FEATURE_BY_TYPE,
                                         SOUP_TYPE_PROXY_RESOLVER_DEFAULT,
                                         SOUP_SESSION_ADD_FEATURE_BY_TYPE,
                                         SOUP_TYPE_CONTENT_DECODER,
                                         NULL);

      g_object_set (G_OBJECT (data->self->soup_session),
                    "user-agent", shumate_get_user_agent (),
                    "max-conns-per-host", MAX_CONNS_DEFAULT,
                    "max-conns", MAX_CONNS_DEFAULT,
                    NULL);

#endif
    }

  soup_session_send_async (data->self->soup_session,
                           data->msg,
#ifdef SHUMATE_LIBSOUP_3
                           G_PRIORITY_DEFAULT,
#endif
                           cancellable,
                           on_message_sent,
                           g_object_ref (task));
}

/* Receive the response from the network. If the tile hasn't been modified,
 * return; otherwise, read the data into a GBytes. */
static void
on_message_sent (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  g_autoptr(GTask) task = user_data;
  FillTileData *data = g_task_get_task_data (task);
  GCancellable *cancellable = g_task_get_cancellable (task);
  g_autoptr(GInputStream) input_stream = NULL;
  g_autoptr(GError) error = NULL;
  g_autoptr(GOutputStream) output_stream = NULL;
  SoupStatus status;
  SoupMessageHeaders *headers;

  input_stream = soup_session_send_finish (data->self->soup_session, res, &error);
  if (error != NULL)
    {
      if (data->bytes)
        {
          /* The tile has already been filled from the cache, so the operation
           * was overall successful even though the network request failed. */
          g_debug ("Fetching tile failed, but there is a cached version (error: %s)", error->message);
          g_task_return_pointer (task, g_steal_pointer (&data->bytes), (GDestroyNotify)g_bytes_unref);
        }
      else
        g_task_return_error (task, g_steal_pointer (&error));

      return;
    }

#ifdef SHUMATE_LIBSOUP_3
  status = soup_message_get_status (data->msg);
  headers = soup_message_get_response_headers (data->msg);
#else
  status = data->msg->status_code;
  headers = data->msg->response_headers;
#endif

  g_debug ("Got reply %d", status);

  if (status == SOUP_STATUS_NOT_MODIFIED)
    {
      /* The tile has already been filled from the cache, and the server says
       * it doesn't have a newer one. Just update the cache, mark the tile as
       * DONE, and return. */

      shumate_file_cache_mark_up_to_date (data->self->cache, data->x, data->y, data->z);
      g_task_return_pointer (task, g_steal_pointer (&data->bytes), (GDestroyNotify)g_bytes_unref);
      return;
    }

  if (!SOUP_STATUS_IS_SUCCESSFUL (status))
    {
      if (data->bytes)
        {
          g_debug ("Fetching tile failed, but there is a cached version (HTTP %s)",
                   soup_status_get_phrase (status));
          g_task_return_pointer (task, g_steal_pointer (&data->bytes), (GDestroyNotify)g_bytes_unref);
        }
      else
        {
          g_task_return_new_error (task, SHUMATE_TILE_DOWNLOADER_ERROR,
                                   SHUMATE_TILE_DOWNLOADER_ERROR_BAD_RESPONSE,
                                   "Unable to download tile: HTTP %s",
                                   soup_status_get_phrase (status));
        }

      return;
    }

  /* Verify if the server sent an etag and save it */
  g_clear_pointer (&data->etag, g_free);
  data->etag = g_strdup (soup_message_headers_get_one (headers, "ETag"));
  g_debug ("Received ETag %s", data->etag);

  output_stream = g_memory_output_stream_new_resizable ();
  g_output_stream_splice_async (output_stream,
                                input_stream,
                                G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET,
                                G_PRIORITY_DEFAULT,
                                cancellable,
                                on_message_read,
                                g_steal_pointer (&task));
}

static void
on_message_read (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  GOutputStream *output_stream = G_OUTPUT_STREAM (source_object);
  g_autoptr(GTask) task = user_data;
  GCancellable *cancellable = g_task_get_cancellable (task);
  FillTileData *data = g_task_get_task_data (task);
  g_autoptr(GError) error = NULL;

  g_output_stream_splice_finish (output_stream, res, &error);
  if (error != NULL)
    {
      g_task_return_error (task, g_steal_pointer (&error));
      return;
    }

  g_bytes_unref (data->bytes);
  data->bytes = g_memory_output_stream_steal_as_bytes (G_MEMORY_OUTPUT_STREAM (output_stream));

  g_signal_emit_by_name (data->self, "received-data", data->x, data->y, data->z, data->bytes);

  shumate_file_cache_store_tile_async (data->self->cache,
                                       data->x, data->y, data->z,
                                       data->bytes,
                                       data->etag,
                                       cancellable,
                                       on_file_cache_stored,
                                       g_steal_pointer (&task));
}

static void
on_file_cache_stored (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  g_autoptr(GTask) task = user_data;
  FillTileData *data = g_task_get_task_data (task);

  g_task_return_pointer (task, g_steal_pointer (&data->bytes), (GDestroyNotify)g_bytes_unref);
}


/**
 * shumate_tile_downloader_error_quark:
 *
 * Gets the #ShumateTileDownloader error quark.
 *
 * Returns: a #GQuark
 */
G_DEFINE_QUARK (shumate-tile-downloader-error-quark, shumate_tile_downloader_error);
