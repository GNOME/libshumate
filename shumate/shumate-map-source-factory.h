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

#ifndef SHUMATE_MAP_SOURCE_FACTORY_H
#define SHUMATE_MAP_SOURCE_FACTORY_H

#include <shumate/shumate-features.h>
#include <shumate/shumate-defines.h>
#include <shumate/shumate-map-source.h>
#include <shumate/shumate-map-source-desc.h>

#include <glib-object.h>

G_BEGIN_DECLS

#define SHUMATE_TYPE_MAP_SOURCE_FACTORY shumate_map_source_factory_get_type ()
G_DECLARE_FINAL_TYPE (ShumateMapSourceFactory, shumate_map_source_factory, SHUMATE, MAP_SOURCE_FACTORY, GObject)

/**
 * ShumateMapSourceFactory:
 *
 * The #ShumateMapSourceFactory structure contains only private data
 * and should be accessed using the provided API
 */

ShumateMapSourceFactory *shumate_map_source_factory_dup_default (void);

ShumateMapSource *shumate_map_source_factory_create (ShumateMapSourceFactory *factory,
    const gchar *id);
ShumateMapSource *shumate_map_source_factory_create_cached_source (ShumateMapSourceFactory *factory,
    const gchar *id);
ShumateMapSource *shumate_map_source_factory_create_memcached_source (ShumateMapSourceFactory *factory,
    const gchar *id);
ShumateMapSource *shumate_map_source_factory_create_error_source (ShumateMapSourceFactory *factory,
    guint tile_size);

gboolean shumate_map_source_factory_register (ShumateMapSourceFactory *factory,
    ShumateMapSourceDesc *desc);
GSList *shumate_map_source_factory_get_registered (ShumateMapSourceFactory *factory);

/**
 * SHUMATE_MAP_SOURCE_OSM_MAPNIK:
 *
 * OpenStreetMap Mapnik
 */
#define SHUMATE_MAP_SOURCE_OSM_MAPNIK "osm-mapnik"
/**
 * SHUMATE_MAP_SOURCE_OSM_CYCLE_MAP:
 *
 * OpenStreetMap Cycle Map
 */
#define SHUMATE_MAP_SOURCE_OSM_CYCLE_MAP "osm-cyclemap"
/**
 * SHUMATE_MAP_SOURCE_OSM_TRANSPORT_MAP:
 *
 * OpenStreetMap Transport Map
 */
#define SHUMATE_MAP_SOURCE_OSM_TRANSPORT_MAP "osm-transportmap"
/**
 * SHUMATE_MAP_SOURCE_MFF_RELIEF:
 *
 * Maps for Free Relief
 */
#define SHUMATE_MAP_SOURCE_MFF_RELIEF "mff-relief"
/**
 * SHUMATE_MAP_SOURCE_OWM_CLOUDS:
 *
 * OpenWeatherMap clouds layer
 */
#define SHUMATE_MAP_SOURCE_OWM_CLOUDS "owm-clouds"
/**
 * SHUMATE_MAP_SOURCE_OWM_PRECIPITATION:
 *
 * OpenWeatherMap precipitation
 */
#define SHUMATE_MAP_SOURCE_OWM_PRECIPITATION "owm-precipitation"
/**
 * SHUMATE_MAP_SOURCE_OWM_PRESSURE:
 *
 * OpenWeatherMap sea level pressure
 */
#define SHUMATE_MAP_SOURCE_OWM_PRESSURE "owm-pressure"
/**
 * SHUMATE_MAP_SOURCE_OWM_WIND:
 *
 * OpenWeatherMap wind
 */
#define SHUMATE_MAP_SOURCE_OWM_WIND "owm-wind"
/**
 * SHUMATE_MAP_SOURCE_OWM_TEMPERATURE:
 *
 * OpenWeatherMap temperature
 */
#define SHUMATE_MAP_SOURCE_OWM_TEMPERATURE "owm-temperature"

G_END_DECLS

#endif
