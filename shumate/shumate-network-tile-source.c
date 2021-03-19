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
 * SECTION:shumate-network-tile-source
 * @short_description: A map source that downloads tile data from a web server
 *
 * #ShumateNetworkTileSource fetches raster (that is, image) tiles from a
 * web API such as Mapbox or OpenStreetMap. It has a built-in #ShumateFileCache
 * to avoid repeated downloads.
 *
 * Some preconfigured network map sources are built-in this library,
 * see #ShumateMapSourceFactory.
 */

#include "config.h"

#include "shumate-network-tile-source.h"

#define DEBUG_FLAG SHUMATE_DEBUG_LOADING
#include "shumate-debug.h"

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
};

typedef struct
{
  gboolean offline;
  char *uri_format;
  char *proxy_uri;
  SoupSession *soup_session;
  int max_conns;
  ShumateFileCache *file_cache;
} ShumateNetworkTileSourcePrivate;

G_DEFINE_TYPE_WITH_PRIVATE (ShumateNetworkTileSource, shumate_network_tile_source, SHUMATE_TYPE_TILE_SOURCE);

/* The osm.org tile set require us to use no more than 2 simultaneous
 * connections so let that be the default.
 */
#define MAX_CONNS_DEFAULT 2

typedef struct
{
  ShumateNetworkTileSource *self;
  GCancellable *cancellable;
  ShumateTile *tile;
  SoupMessage *msg;
  GBytes *cached_data;
} TileLoadedData;

typedef struct
{
  ShumateNetworkTileSource *self;
  GCancellable *cancellable;
  ShumateTile *tile;
  char *etag;
} TileRenderedData;


static void fill_tile (ShumateMapSource *map_source,
                       ShumateTile      *tile,
                       GCancellable     *cancellable);
static void tile_state_notify (ShumateTile *tile,
    G_GNUC_UNUSED GParamSpec *pspec,
    GCancellable *cancellable);

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
  g_object_notify (object, "file-cache");

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
  GParamSpec *pspec;

  object_class->constructed = shumate_network_tile_source_constructed;
  object_class->finalize = shumate_network_tile_source_finalize;
  object_class->dispose = shumate_network_tile_source_dispose;
  object_class->get_property = shumate_network_tile_source_get_property;
  object_class->set_property = shumate_network_tile_source_set_property;

  map_source_class->fill_tile = fill_tile;

  /**
   * ShumateNetworkTileSource:uri-format:
   *
   * The uri format of the tile source, see #shumate_network_tile_source_set_uri_format
   */
  pspec = g_param_spec_string ("uri-format",
        "URI Format",
        "The URI format",
        "",
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_URI_FORMAT, pspec);

  /**
   * ShumateNetworkTileSource:offline:
   *
   * Specifies whether the network tile source can access network
   */
  pspec = g_param_spec_boolean ("offline",
        "Offline",
        "Offline",
        FALSE,
        G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_OFFLINE, pspec);

  /**
   * ShumateNetworkTileSource:proxy-uri:
   *
   * Used to override the default proxy for accessing the network.
   */
  pspec = g_param_spec_string ("proxy-uri",
        "Proxy URI",
        "The proxy URI to use to access network",
        "",
        G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_PROXY_URI, pspec);

  /**
   * ShumateNetworkTileSource:max-conns:
   *
   * Specifies the max number of allowed simultaneous connections for this tile
   * source.
   *
   * Before changing this remember to verify how many simultaneous connections
   * your tile provider allows you to make.
   */
  pspec = g_param_spec_int ("max-conns",
        "Max Connection Count",
        "The maximum number of allowed simultaneous connections "
        "for this tile source.",
        1,
        G_MAXINT,
        MAX_CONNS_DEFAULT,
        G_PARAM_READWRITE);

  g_object_class_install_property (object_class, PROP_MAX_CONNS, pspec);

  /**
   * ShumateNetworkTileSource:user-agent:
   *
   * The HTTP user agent used for requests
   */
  pspec = g_param_spec_string ("user-agent",
        "HTTP User Agent",
        "The HTTP user agent used for network requests",
        "libshumate/" SHUMATE_VERSION_S,
        G_PARAM_WRITABLE);

  g_object_class_install_property (object_class, PROP_USER_AGENT, pspec);

  /**
   * ShumateNetworkTileSource:file-cache:
   *
   * The cache where downloaded tiles are stored.
   */
  pspec = g_param_spec_object("file-cache",
                              "File Cache",
                              "Cache for storing tile data",
                              SHUMATE_TYPE_FILE_CACHE,
                              G_PARAM_READABLE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);
  g_object_class_install_property (object_class, PROP_FILE_CACHE, pspec);
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
      "libshumate/" SHUMATE_VERSION_S,
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
 * shumate_network_tile_source_set_uri_format() for more information.
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

  g_object_notify (G_OBJECT (tile_source), "uri-format");
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

  g_object_notify (G_OBJECT (tile_source), "proxy-uri");
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

  g_object_notify (G_OBJECT (tile_source), "offline");
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

  g_object_notify (G_OBJECT (tile_source), "max_conns");
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


