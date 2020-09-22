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
 * SECTION:shumate-map-source-factory
 * @short_description: Manages #ShumateMapSource instances
 *
 * This factory manages the create of #ShumateMapSource. It contains names
 * and constructor functions for each available map sources in libshumate.
 * You can add your own with #shumate_map_source_factory_register.
 *
 * To get the wanted map source, use #shumate_map_source_factory_create. It
 * will return a ready to use #ShumateMapSource.
 *
 * To get the list of registered map sources, use
 * #shumate_map_source_factory_get_registered.
 *
 */
#include "config.h"

#include "shumate-map-source-factory.h"

#define DEBUG_FLAG SHUMATE_DEBUG_NETWORK
#include "shumate-debug.h"

#include "shumate.h"
#include "shumate-file-cache.h"
#include "shumate-enum-types.h"
#include "shumate-map-source.h"
#include "shumate-marshal.h"
#include "shumate-network-tile-source.h"
#include "shumate-map-source-chain.h"

#include <glib.h>
#include <string.h>

static ShumateMapSourceFactory *instance = NULL;

struct _ShumateMapSourceFactory
{
  GObject parent_instance;

  GSList *registered_sources;
};

G_DEFINE_TYPE (ShumateMapSourceFactory, shumate_map_source_factory, G_TYPE_OBJECT);

static ShumateMapSource *shumate_map_source_new_generic (
    ShumateMapSourceDesc *desc);


static void
shumate_map_source_factory_finalize (GObject *object)
{
  ShumateMapSourceFactory *factory = SHUMATE_MAP_SOURCE_FACTORY (object);

  g_clear_pointer (&factory->registered_sources, g_slist_free);

  G_OBJECT_CLASS (shumate_map_source_factory_parent_class)->finalize (object);
}


static GObject *
shumate_map_source_factory_constructor (GType type,
    guint n_construct_params,
    GObjectConstructParam *construct_params)
{
  GObject *retval;

  if (instance == NULL)
    {
      retval = G_OBJECT_CLASS (shumate_map_source_factory_parent_class)->constructor
          (type, n_construct_params, construct_params);

      instance = SHUMATE_MAP_SOURCE_FACTORY (retval);
      g_object_add_weak_pointer (retval, (gpointer *) &instance);
    }
  else
    {
      retval = g_object_ref (G_OBJECT (instance));
    }

  return retval;
}


static void
shumate_map_source_factory_class_init (ShumateMapSourceFactoryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructor = shumate_map_source_factory_constructor;
  object_class->finalize = shumate_map_source_factory_finalize;
}


