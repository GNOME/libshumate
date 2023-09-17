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
#include "shumate-profiling-private.h"
#include "shumate-data-source-request.h"

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

static ShumateDataSourceRequest *
start_request (ShumateDataSource *data_source,
               int                x,
               int                y,
               int                zoom_level,
               GCancellable      *cancellable);

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
  source_class->start_request = start_request;

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


typedef struct {
  ShumateTileDownloader *self;
  ShumateDataSourceRequest *req;
  GCancellable *cancellable;
  char *etag;
  SoupMessage *msg;
  GDateTime *modtime;
} FillTileData;

static void
fill_tile_data_free (FillTileData *data)
{
  g_clear_object (&data->self);
  g_clear_object (&data->req);
  g_clear_object (&data->cancellable);
  g_clear_pointer (&data->etag, g_free);
  g_clear_object (&data->msg);
  g_clear_pointer (&data->modtime, g_date_time_unref);
  g_free (data);
}

G_DEFINE_AUTOPTR_CLEANUP_FUNC (FillTileData, fill_tile_data_free)

static void on_file_cache_get_tile (GObject *source_object, GAsyncResult *res, gpointer user_data);
static void fetch_from_network (FillTileData *data);
static void on_message_sent (GObject *source_object, GAsyncResult *res, gpointer user_data);
static void on_message_read (GObject *source_object, GAsyncResult *res, gpointer user_data);


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
on_request_notify_completed (GTask                    *task,
                             GParamSpec               *pspec,
                             ShumateDataSourceRequest *req)
{
  GBytes *bytes;

  if ((bytes = shumate_data_source_request_get_data (req)))
    g_task_return_pointer (task, g_bytes_ref (bytes), (GDestroyNotify)g_bytes_unref);
  else
    g_task_return_error (task, g_error_copy (shumate_data_source_request_get_error (req)));

  g_clear_object (&task);
}

static void
on_request_notify_data (ShumateTileDownloader    *self,
                        GParamSpec               *pspec,
                        ShumateDataSourceRequest *req)
{
  GBytes *bytes = shumate_data_source_request_get_data (req);

  if (bytes != NULL)
    {
      int x = shumate_data_source_request_get_x (req);
      int y = shumate_data_source_request_get_y (req);
      int z = shumate_data_source_request_get_zoom_level (req);
      g_autofree char *profiling_desc = g_strdup_printf ("(%d, %d) @ %d", x, y, z);

      SHUMATE_PROFILE_START_NAMED (emit_received_data);
      g_signal_emit_by_name (self, "received-data", x, y, z, bytes);
      SHUMATE_PROFILE_END_NAMED (emit_received_data, profiling_desc);
    }
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
  g_autoptr(ShumateDataSourceRequest) req = NULL;

  g_return_if_fail (SHUMATE_IS_TILE_DOWNLOADER (self));
  g_return_if_fail (!cancellable || G_IS_CANCELLABLE (cancellable));

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_source_tag (task, get_tile_data_async);

  req = start_request ((ShumateDataSource *)self, x, y, zoom_level, cancellable);

  if (shumate_data_source_request_is_completed (req))
    {
      on_request_notify_data (self, NULL, req);
      on_request_notify_completed (task, NULL, req);
    }
  else
    {
      g_signal_connect_object (req,
                               "notify::data",
                               (GCallback)on_request_notify_data,
                               self,
                               G_CONNECT_SWAPPED);
      g_signal_connect_object (req,
                               "notify::completed",
                               (GCallback)on_request_notify_completed,
                               g_steal_pointer (&task),
                               G_CONNECT_SWAPPED);
    }
}

static ShumateDataSourceRequest *
start_request (ShumateDataSource *data_source,
               int                x,
               int                y,
               int                zoom_level,
               GCancellable      *cancellable)
{
  ShumateTileDownloader *self = (ShumateTileDownloader *)data_source;
  g_autoptr(ShumateDataSourceRequest) req = NULL;
  FillTileData *data;

  req = shumate_data_source_request_new (x, y, zoom_level);
  data = g_new0 (FillTileData, 1);
  data->self = g_object_ref (self);
  data->req = g_object_ref (req);
  data->cancellable = cancellable ? g_object_ref (cancellable) : NULL;

  shumate_file_cache_get_tile_async (self->cache, x, y, zoom_level, cancellable, on_file_cache_get_tile, data);

  return g_steal_pointer (&req);
}

static void
on_file_cache_get_tile (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  g_autoptr(FillTileData) data = user_data;
  g_autoptr(GBytes) bytes = NULL;

  bytes = shumate_file_cache_get_tile_finish (SHUMATE_FILE_CACHE (source_object),
                                              &data->etag, &data->modtime, res, NULL);

  if (bytes != NULL)
    {
      gboolean complete = !tile_is_expired (data->modtime);

      shumate_data_source_request_emit_data (data->req, bytes, complete);

      if (complete)
        return;
    }

  fetch_from_network (g_steal_pointer (&data));
}