static void
on_pixbuf_created (GObject      *source_object,
                   GAsyncResult *res,
                   gpointer      user_data)
{
  TileRenderedData *rendered_data = (TileRenderedData *) user_data;
  g_autoptr(ShumateNetworkTileSource) self = g_steal_pointer (&rendered_data->self);
  ShumateNetworkTileSourcePrivate *priv = shumate_network_tile_source_get_instance_private (self);
  g_autoptr(GCancellable) cancellable = g_steal_pointer (&rendered_data->cancellable);
  g_autoptr(ShumateTile) tile = g_steal_pointer (&rendered_data->tile);
  g_autoptr(GError) error = NULL;
  g_autoptr(GdkPixbuf) pixbuf = NULL;
  g_autoptr(GdkTexture) texture = NULL;
  g_autofree char *etag = g_steal_pointer (&rendered_data->etag);
  ShumateTileCache *tile_cache = shumate_tile_source_get_cache (SHUMATE_TILE_SOURCE (self));

  g_slice_free (TileRenderedData, rendered_data);
  g_signal_handlers_disconnect_by_func (tile, tile_state_notify, cancellable);

  pixbuf = gdk_pixbuf_new_from_stream_finish (res, &error);
  if (!pixbuf)
    {
      ShumateMapSource *next_source = shumate_map_source_get_next_source (SHUMATE_MAP_SOURCE (self));
      if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        {
          DEBUG ("Download of tile %d, %d got cancelled",
              shumate_tile_get_x (tile), shumate_tile_get_y (tile));
          return;
        }

      if (next_source)
        shumate_map_source_fill_tile (next_source, tile, cancellable);

      return;
    }


  if (etag != NULL)
    shumate_tile_set_etag (tile, etag);

  g_autofree char *buffer = NULL;
  gsize buffer_size;
  if (!gdk_pixbuf_save_to_buffer (pixbuf, &buffer, &buffer_size, "png", &error, NULL))
    g_warning ("Unable to export tile: %s", error->message);
  else
    {
      g_autoptr(GBytes) bytes = NULL;

      if (tile_cache)
        shumate_tile_cache_store_tile (tile_cache, tile, buffer, buffer_size);

      bytes = g_bytes_new_take (buffer, buffer_size);
      buffer = NULL;
      shumate_file_cache_store_tile_async (priv->file_cache, tile, bytes, etag, cancellable, NULL, NULL);
    }

  texture = gdk_texture_new_for_pixbuf (pixbuf);
  shumate_tile_set_texture (tile, texture);
  shumate_tile_set_fade_in (tile, TRUE);
  shumate_tile_set_state (tile, SHUMATE_STATE_DONE);
}

static void
on_pixbuf_created_from_cache (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  g_autoptr(ShumateTile) tile = SHUMATE_TILE (user_data);
  g_autoptr(GdkTexture) texture = NULL;
  g_autoptr(GdkPixbuf) pixbuf = NULL;

  pixbuf = gdk_pixbuf_new_from_stream_finish (res, NULL);

  if (pixbuf == NULL)
    return;

  texture = gdk_texture_new_for_pixbuf (pixbuf);
  shumate_tile_set_texture (tile, texture);
  shumate_tile_set_fade_in (tile, TRUE);
  shumate_tile_set_state (tile, SHUMATE_STATE_DONE);
}