static void
shumate_map_source_factory_init (ShumateMapSourceFactory *factory)
{
  ShumateMapSourceDesc *desc;

  factory->registered_sources = NULL;

  desc = shumate_map_source_desc_new_full (
        SHUMATE_MAP_SOURCE_OSM_MAPNIK,
        "OpenStreetMap Mapnik",
        "Map Data ODBL OpenStreetMap Contributors, Map Imagery CC-BY-SA 2.0 OpenStreetMap",
        "http://creativecommons.org/licenses/by-sa/2.0/",
        0,
        18,
        256,
        SHUMATE_MAP_PROJECTION_MERCATOR,
        "https://tile.openstreetmap.org/#Z#/#X#/#Y#.png",
        shumate_map_source_new_generic,
        NULL);
  shumate_map_source_factory_register (factory, desc);

  desc = shumate_map_source_desc_new_full (
        SHUMATE_MAP_SOURCE_OSM_CYCLE_MAP,
        "OpenStreetMap Cycle Map",
        "Map data is CC-BY-SA 2.0 OpenStreetMap contributors",
        "http://creativecommons.org/licenses/by-sa/2.0/",
        0,
        18,
        256,
        SHUMATE_MAP_PROJECTION_MERCATOR,
        "http://tile.opencyclemap.org/cycle/#Z#/#X#/#Y#.png",
        shumate_map_source_new_generic,
        NULL);
  shumate_map_source_factory_register (factory, desc);

  desc = shumate_map_source_desc_new_full (
        SHUMATE_MAP_SOURCE_OSM_TRANSPORT_MAP,
        "OpenStreetMap Transport Map",
        "Map data is CC-BY-SA 2.0 OpenStreetMap contributors",
        "http://creativecommons.org/licenses/by-sa/2.0/",
        0,
        18,
        256,
        SHUMATE_MAP_PROJECTION_MERCATOR,
        "http://tile.xn--pnvkarte-m4a.de/tilegen/#Z#/#X#/#Y#.png",
        shumate_map_source_new_generic,
        NULL);
  shumate_map_source_factory_register (factory, desc);

  desc = shumate_map_source_desc_new_full (
        SHUMATE_MAP_SOURCE_MFF_RELIEF,
        "Maps for Free Relief",
        "Map data available under GNU Free Documentation license, Version 1.2 or later",
        "http://www.gnu.org/copyleft/fdl.html",
        0,
        11,
        256,
        SHUMATE_MAP_PROJECTION_MERCATOR,
        "http://maps-for-free.com/layer/relief/z#Z#/row#Y#/#Z#_#X#-#Y#.jpg",
        shumate_map_source_new_generic,
        NULL);
  shumate_map_source_factory_register (factory, desc);

  desc = shumate_map_source_desc_new_full (
        SHUMATE_MAP_SOURCE_OWM_CLOUDS,
        "OpenWeatherMap cloud layer",
        "Map data is CC-BY-SA 2.0 OpenWeatherMap contributors",
        "http://creativecommons.org/licenses/by-sa/2.0/",
        0,
        18,
        256,
        SHUMATE_MAP_PROJECTION_MERCATOR,
        "http://tile.openweathermap.org/map/clouds/#Z#/#X#/#Y#.png",
        shumate_map_source_new_generic,
        NULL);
  shumate_map_source_factory_register (factory, desc);

  desc = shumate_map_source_desc_new_full (
        SHUMATE_MAP_SOURCE_OWM_WIND,
        "OpenWeatherMap wind layer",
        "Map data is CC-BY-SA 2.0 OpenWeatherMap contributors",
        "http://creativecommons.org/licenses/by-sa/2.0/",
        0,
        18,
        256,
        SHUMATE_MAP_PROJECTION_MERCATOR,
        "http://tile.openweathermap.org/map/wind/#Z#/#X#/#Y#.png",
        shumate_map_source_new_generic,
        NULL);
  shumate_map_source_factory_register (factory, desc);

  desc = shumate_map_source_desc_new_full (
        SHUMATE_MAP_SOURCE_OWM_TEMPERATURE,
        "OpenWeatherMap temperature layer",
        "Map data is CC-BY-SA 2.0 OpenWeatherMap contributors",
        "http://creativecommons.org/licenses/by-sa/2.0/",
        0,
        18,
        256,
        SHUMATE_MAP_PROJECTION_MERCATOR,
        "http://tile.openweathermap.org/map/temp/#Z#/#X#/#Y#.png",
        shumate_map_source_new_generic,
        NULL);
  shumate_map_source_factory_register (factory, desc);

  desc = shumate_map_source_desc_new_full (
        SHUMATE_MAP_SOURCE_OWM_PRECIPITATION,
        "OpenWeatherMap precipitation layer",
        "Map data is CC-BY-SA 2.0 OpenWeatherMap contributors",
        "http://creativecommons.org/licenses/by-sa/2.0/",
        0,
        18,
        256,
        SHUMATE_MAP_PROJECTION_MERCATOR,
        "http://tile.openweathermap.org/map/precipitation/#Z#/#X#/#Y#.png",
        shumate_map_source_new_generic,
        NULL);
  shumate_map_source_factory_register (factory, desc);

  desc = shumate_map_source_desc_new_full (
        SHUMATE_MAP_SOURCE_OWM_PRESSURE,
        "OpenWeatherMap sea level pressure layer",
        "Map data is CC-BY-SA 2.0 OpenWeatherMap contributors",
        "http://creativecommons.org/licenses/by-sa/2.0/",
        0,
        18,
        256,
        SHUMATE_MAP_PROJECTION_MERCATOR,
        "http://tile.openweathermap.org/map/pressure/#Z#/#X#/#Y#.png",
        shumate_map_source_new_generic,
        NULL);
  shumate_map_source_factory_register (factory, desc);
}