static void
fetch_from_network (FillTileData *data_arg)
{
  g_autoptr(FillTileData) data = data_arg;
  g_autofree char *uri = NULL;
  g_autofree char *modtime_string = NULL;
  SoupMessageHeaders *headers;
  int x, y, z;

  if (g_cancellable_is_cancelled (data->cancellable))
    {
      g_autoptr(GError) error = g_error_new (G_IO_ERROR, G_IO_ERROR_CANCELLED, "Cancelled");
      shumate_data_source_request_emit_error (data->req, error);
      return;
    }

  x = shumate_data_source_request_get_x (data->req);
  y = shumate_data_source_request_get_y (data->req);
  z = shumate_data_source_request_get_zoom_level (data->req);
  uri = get_tile_uri (data->self, x, y, z);

  data->msg = soup_message_new (SOUP_METHOD_GET, uri);

  if (data->msg == NULL)
    {
      g_autoptr(GError) error =
        g_error_new (SHUMATE_TILE_DOWNLOADER_ERROR,
                     SHUMATE_TILE_DOWNLOADER_ERROR_MALFORMED_URL,
                     "The URL %s is not valid", uri);
      shumate_data_source_request_emit_error (data->req, error);
      return;
    }

  modtime_string = get_modified_time_string (data->modtime);
  headers = soup_message_get_request_headers (data->msg);

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
      data->self->soup_session =
        soup_session_new_with_options ("user-agent", "libshumate/" SHUMATE_VERSION,
                                       "max-conns-per-host", MAX_CONNS_DEFAULT,
                                       "max-conns", MAX_CONNS_DEFAULT,
                                       NULL);
    }

  soup_session_send_async (data->self->soup_session,
                           data->msg,
                           G_PRIORITY_DEFAULT,
                           data->cancellable,
                           on_message_sent,
                           data);
  data = NULL;
}

/* Receive the response from the network. If the tile hasn't been modified,
 * return; otherwise, read the data into a GBytes. */
static void
on_message_sent (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  g_autoptr(FillTileData) data = user_data;
  g_autoptr(GInputStream) input_stream = NULL;
  g_autoptr(GError) error = NULL;
  g_autoptr(GOutputStream) output_stream = NULL;
  SoupStatus status;
  SoupMessageHeaders *headers;

  input_stream = soup_session_send_finish (data->self->soup_session, res, &error);
  if (error != NULL)
    {
      if (shumate_data_source_request_get_data (data->req))
        {
          /* The tile has already been filled from the cache, so the operation
           * was overall successful even though the network request failed. */
          g_debug ("Fetching tile failed, but there is a cached version (error: %s)", error->message);
          shumate_data_source_request_complete (data->req);
        }
      else
        shumate_data_source_request_emit_error (data->req, error);

      return;
    }

  status = soup_message_get_status (data->msg);
  headers = soup_message_get_response_headers (data->msg);

  g_debug ("Got reply %d", status);

  if (status == SOUP_STATUS_NOT_MODIFIED)
    {
      /* The tile has already been filled from the cache, and the server says
       * it doesn't have a newer one. Just update the cache and return. */

      int x, y, z;

      x = shumate_data_source_request_get_x (data->req);
      y = shumate_data_source_request_get_y (data->req);
      z = shumate_data_source_request_get_zoom_level (data->req);

      shumate_file_cache_mark_up_to_date (data->self->cache, x, y, z);
      shumate_data_source_request_complete (data->req);
      return;
    }

  if (!SOUP_STATUS_IS_SUCCESSFUL (status))
    {
      if (shumate_data_source_request_get_data (data->req))
        {
          g_debug ("Fetching tile failed, but there is a cached version (HTTP %s)",
                   soup_status_get_phrase (status));
          shumate_data_source_request_complete (data->req);
        }
      else
        {
          g_autoptr(GError) error =
            g_error_new (SHUMATE_TILE_DOWNLOADER_ERROR,
                         SHUMATE_TILE_DOWNLOADER_ERROR_BAD_RESPONSE,
                         "Unable to download tile: HTTP %s",
                         soup_status_get_phrase (status));
          shumate_data_source_request_emit_error (data->req, error);
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
                                data->cancellable,
                                on_message_read,
                                data);
  data = NULL;
}

static void
on_message_read (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  g_autoptr(FillTileData) data = user_data;
  GOutputStream *output_stream = G_OUTPUT_STREAM (source_object);
  g_autoptr(GError) error = NULL;
  g_autoptr (GBytes) bytes = NULL;
  int x = shumate_data_source_request_get_x (data->req);
  int y = shumate_data_source_request_get_y (data->req);
  int z = shumate_data_source_request_get_zoom_level (data->req);

  g_output_stream_splice_finish (output_stream, res, &error);
  if (error != NULL)
    {
      shumate_data_source_request_emit_error (data->req, error);
      return;
    }

  bytes = g_memory_output_stream_steal_as_bytes (G_MEMORY_OUTPUT_STREAM (output_stream));
  shumate_data_source_request_emit_data (data->req, bytes, TRUE);

  shumate_file_cache_store_tile_async (data->self->cache,
                                       x, y, z,
                                       bytes,
                                       data->etag,
                                       NULL,
                                       NULL,
                                       NULL);
}


/**
 * shumate_tile_downloader_error_quark:
 *
 * Gets the #ShumateTileDownloader error quark.
 *
 * Returns: a #GQuark
 */
G_DEFINE_QUARK (shumate-tile-downloader-error-quark, shumate_tile_downloader_error);