static void
on_message_sent (GObject *source_object,
                 GAsyncResult *res,
                 gpointer user_data)
{
  TileLoadedData *callback_data = (TileLoadedData *) user_data;
  g_autoptr(SoupMessage) msg = g_steal_pointer (&callback_data->msg);
  g_autoptr(ShumateNetworkTileSource) self = g_steal_pointer (&callback_data->self);
  g_autoptr(GCancellable) cancellable = g_steal_pointer (&callback_data->cancellable);
  g_autoptr(ShumateTile) tile = g_steal_pointer (&callback_data->tile);
  g_autoptr(GBytes) cached_data = g_steal_pointer (&callback_data->cached_data);
  g_autoptr(GInputStream) input_stream = NULL;
  g_autoptr(GError) error = NULL;
  ShumateNetworkTileSourcePrivate *priv = shumate_network_tile_source_get_instance_private (self);
  ShumateTileSource *tile_source = SHUMATE_TILE_SOURCE (self);
  ShumateTileCache *tile_cache = shumate_tile_source_get_cache (tile_source);
  ShumateMapSource *next_source = shumate_map_source_get_next_source (SHUMATE_MAP_SOURCE (self));
  const char *etag;
  TileRenderedData *data;

  g_slice_free (TileLoadedData, callback_data);

  input_stream = soup_session_send_finish (priv->soup_session, res, &error);
  if (!input_stream)
    {
      if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        {
          DEBUG ("Download of tile %d, %d got cancelled",
              shumate_tile_get_x (tile), shumate_tile_get_y (tile));

          g_signal_handlers_disconnect_by_func (tile, tile_state_notify, cancellable);
          return;
        }
    }
  
  DEBUG ("Got reply %d", msg->status_code);

  if (msg->status_code == SOUP_STATUS_NOT_MODIFIED)
    {
      g_autoptr(GInputStream) cache_stream = NULL;

      if (tile_cache)
        shumate_tile_cache_refresh_tile_time (tile_cache, tile);

      shumate_file_cache_mark_up_to_date (priv->file_cache, tile);

      cache_stream = g_memory_input_stream_new_from_bytes (cached_data);
      gdk_pixbuf_new_from_stream_async (cache_stream, cancellable, on_pixbuf_created_from_cache, g_object_ref (tile));

      g_signal_handlers_disconnect_by_func (tile, tile_state_notify, cancellable);
      return;
    }

  if (!SOUP_STATUS_IS_SUCCESSFUL (msg->status_code))
    {
      DEBUG ("Unable to download tile %d, %d: %s",
          shumate_tile_get_x (tile),
          shumate_tile_get_y (tile),
          soup_status_get_phrase (msg->status_code));

      g_signal_handlers_disconnect_by_func (tile, tile_state_notify, cancellable);
      if (next_source)
        shumate_map_source_fill_tile (next_source, tile, cancellable);

      return;
    }

  /* Verify if the server sent an etag and save it */
  etag = soup_message_headers_get_one (msg->response_headers, "ETag");
  DEBUG ("Received ETag %s", etag);

  data = g_slice_new0 (TileRenderedData);
  data->self = g_object_ref (self);
  if (data->cancellable)
    data->cancellable = g_object_ref (cancellable);
  data->tile = g_object_ref (tile);
  data->etag = g_strdup (etag);

  gdk_pixbuf_new_from_stream_async (input_stream, cancellable, on_pixbuf_created, data);
}

static void
tile_state_notify (ShumateTile *tile,
    G_GNUC_UNUSED GParamSpec *pspec,
    GCancellable *cancellable)
{
  if (shumate_tile_get_state (tile) == SHUMATE_STATE_DONE)
    {
      DEBUG ("Canceling tile download");
      g_cancellable_cancel (cancellable);
    }
}


static char *
get_modified_time_string (ShumateTile *tile)
{
  GDateTime *modified_time;

  g_return_val_if_fail (SHUMATE_TILE (tile), NULL);

  modified_time = shumate_tile_get_modified_time (tile);
  if (modified_time == NULL)
    return NULL;

  return g_date_time_format (modified_time, "%a, %d %b %Y %T %Z");
}


typedef struct {
  ShumateNetworkTileSource *self;
  ShumateTile *tile;
  GCancellable *cancellable;
} FillTileData;

static void
fill_tile_data_free (FillTileData *data)
{
  g_clear_object (&data->self);
  g_clear_object (&data->tile);
  g_clear_object (&data->cancellable);
  g_free (data);
}
G_DEFINE_AUTOPTR_CLEANUP_FUNC (FillTileData, fill_tile_data_free);