/**
 * shumate_map_source_factory_dup_default:
 *
 * A method to obtain the singleton object.
 *
 * Returns: (transfer full): the singleton #ShumateMapSourceFactory, it should be freed
 * using #g_object_unref() when not needed.
 */
ShumateMapSourceFactory *
shumate_map_source_factory_dup_default (void)
{
  return g_object_new (SHUMATE_TYPE_MAP_SOURCE_FACTORY, NULL);
}


/**
 * shumate_map_source_factory_get_registered:
 * @factory: the Factory
 *
 * Get the list of registered map sources.
 *
 * Returns: (transfer container) (element-type ShumateMapSourceDesc): the list of registered map sources, the items should not be freed,
 * the list should be freed with #g_slist_free.
 */
GSList *
shumate_map_source_factory_get_registered (ShumateMapSourceFactory *factory)
{
  g_return_val_if_fail (SHUMATE_IS_MAP_SOURCE_FACTORY (factory), NULL);

  return g_slist_copy (factory->registered_sources);
}


/**
 * shumate_map_source_factory_create:
 * @factory: the Factory
 * @id: the wanted map source id
 *
 * Note: The id should not contain any character that can't be in a filename as it
 * will be used as the cache directory name for that map source.
 *
 * Returns: (transfer none): a ready to use #ShumateMapSource matching the given name;
 * returns NULL if the source with the given name doesn't exist.
 */
ShumateMapSource *
shumate_map_source_factory_create (ShumateMapSourceFactory *factory,
    const char *id)
{
  GSList *item;

  g_return_val_if_fail (SHUMATE_IS_MAP_SOURCE_FACTORY (factory), NULL);

  item = factory->registered_sources;

  while (item != NULL)
    {
      ShumateMapSourceDesc *desc = SHUMATE_MAP_SOURCE_DESC (item->data);
      if (strcmp (shumate_map_source_desc_get_id (desc), id) == 0)
        {
          ShumateMapSourceConstructor constructor;

          constructor = shumate_map_source_desc_get_constructor (desc);
          return constructor (desc);
        }
      item = g_slist_next (item);
    }

  return NULL;
}


/**
 * shumate_map_source_factory_create_cached_source:
 * @factory: the Factory
 * @id: the wanted map source id
 *
 * Creates a cached map source.
 *
 * Returns: (transfer none): a ready to use #ShumateMapSourceChain consisting of
 * #ShumateMemoryCache, #ShumateFileCache, #ShumateMapSource matching the given name, and
 * an error tile source created with shumate_map_source_factory_create_error_source ().
 * Returns NULL if the source with the given name doesn't exist.
 */
ShumateMapSource *
shumate_map_source_factory_create_cached_source (ShumateMapSourceFactory *factory,
    const char *id)
{
  ShumateMapSourceChain *source_chain;
  ShumateMapSource *tile_source;
  ShumateMapSource *error_source;
  ShumateMapSource *memory_cache;
  ShumateMapSource *file_cache;
  guint tile_size;

  g_return_val_if_fail (SHUMATE_IS_MAP_SOURCE_FACTORY (factory), NULL);

  tile_source = shumate_map_source_factory_create (factory, id);
  if (!tile_source)
    return NULL;

  tile_size = shumate_map_source_get_tile_size (tile_source);
  error_source = shumate_map_source_factory_create_error_source (factory, tile_size);

  file_cache = SHUMATE_MAP_SOURCE (shumate_file_cache_new_full (100000000, NULL));

  memory_cache = SHUMATE_MAP_SOURCE (shumate_memory_cache_new_full (100));

  source_chain = shumate_map_source_chain_new ();
  shumate_map_source_chain_push (source_chain, error_source);
  shumate_map_source_chain_push (source_chain, tile_source);
  shumate_map_source_chain_push (source_chain, file_cache);
  shumate_map_source_chain_push (source_chain, memory_cache);

  return SHUMATE_MAP_SOURCE (source_chain);
}


