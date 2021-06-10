/*
 * Copyright 2021 Collabora Ltd. (https://collabora.com)
 * Copyright 2021 Corentin Noël <corentin.noel@collabora.com>
 * Copyright 2009 Pierre-Luc Beaudoin <pierre-luc@pierlux.com>
 * Copyright 2010-2013 Jiri Techet <techet@gmail.com>
 * Copyright 2019 Marcus Lundblad <ml@update.uu.se>
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

#ifndef __SHUMATE_MAP_SOURCE_REGISTRY_H__
#define __SHUMATE_MAP_SOURCE_REGISTRY_H__

#include <glib-object.h>

#include <shumate/shumate-map-source.h>

G_BEGIN_DECLS

#define SHUMATE_TYPE_MAP_SOURCE_REGISTRY (shumate_map_source_registry_get_type())

G_DECLARE_FINAL_TYPE (ShumateMapSourceRegistry, shumate_map_source_registry, SHUMATE, MAP_SOURCE_REGISTRY, GObject)

ShumateMapSourceRegistry *shumate_map_source_registry_new (void);
ShumateMapSourceRegistry *shumate_map_source_registry_new_with_defaults (void);
void shumate_map_source_registry_populate_defaults (ShumateMapSourceRegistry *self);
ShumateMapSource *shumate_map_source_registry_get_by_id (ShumateMapSourceRegistry *self,
                                                         const gchar              *id);
void shumate_map_source_registry_add (ShumateMapSourceRegistry *self,
                                      ShumateMapSource         *map_source);
void shumate_map_source_registry_remove (ShumateMapSourceRegistry *self,
                                         const gchar              *id);

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

#endif /* __SHUMATE_MAP_SOURCE_REGISTRY_H__ */
