/*
 * Copyright (C) 2008-2009 Pierre-Luc Beaudoin <pierre-luc@pierlux.com>
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
 * ShumateNetworkTileSource:
 *
 * Fetches raster (that is, image) tiles from a web API such as Mapbox or
 * OpenStreetMap. It has a built-in [class@FileCache] to avoid repeated
 * downloads.
 *
 * Some preconfigured network map sources are built-in this library,
 * see [class@MapSourceRegistry].
 */

#include "shumate-network-tile-source.h"

#include "shumate.h"
#include "shumate-enum-types.h"
#include "shumate-map-source.h"
#include "shumate-marshal.h"

#include <errno.h>
#include <gdk/gdk.h>
#include <gio/gio.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <glib-object.h>
#include <libsoup/soup.h>
#include <math.h>
#include <sys/stat.h>
#include <string.h>

enum
{
  PROP_0,
  PROP_URI_FORMAT,
  PROP_OFFLINE,
  PROP_PROXY_URI,
  PROP_MAX_CONNS,
  PROP_USER_AGENT,
  PROP_FILE_CACHE,
  N_PROPERTIES,
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

typedef struct
{
  gboolean offline;
  char *uri_format;
  char *proxy_uri;
  SoupSession *soup_session;
  int max_conns;
  ShumateFileCache *file_cache;
} ShumateNetworkTileSourcePrivate;

G_DEFINE_TYPE_WITH_PRIVATE (ShumateNetworkTileSource, shumate_network_tile_source, SHUMATE_TYPE_MAP_SOURCE);

/* The osm.org tile set require us to use no more than 2 simultaneous
 * connections so let that be the default.
 */
#define MAX_CONNS_DEFAULT 2


static void fill_tile_async (ShumateMapSource *map_source,
                             ShumateTile *tile,
                             GCancellable *cancellable,
                             GAsyncReadyCallback callback,
                             gpointer user_data);

static char *get_tile_uri (ShumateNetworkTileSource *source,
    int x,
    int y,
    int z);


static void
shumate_network_tile_source_constructed (GObject *object)
{
  ShumateNetworkTileSource *self = SHUMATE_NETWORK_TILE_SOURCE (object);
  ShumateNetworkTileSourcePrivate *priv = shumate_network_tile_source_get_instance_private (self);
  const char *id = shumate_map_source_get_id (SHUMATE_MAP_SOURCE (self));

  priv->file_cache = shumate_file_cache_new_full (100000000, id, NULL);
  g_object_notify_by_pspec (object, obj_properties[PROP_FILE_CACHE]);

  G_OBJECT_CLASS (shumate_network_tile_source_parent_class)->constructed (object);
}


static void
shumate_network_tile_source_get_property (GObject *object,
    guint prop_id,
    GValue *value,
    GParamSpec *pspec)
{
  ShumateNetworkTileSource *tile_source  = SHUMATE_NETWORK_TILE_SOURCE (object);
  ShumateNetworkTileSourcePrivate *priv = shumate_network_tile_source_get_instance_private (tile_source);

  switch (prop_id)
    {
    case PROP_URI_FORMAT:
      g_value_set_string (value, priv->uri_format);
      break;

    case PROP_OFFLINE:
      g_value_set_boolean (value, priv->offline);
      break;

    case PROP_PROXY_URI:
      g_value_set_string (value, priv->proxy_uri);
      break;

    case PROP_MAX_CONNS:
      g_value_set_int (value, priv->max_conns);
      break;

    case PROP_FILE_CACHE:
      g_value_set_object (value, priv->file_cache);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}


static void
shumate_network_tile_source_set_property (GObject *object,
    guint prop_id,
    const GValue *value,
    GParamSpec *pspec)
{
  ShumateNetworkTileSource *tile_source = SHUMATE_NETWORK_TILE_SOURCE (object);

  switch (prop_id)
    {
    case PROP_URI_FORMAT:
      shumate_network_tile_source_set_uri_format (tile_source, g_value_get_string (value));
      break;

    case PROP_OFFLINE:
      shumate_network_tile_source_set_offline (tile_source, g_value_get_boolean (value));
      break;

    case PROP_PROXY_URI:
      shumate_network_tile_source_set_proxy_uri (tile_source, g_value_get_string (value));
      break;

    case PROP_MAX_CONNS:
      shumate_network_tile_source_set_max_conns (tile_source, g_value_get_int (value));
      break;

    case PROP_USER_AGENT:
      shumate_network_tile_source_set_user_agent (tile_source, g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}


static void
shumate_network_tile_source_dispose (GObject *object)
{
  ShumateNetworkTileSource *tile_source  = SHUMATE_NETWORK_TILE_SOURCE (object);
  ShumateNetworkTileSourcePrivate *priv = shumate_network_tile_source_get_instance_private (tile_source);

  if (priv->soup_session)
    {
      soup_session_abort (priv->soup_session);
      g_clear_object (&priv->soup_session);
    }

  G_OBJECT_CLASS (shumate_network_tile_source_parent_class)->dispose (object);
}


static void
shumate_network_tile_source_finalize (GObject *object)
{
  ShumateNetworkTileSource *tile_source  = SHUMATE_NETWORK_TILE_SOURCE (object);
  ShumateNetworkTileSourcePrivate *priv = shumate_network_tile_source_get_instance_private (tile_source);

  g_clear_pointer (&priv->uri_format, g_free);
  g_clear_pointer (&priv->proxy_uri, g_free);
  g_clear_object (&priv->file_cache);

  G_OBJECT_CLASS (shumate_network_tile_source_parent_class)->finalize (object);
}

static void
shumate_network_tile_source_class_init (ShumateNetworkTileSourceClass *klass)
{
  ShumateMapSourceClass *map_source_class = SHUMATE_MAP_SOURCE_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = shumate_network_tile_source_constructed;
  object_class->finalize = shumate_network_tile_source_finalize;
  object_class->dispose = shumate_network_tile_source_dispose;
  object_class->get_property = shumate_network_tile_source_get_property;
  object_class->set_property = shumate_network_tile_source_set_property;

  map_source_class->fill_tile_async = fill_tile_async;

  /**
   * ShumateNetworkTileSource:uri-format:
   *
   * The uri format of the tile source, see #shumate_network_tile_source_set_uri_format
   */
  obj_properties[PROP_URI_FORMAT] =
    g_param_spec_string ("uri-format",
                         "URI Format",
                         "The URI format",
                         "",
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * ShumateNetworkTileSource:offline:
   *
   * Specifies whether the network tile source can access network
   */
  obj_properties[PROP_OFFLINE] =
    g_param_spec_boolean ("offline",
                          "Offline",
                          "Offline",
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * ShumateNetworkTileSource:proxy-uri:
   *
   * Used to override the default proxy for accessing the network.
   */
  obj_properties[PROP_PROXY_URI] =
    g_param_spec_string ("proxy-uri",
                         "Proxy URI",
                         "The proxy URI to use to access network",
                         "",
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * ShumateNetworkTileSource:max-conns:
   *
   * Specifies the max number of allowed simultaneous connections for this tile
   * source.
   *
   * Before changing this remember to verify how many simultaneous connections
   * your tile provider allows you to make.
   */
  obj_properties[PROP_MAX_CONNS] =
    g_param_spec_int ("max-conns",
                      "Max Connection Count",
                      "The maximum number of allowed simultaneous connections "
                      "for this tile source.",
                      1,
                      G_MAXINT,
                      MAX_CONNS_DEFAULT,
                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * ShumateNetworkTileSource:user-agent:
   *
   * The HTTP user agent used for requests
   */
  obj_properties[PROP_USER_AGENT] =
    g_param_spec_string ("user-agent",
                         "HTTP User Agent",
                         "The HTTP user agent used for network requests",
                         "libshumate/" SHUMATE_VERSION,
                         G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * ShumateNetworkTileSource:file-cache:
   *
   * The cache where downloaded tiles are stored.
   */
  obj_properties[PROP_FILE_CACHE] =
    g_param_spec_object("file-cache",
                        "File Cache",
                        "Cache for storing tile data",
                        SHUMATE_TYPE_FILE_CACHE,
                        G_PARAM_READABLE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, N_PROPERTIES, obj_properties);
}


static void
shumate_network_tile_source_init (ShumateNetworkTileSource *tile_source)
{
  ShumateNetworkTileSourcePrivate *priv = shumate_network_tile_source_get_instance_private (tile_source);

  priv->proxy_uri = NULL;
  priv->uri_format = NULL;
  priv->offline = FALSE;
  priv->max_conns = MAX_CONNS_DEFAULT;

  priv->soup_session = soup_session_new_with_options (
        "proxy-uri", NULL,
        "ssl-strict", FALSE,
        SOUP_SESSION_ADD_FEATURE_BY_TYPE,
        SOUP_TYPE_PROXY_RESOLVER_DEFAULT,
        SOUP_SESSION_ADD_FEATURE_BY_TYPE,
        SOUP_TYPE_CONTENT_DECODER,
        NULL);
  g_object_set (G_OBJECT (priv->soup_session),
      "user-agent",
      "libshumate/" SHUMATE_VERSION,
      "max-conns-per-host", MAX_CONNS_DEFAULT,
      "max-conns", MAX_CONNS_DEFAULT,
      NULL);
}


/**
 * shumate_network_tile_source_new_full:
 * @id: the map source's id
 * @name: the map source's name
 * @license: the map source's license
 * @license_uri: the map source's license URI
 * @min_zoom: the map source's minimum zoom level
 * @max_zoom: the map source's maximum zoom level
 * @tile_size: the map source's tile size (in pixels)
 * @projection: the map source's projection
 * @uri_format: the URI to fetch the tiles from, see #shumate_network_tile_source_set_uri_format
 *
 * Constructor of #ShumateNetworkTileSource.
 *
 * Returns: a constructed #ShumateNetworkTileSource object
 */
ShumateNetworkTileSource *
shumate_network_tile_source_new_full (const char *id,
    const char *name,
    const char *license,
    const char *license_uri,
    guint min_zoom,
    guint max_zoom,
    guint tile_size,
    ShumateMapProjection projection,
    const char *uri_format)
{
  ShumateNetworkTileSource *source;

  source = g_object_new (SHUMATE_TYPE_NETWORK_TILE_SOURCE,
        "id", id,
        "name", name,
        "license", license,
        "license-uri", license_uri,
        "min-zoom-level", min_zoom,
        "max-zoom-level", max_zoom,
        "tile-size", tile_size,
        "projection", projection,
        "uri-format", uri_format,
        NULL);
  return source;
}


/**
 * shumate_network_tile_source_get_uri_format:
 * @tile_source: the #ShumateNetworkTileSource
 *
 * Default constructor of #ShumateNetworkTileSource.
 *
 * Returns: A URI format used for URI creation when downloading tiles. See
 * [method@NetworkTileSource.set_uri_format] for more information.
 */
const char *
shumate_network_tile_source_get_uri_format (ShumateNetworkTileSource *tile_source)
{
  ShumateNetworkTileSourcePrivate *priv = shumate_network_tile_source_get_instance_private (tile_source);

  g_return_val_if_fail (SHUMATE_IS_NETWORK_TILE_SOURCE (tile_source), NULL);

  return priv->uri_format;
}


/**
 * shumate_network_tile_source_set_uri_format:
 * @tile_source: the #ShumateNetworkTileSource
 * @uri_format: the URI format
 *
 * A URI format is a URI where x, y and zoom level information have been
 * marked for parsing and insertion.  There can be an unlimited number of
 * marked items in a URI format.  They are delimited by "#" before and after
 * the variable name. There are 4 defined variable names: X, Y, Z, and TMSY for
 * Y in TMS coordinates.
 *
 * For example, this is the OpenStreetMap URI format:
 * "http://tile.openstreetmap.org/\#Z\#/\#X\#/\#Y\#.png"
 */
void
shumate_network_tile_source_set_uri_format (ShumateNetworkTileSource *tile_source,
    const char *uri_format)
{
  ShumateNetworkTileSourcePrivate *priv = shumate_network_tile_source_get_instance_private (tile_source);

  g_return_if_fail (SHUMATE_IS_NETWORK_TILE_SOURCE (tile_source));

  g_free (priv->uri_format);
  priv->uri_format = g_strdup (uri_format);

  g_object_notify_by_pspec (G_OBJECT (tile_source), obj_properties[PROP_URI_FORMAT]);
}


/**
 * shumate_network_tile_source_get_proxy_uri:
 * @tile_source: the #ShumateNetworkTileSource
 *
 * Gets the proxy uri used to access network.
 *
 * Returns: the proxy uri
 */
const char *
shumate_network_tile_source_get_proxy_uri (ShumateNetworkTileSource *tile_source)
{
  ShumateNetworkTileSourcePrivate *priv = shumate_network_tile_source_get_instance_private (tile_source);

  g_return_val_if_fail (SHUMATE_IS_NETWORK_TILE_SOURCE (tile_source), NULL);

  return priv->proxy_uri;
}


/**
 * shumate_network_tile_source_set_proxy_uri:
 * @tile_source: the #ShumateNetworkTileSource
 * @proxy_uri: the proxy uri used to access network
 *
 * Override the default proxy for accessing the network.
 */
void
shumate_network_tile_source_set_proxy_uri (ShumateNetworkTileSource *tile_source,
    const char *proxy_uri)
{
  ShumateNetworkTileSourcePrivate *priv = shumate_network_tile_source_get_instance_private (tile_source);

  g_return_if_fail (SHUMATE_IS_NETWORK_TILE_SOURCE (tile_source));

  SoupURI *uri = NULL;

  g_free (priv->proxy_uri);
  priv->proxy_uri = g_strdup (proxy_uri);

  if (priv->proxy_uri)
    uri = soup_uri_new (priv->proxy_uri);

  if (priv->soup_session)
    g_object_set (G_OBJECT (priv->soup_session),
        "proxy-uri", uri,
        NULL);

  if (uri)
    soup_uri_free (uri);

  g_object_notify_by_pspec (G_OBJECT (tile_source), obj_properties[PROP_PROXY_URI]);
}


/**
 * shumate_network_tile_source_get_offline:
 * @tile_source: the #ShumateNetworkTileSource
 *
 * Gets offline status.
 *
 * Returns: TRUE when the tile source is set to be offline; FALSE otherwise.
 */
gboolean
shumate_network_tile_source_get_offline (ShumateNetworkTileSource *tile_source)
{
  ShumateNetworkTileSourcePrivate *priv = shumate_network_tile_source_get_instance_private (tile_source);

  g_return_val_if_fail (SHUMATE_IS_NETWORK_TILE_SOURCE (tile_source), FALSE);

  return priv->offline;
}


/**
 * shumate_network_tile_source_set_offline:
 * @tile_source: the #ShumateNetworkTileSource
 * @offline: TRUE when the tile source should be offline; FALSE otherwise
 *
 * Sets offline status.
 */
void
shumate_network_tile_source_set_offline (ShumateNetworkTileSource *tile_source,
    gboolean offline)
{
  ShumateNetworkTileSourcePrivate *priv = shumate_network_tile_source_get_instance_private (tile_source);

  g_return_if_fail (SHUMATE_IS_NETWORK_TILE_SOURCE (tile_source));

  priv->offline = offline;

  g_object_notify_by_pspec (G_OBJECT (tile_source), obj_properties[PROP_OFFLINE]);
}


/**
 * shumate_network_tile_source_get_max_conns:
 * @tile_source: the #ShumateNetworkTileSource
 *
 * Gets the max number of allowed simultaneous connections for this tile
 * source.
 *
 * Returns: the max number of allowed simultaneous connections for this tile
 * source.
 */
int
shumate_network_tile_source_get_max_conns (ShumateNetworkTileSource *tile_source)
{
  ShumateNetworkTileSourcePrivate *priv = shumate_network_tile_source_get_instance_private (tile_source);

  g_return_val_if_fail (SHUMATE_IS_NETWORK_TILE_SOURCE (tile_source), 0);

  return priv->max_conns;
}


/**
 * shumate_network_tile_source_set_max_conns:
 * @tile_source: the #ShumateNetworkTileSource
 * @max_conns: the number of allowed simultaneous connections
 *
 * Sets the max number of allowed simultaneous connections for this tile source.
 *
 * Before changing this remember to verify how many simultaneous connections
 * your tile provider allows you to make.
 */
void
shumate_network_tile_source_set_max_conns (ShumateNetworkTileSource *tile_source,
    int max_conns)
{
  ShumateNetworkTileSourcePrivate *priv = shumate_network_tile_source_get_instance_private (tile_source);

  g_return_if_fail (SHUMATE_IS_NETWORK_TILE_SOURCE (tile_source));
  g_return_if_fail (SOUP_IS_SESSION (priv->soup_session));

  priv->max_conns = max_conns;

  g_object_set (G_OBJECT (priv->soup_session),
      "max-conns-per-host", max_conns,
      "max-conns", max_conns,
      NULL);

  g_object_notify_by_pspec (G_OBJECT (tile_source), obj_properties[PROP_MAX_CONNS]);
}

/**
 * shumate_network_tile_source_set_user_agent:
 * @tile_source: a #ShumateNetworkTileSource
 * @user_agent: A User-Agent string
 *
 * Sets the User-Agent header used communicating with the server.
 */
void
shumate_network_tile_source_set_user_agent (
    ShumateNetworkTileSource *tile_source,
    const char *user_agent)
{
  ShumateNetworkTileSourcePrivate *priv = shumate_network_tile_source_get_instance_private (tile_source);

  g_return_if_fail (SHUMATE_IS_NETWORK_TILE_SOURCE (tile_source)
      && user_agent != NULL);

  if (priv->soup_session)
    g_object_set (G_OBJECT (priv->soup_session), "user-agent",
        user_agent, NULL);

  g_object_notify_by_pspec (G_OBJECT (tile_source), obj_properties[PROP_USER_AGENT]);
}


#define SIZE 8
static char *
get_tile_uri (ShumateNetworkTileSource *tile_source,
    int x,
    int y,
    int z)
{
  ShumateNetworkTileSourcePrivate *priv = shumate_network_tile_source_get_instance_private (tile_source);

  char **tokens;
  char *token;
  GString *ret;
  int i = 0;

  tokens = g_strsplit (priv->uri_format, "#", 20);
  token = tokens[i];
  ret = g_string_sized_new (strlen (priv->uri_format));

  while (token != NULL)
    {
      int number = G_MAXINT;
      char value[SIZE];

      if (strcmp (token, "X") == 0)
        number = x;
      if (strcmp (token, "Y") == 0)
        number = y;
      if (strcmp (token, "TMSY") == 0){
        int ymax = 1 << z;
        number = ymax - y - 1;
      }
      if (strcmp (token, "Z") == 0)
        number = z;

      if (number != G_MAXINT)
        {
          g_snprintf (value, SIZE, "%d", number);
          g_string_append (ret, value);
        }
      else
        g_string_append (ret, token);

      token = tokens[++i];
    }

  token = ret->str;
  g_string_free (ret, FALSE);
  g_strfreev (tokens);

  return token;
}

static void on_file_cache_get_tile (GObject *source_object, GAsyncResult *res, gpointer user_data);
static void on_pixbuf_created_from_cache (GObject *source_object, GAsyncResult *res, gpointer user_data);
static void fetch_from_network (GTask *task);
static void on_message_sent (GObject *source_object, GAsyncResult *res, gpointer user_data);
static void on_message_read (GObject *source_object, GAsyncResult *res, gpointer user_data);
static void on_pixbuf_created (GObject *source_object, GAsyncResult *res, gpointer user_data);

typedef struct {
  ShumateNetworkTileSource *self;
  ShumateTile *tile;
  GBytes *bytes;
  char *etag;
  SoupMessage *msg;
  GDateTime *modtime;
} FillTileData;

static void
fill_tile_data_free (FillTileData *data)
{
  g_clear_object (&data->self);
  g_clear_object (&data->tile);
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


static void
fill_tile_async (ShumateMapSource *self,
                 ShumateTile *tile,
                 GCancellable *cancellable,
                 GAsyncReadyCallback callback,
                 gpointer user_data)
{
  g_autoptr(GTask) task = NULL;
  ShumateNetworkTileSource *tile_source = SHUMATE_NETWORK_TILE_SOURCE (self);
  ShumateNetworkTileSourcePrivate *priv = shumate_network_tile_source_get_instance_private (tile_source);
  FillTileData *data;

  g_return_if_fail (SHUMATE_IS_NETWORK_TILE_SOURCE (self));
  g_return_if_fail (SHUMATE_IS_TILE (tile));
  g_return_if_fail (!cancellable || G_IS_CANCELLABLE (cancellable));

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_source_tag (task, fill_tile_async);

  if (priv->offline)
    {
      g_task_return_new_error (task, SHUMATE_NETWORK_SOURCE_ERROR,
                               SHUMATE_NETWORK_SOURCE_ERROR_OFFLINE,
                               "The tile source is offline.");
      return;
    }

  data = g_new0 (FillTileData, 1);
  data->self = g_object_ref (tile_source);
  data->tile = g_object_ref (tile);
  g_task_set_task_data (task, data, (GDestroyNotify) fill_tile_data_free);

  shumate_file_cache_get_tile_async (priv->file_cache, tile, cancellable, on_file_cache_get_tile, g_object_ref (task));
}

/* If the cache returned data, parse it into a pixbuf, otherwise go straight
 * to fetching from the network */
static void
on_file_cache_get_tile (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  g_autoptr(GTask) task = user_data;
  FillTileData *data = g_task_get_task_data (task);
  GCancellable *cancellable = g_task_get_cancellable (task);

  data->bytes = shumate_file_cache_get_tile_finish (SHUMATE_FILE_CACHE (source_object),
                                                    &data->etag, &data->modtime, res, NULL);

  if (data->bytes != NULL)
    {
      g_autoptr(GInputStream) input_stream = g_memory_input_stream_new_from_bytes (data->bytes);

      /* When on_pixbuf_created_from_cache() is called, it will call
       * fetch_from_network() if needed */
      gdk_pixbuf_new_from_stream_async (input_stream, cancellable, on_pixbuf_created_from_cache, g_object_ref (task));
    }
  else
    {
      fetch_from_network (task);
    }
}

/* Fill the tile from the pixbuf, created from the cache. Then, if the cache
 * data is potentially out of date, fetch from the network. */
static void
on_pixbuf_created_from_cache (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  g_autoptr(GTask) task = user_data;
  FillTileData *data = g_task_get_task_data (task);
  g_autoptr(GdkTexture) texture = NULL;
  g_autoptr(GdkPixbuf) pixbuf = NULL;
  g_autoptr(GError) error = NULL;

  pixbuf = gdk_pixbuf_new_from_stream_finish (res, &error);

  if (error != NULL)
    {
      g_task_return_error (task, g_steal_pointer (&error));
      return;
    }

  texture = gdk_texture_new_for_pixbuf (pixbuf);
  shumate_tile_set_texture (data->tile, texture);
  shumate_tile_set_fade_in (data->tile, TRUE);

  if (data->bytes != NULL && !tile_is_expired (data->modtime))
    {
      shumate_tile_set_state (data->tile, SHUMATE_STATE_DONE);
      g_task_return_boolean (task, TRUE);
    }
  else
    fetch_from_network (task);
}

/* Fetch the tile from the network. */
static void
fetch_from_network (GTask *task)
{
  FillTileData *data = g_task_get_task_data (task);
  GCancellable *cancellable = g_task_get_cancellable (task);
  ShumateNetworkTileSourcePrivate *priv = shumate_network_tile_source_get_instance_private (data->self);
  g_autofree char *uri = NULL;
  g_autofree char *modtime_string = NULL;

  uri = get_tile_uri (data->self,
        shumate_tile_get_x (data->tile),
        shumate_tile_get_y (data->tile),
        shumate_tile_get_zoom_level (data->tile));

  data->msg = soup_message_new (SOUP_METHOD_GET, uri);

  if (data->msg == NULL)
    {
      g_task_return_new_error (task, SHUMATE_NETWORK_SOURCE_ERROR,
                               SHUMATE_NETWORK_SOURCE_ERROR_MALFORMED_URL,
                               "The URL %s is not valid", uri);
      return;
    }

  modtime_string = get_modified_time_string (data->modtime);

  /* If an etag is available, only use it.
   * OSM servers seems to send now as the modified time for all tiles
   * Omarender servers set the modified time correctly
   */
  if (data->etag)
    {
      g_debug ("If-None-Match: %s", data->etag);
      soup_message_headers_append (data->msg->request_headers,
          "If-None-Match", data->etag);
    }
  else if (modtime_string)
    {
      g_debug ("If-Modified-Since %s", modtime_string);
      soup_message_headers_append (data->msg->request_headers,
          "If-Modified-Since", modtime_string);
    }

  soup_session_send_async (priv->soup_session, data->msg, cancellable, on_message_sent, g_object_ref (task));
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
  ShumateNetworkTileSourcePrivate *priv = shumate_network_tile_source_get_instance_private (data->self);
  g_autoptr(GOutputStream) output_stream = NULL;

  input_stream = soup_session_send_finish (priv->soup_session, res, &error);
  if (error != NULL)
    {
      if (data->bytes)
        {
          /* The tile has already been filled from the cache, so the operation
           * was overall successful even though the network request failed. */
          g_debug ("Fetching tile failed, but there is a cached version (error: %s)", error->message);
          shumate_tile_set_state (data->tile, SHUMATE_STATE_DONE);
          g_task_return_boolean (task, TRUE);
        }
      else
        {
          g_task_return_error (task, g_steal_pointer (&error));
        }

      return;
    }

  g_debug ("Got reply %d", data->msg->status_code);

  if (data->msg->status_code == SOUP_STATUS_NOT_MODIFIED)
    {
      /* The tile has already been filled from the cache, and the server says
       * it doesn't have a newer one. Just update the cache, mark the tile as
       * DONE, and return. */

      shumate_file_cache_mark_up_to_date (priv->file_cache, data->tile);

      shumate_tile_set_state (data->tile, SHUMATE_STATE_DONE);
      g_task_return_boolean (task, TRUE);
      return;
    }

  if (!SOUP_STATUS_IS_SUCCESSFUL (data->msg->status_code))
    {
      if (data->bytes)
        {
          g_debug ("Fetching tile failed, but there is a cached version (HTTP %s)",
                   soup_status_get_phrase (data->msg->status_code));
          shumate_tile_set_state (data->tile, SHUMATE_STATE_DONE);
          g_task_return_boolean (task, TRUE);
        }
      else
        {
          g_task_return_new_error (task, SHUMATE_NETWORK_SOURCE_ERROR,
                                   SHUMATE_NETWORK_SOURCE_ERROR_BAD_RESPONSE,
                                   "Unable to download tile: HTTP %s",
                                   soup_status_get_phrase (data->msg->status_code));
        }

      return;
    }

  /* Verify if the server sent an etag and save it */
  g_clear_pointer (&data->etag, g_free);
  data->etag = g_strdup (soup_message_headers_get_one (data->msg->response_headers, "ETag"));
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

/* Parse the GBytes into a pixbuf. */
static void
on_message_read (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  GOutputStream *output_stream = G_OUTPUT_STREAM (source_object);
  g_autoptr(GTask) task = user_data;
  FillTileData *data = g_task_get_task_data (task);
  GCancellable *cancellable = g_task_get_cancellable (task);
  g_autoptr(GError) error = NULL;
  g_autoptr(GInputStream) input_stream = NULL;

  g_output_stream_splice_finish (output_stream, res, &error);
  if (error != NULL)
    {
      g_task_return_error (task, g_steal_pointer (&error));
      return;
    }

  g_bytes_unref (data->bytes);
  data->bytes = g_memory_output_stream_steal_as_bytes (G_MEMORY_OUTPUT_STREAM (output_stream));

  input_stream = g_memory_input_stream_new_from_bytes (data->bytes);
  gdk_pixbuf_new_from_stream_async (input_stream, cancellable, on_pixbuf_created, g_object_ref (task));
}

/* Fill the tile from the pixbuf, created from the network response. Begin
 * storing the data in the cache (but don't wait for that to finish). Then
 * return. */
static void
on_pixbuf_created (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  g_autoptr(GTask) task = user_data;
  FillTileData *data = g_task_get_task_data (task);
  ShumateNetworkTileSourcePrivate *priv = shumate_network_tile_source_get_instance_private (data->self);
  GCancellable *cancellable = g_task_get_cancellable (task);
  g_autoptr(GError) error = NULL;
  g_autoptr(GdkPixbuf) pixbuf = NULL;
  g_autoptr(GdkTexture) texture = NULL;

  pixbuf = gdk_pixbuf_new_from_stream_finish (res, &error);
  if (error != NULL)
    {
      g_task_return_error (task, g_steal_pointer (&error));
      return;
    }

  shumate_file_cache_store_tile_async (priv->file_cache, data->tile, data->bytes, data->etag, cancellable, NULL, NULL);

  texture = gdk_texture_new_for_pixbuf (pixbuf);
  shumate_tile_set_texture (data->tile, texture);
  shumate_tile_set_fade_in (data->tile, TRUE);

  shumate_tile_set_state (data->tile, SHUMATE_STATE_DONE);
  g_task_return_boolean (task, TRUE);
}


/**
 * shumate_network_source_error_quark:
 *
 * Gets the #ShumateNetworkTileSource error quark.
 *
 * Returns: a #GQuark
 */
G_DEFINE_QUARK (shumate-network-source-error-quark, shumate_network_source_error);