static void on_file_cache_get_tile (GObject *source_object, GAsyncResult *res, gpointer user_data);

static void
fill_tile (ShumateMapSource *map_source,
           ShumateTile      *tile,
           GCancellable     *cancellable)
{
  ShumateNetworkTileSource *tile_source = SHUMATE_NETWORK_TILE_SOURCE (map_source);
  ShumateNetworkTileSourcePrivate *priv = shumate_network_tile_source_get_instance_private (tile_source);
  FillTileData *data;

  g_return_if_fail (SHUMATE_IS_NETWORK_TILE_SOURCE (map_source));
  g_return_if_fail (SHUMATE_IS_TILE (tile));

  if (shumate_tile_get_state (tile) == SHUMATE_STATE_DONE)
    return;

  if (priv->offline)
    {
      ShumateMapSource *next_source = shumate_map_source_get_next_source (map_source);

      if (SHUMATE_IS_MAP_SOURCE (next_source))
        shumate_map_source_fill_tile (next_source, tile, cancellable);

      return;
    }

  data = g_new0 (FillTileData, 1);
  data->self = g_object_ref (tile_source);
  data->tile = g_object_ref (tile);
  data->cancellable = g_object_ref (cancellable);

  shumate_file_cache_get_tile_async (priv->file_cache, tile, cancellable, on_file_cache_get_tile, data);
}

static void
on_file_cache_get_tile (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  g_autoptr(FillTileData) data = user_data;
  ShumateNetworkTileSourcePrivate *priv = shumate_network_tile_source_get_instance_private (data->self);
  TileLoadedData *callback_data;
  g_autofree char *uri = NULL;
  g_autofree char *etag = NULL;
  g_autoptr(GBytes) bytes = NULL;

  bytes = shumate_file_cache_get_tile_finish (SHUMATE_FILE_CACHE (source_object), &etag, res, NULL);

  if (bytes)
    {
      g_autoptr(GInputStream) input_stream = NULL;
      ShumateTileCache *tile_cache = shumate_tile_source_get_cache (SHUMATE_TILE_SOURCE (data->self));

      if (tile_cache)
        {
          gsize size;
          gconstpointer buffer = g_bytes_get_data (bytes, &size);
          shumate_tile_cache_store_tile (tile_cache, data->tile, buffer, size);
        }

      if (etag == NULL)
        {
          /* No need to fetch new data from the network. Just fill the tile
           * directly from the cache. */
          input_stream = g_memory_input_stream_new_from_bytes (bytes);
          gdk_pixbuf_new_from_stream_async (input_stream, data->cancellable, on_pixbuf_created_from_cache, g_object_ref (data->tile));
          return;
        }
    }

  uri = get_tile_uri (data->self,
        shumate_tile_get_x (data->tile),
        shumate_tile_get_y (data->tile),
        shumate_tile_get_zoom_level (data->tile));

  callback_data = g_slice_new0 (TileLoadedData);
  callback_data->tile = g_object_ref (data->tile);
  callback_data->self = g_object_ref (data->self);
  if (data->cancellable)
    callback_data->cancellable = g_object_ref (data->cancellable);
  if (bytes)
    callback_data->cached_data = g_bytes_ref (bytes);
  callback_data->msg = soup_message_new (SOUP_METHOD_GET, uri);

  if (etag || shumate_tile_get_state (data->tile) == SHUMATE_STATE_LOADED)
    {
      char *date = get_modified_time_string (data->tile);

      /* If an etag is available, only use it.
       * OSM servers seems to send now as the modified time for all tiles
       * Omarender servers set the modified time correctly
       */
      if (etag)
        {
          DEBUG ("If-None-Match: %s", etag);
          soup_message_headers_append (callback_data->msg->request_headers,
              "If-None-Match", etag);
        }
      else if (date)
        {
          DEBUG ("If-Modified-Since %s", date);
          soup_message_headers_append (callback_data->msg->request_headers,
              "If-Modified-Since", date);
        }

      g_free (date);
    }

  g_signal_connect_object (data->tile, "notify::state", G_CALLBACK (tile_state_notify), data->cancellable, 0);
  soup_session_send_async (priv->soup_session, callback_data->msg, data->cancellable, on_message_sent, callback_data);
}