/**
 * shumate_map_source_factory_create_memcached_source:
 * @factory: the Factory
 * @id: the wanted map source id
 *
 * Creates a memory cached map source.
 *
 * Returns: (transfer none): a ready to use #ShumateMapSourceChain consisting of
 * #ShumateMemoryCache and #ShumateMapSource matching the given name.
 * Returns NULL if the source with the given name doesn't exist.
 */
ShumateMapSource *
shumate_map_source_factory_create_memcached_source (ShumateMapSourceFactory *factory,
    const char *id)
{
  ShumateMapSourceChain *source_chain;
  ShumateMapSource *tile_source;
  ShumateMapSource *memory_cache;

  g_return_val_if_fail (SHUMATE_IS_MAP_SOURCE_FACTORY (factory), NULL);

  tile_source = shumate_map_source_factory_create (factory, id);
  if (!tile_source)
    return NULL;

  memory_cache = SHUMATE_MAP_SOURCE (shumate_memory_cache_new_full (100));

  source_chain = shumate_map_source_chain_new ();
  shumate_map_source_chain_push (source_chain, tile_source);
  shumate_map_source_chain_push (source_chain, memory_cache);

  return SHUMATE_MAP_SOURCE (source_chain);
}


/**
 * shumate_map_source_factory_create_error_source:
 * @factory: the Factory
 * @tile_size: the size of the error tile
 *
 * Creates a map source generating error tiles.
 *
 * Returns: (transfer none): a ready to use map source generating error tiles.
 */
ShumateMapSource *
shumate_map_source_factory_create_error_source (ShumateMapSourceFactory *factory,
    guint tile_size)
{
  ShumateMapSource *error_source;

  g_return_val_if_fail (SHUMATE_IS_MAP_SOURCE_FACTORY (factory), NULL);

  error_source = SHUMATE_MAP_SOURCE (shumate_error_tile_source_new_full ());

  return error_source;
}


static int
compare_id (ShumateMapSourceDesc *a, ShumateMapSourceDesc *b)
{
  const char *id_a, *id_b;

  id_a = shumate_map_source_desc_get_id (a);
  id_b = shumate_map_source_desc_get_id (b);

  return g_strcmp0 (id_a, id_b);
}


/**
 * shumate_map_source_factory_register:
 * @factory: A #ShumateMapSourceFactory
 * @desc: the description of the map source
 *
 * Registers the new map source with the given constructor.  When this map
 * source is requested, the given constructor will be used to build the
 * map source.  #ShumateMapSourceFactory will take ownership of the passed
 * #ShumateMapSourceDesc, so don't free it.
 *
 * Returns: TRUE if the registration suceeded.
 */
gboolean
shumate_map_source_factory_register (ShumateMapSourceFactory *factory,
    ShumateMapSourceDesc *desc)
{
  g_return_val_if_fail (SHUMATE_IS_MAP_SOURCE_FACTORY (factory), FALSE);

  if(!g_slist_find_custom (factory->registered_sources, desc, (GCompareFunc) compare_id))
    {
      factory->registered_sources = g_slist_append (factory->registered_sources, desc);
      return TRUE;
    }
  return FALSE;
}


static ShumateMapSource *
shumate_map_source_new_generic (ShumateMapSourceDesc *desc)
{
  ShumateMapSource *map_source;
  const char *id, *name, *license, *license_uri, *uri_format;
  guint min_zoom, max_zoom, tile_size;
  ShumateMapProjection projection;

  id = shumate_map_source_desc_get_id (desc);
  name = shumate_map_source_desc_get_name (desc);
  license = shumate_map_source_desc_get_license (desc);
  license_uri = shumate_map_source_desc_get_license_uri (desc);
  min_zoom = shumate_map_source_desc_get_min_zoom_level (desc);
  max_zoom = shumate_map_source_desc_get_max_zoom_level (desc);
  tile_size = shumate_map_source_desc_get_tile_size (desc);
  projection = shumate_map_source_desc_get_projection (desc);
  uri_format = shumate_map_source_desc_get_uri_format (desc);

  map_source = SHUMATE_MAP_SOURCE (shumate_network_tile_source_new_full (
            id,
            name,
            license,
            license_uri,
            min_zoom,
            max_zoom,
            tile_size,
            projection,
            uri_format));

  return map_source;
}

