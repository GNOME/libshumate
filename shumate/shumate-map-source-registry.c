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

/**
 * ShumateMapSourceRegistry:
 *
 * This object allows you to hold [class@MapSource] instances, you can access a
 * default set of sources with [method@MapSourceRegistry.populate_defaults].
 *
 * It conveniently implements [iface@Gio.ListModel] to easily integrate with it.
 */

#include "shumate-map-source-registry.h"

#include <gio/gio.h>

#include "shumate-raster-renderer.h"
#include "shumate-tile-downloader.h"

struct _ShumateMapSourceRegistry
{
  GObject parent_instance;

  GPtrArray *map_sources;
};

static void shumate_map_source_registry_list_model_init (GListModelInterface *iface);

G_DEFINE_TYPE_WITH_CODE (ShumateMapSourceRegistry, shumate_map_source_registry, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE(G_TYPE_LIST_MODEL, shumate_map_source_registry_list_model_init))

static gboolean
shumate_map_source_registry_find_by_id (ShumateMapSource *map_source,
                                        const gchar      *id)
{
  return g_strcmp0 (shumate_map_source_get_id (map_source), id) == 0;
}

static void
shumate_map_source_registry_dispose (GObject *object)
{
  ShumateMapSourceRegistry *self = (ShumateMapSourceRegistry *)object;

  g_clear_pointer (&self->map_sources, g_ptr_array_unref);

  G_OBJECT_CLASS (shumate_map_source_registry_parent_class)->dispose (object);
}

static void
shumate_map_source_registry_class_init (ShumateMapSourceRegistryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = shumate_map_source_registry_dispose;
}

static void
shumate_map_source_registry_init (ShumateMapSourceRegistry *self)
{
  self->map_sources = g_ptr_array_new_with_free_func (g_object_unref);
}

static GType
shumate_map_source_registry_get_item_type (GListModel *list)
{
  return SHUMATE_TYPE_MAP_SOURCE;
}

static guint
shumate_map_source_registry_get_n_items (GListModel *list)
{
  ShumateMapSourceRegistry *self = SHUMATE_MAP_SOURCE_REGISTRY (list);

  return self->map_sources->len;
}

static gpointer
shumate_map_source_registry_get_item (GListModel *list,
                                      guint       position)
{
  ShumateMapSourceRegistry *self = SHUMATE_MAP_SOURCE_REGISTRY (list);

  if (position >= self->map_sources->len)
    return NULL;
  else
    return g_object_ref (g_ptr_array_index (self->map_sources, position));
}

static void
shumate_map_source_registry_list_model_init (GListModelInterface *iface)
{
  iface->get_item_type = shumate_map_source_registry_get_item_type;
  iface->get_n_items = shumate_map_source_registry_get_n_items;
  iface->get_item = shumate_map_source_registry_get_item;
}

/**
 * shumate_map_source_registry_new:
 *
 * Create a new #ShumateMapSourceRegistry.
 *
 * Returns: (transfer full): a newly created #ShumateMapSourceRegistry
 */
ShumateMapSourceRegistry *
shumate_map_source_registry_new (void)
{
  return g_object_new (SHUMATE_TYPE_MAP_SOURCE_REGISTRY, NULL);
}

/**
 * shumate_map_source_registry_new_with_defaults:
 *
 * Create a new #ShumateMapSourceRegistry with defaults map sources.
 * This is identical to calling [method@MapSourceRegistry.populate_defaults]
 * after shumate_map_source_registry_new().
 *
 * Returns: (transfer full): a newly created #ShumateMapSourceRegistry
 */
ShumateMapSourceRegistry *
shumate_map_source_registry_new_with_defaults (void)
{
  ShumateMapSourceRegistry *registry = shumate_map_source_registry_new ();

  shumate_map_source_registry_populate_defaults (registry);

  return registry;
}

/**
 * shumate_map_source_registry_populate_defaults:
 * @self: a #ShumateMapSourceRegistry
 *
 * Populates the #ShumateMapSourceRegistry with a default set of sources.
 */
void
shumate_map_source_registry_populate_defaults (ShumateMapSourceRegistry *self)
{
  guint n_items;

  g_return_if_fail (SHUMATE_IS_MAP_SOURCE_REGISTRY (self));

  n_items = self->map_sources->len;

  if (!shumate_map_source_registry_get_by_id (self, SHUMATE_MAP_SOURCE_OSM_MAPNIK))
    {
      g_ptr_array_add (self->map_sources,
        shumate_raster_renderer_new_full_from_url (
          SHUMATE_MAP_SOURCE_OSM_MAPNIK,
          "OpenStreetMap Mapnik",
          "Map Data ODBL OpenStreetMap Contributors, Map Imagery CC-BY-SA 2.0 OpenStreetMap",
          "http://creativecommons.org/licenses/by-sa/2.0/",
          0,
          18,
          256,
          SHUMATE_MAP_PROJECTION_MERCATOR,
          "https://tile.openstreetmap.org/{z}/{x}/{y}.png"
        )
      );
    }

  if (!shumate_map_source_registry_get_by_id (self, SHUMATE_MAP_SOURCE_OSM_CYCLE_MAP))
    {
      g_ptr_array_add (self->map_sources,
        shumate_raster_renderer_new_full_from_url (
          SHUMATE_MAP_SOURCE_OSM_CYCLE_MAP,
          "OpenStreetMap Cycle Map",
          "Map data is CC-BY-SA 2.0 OpenStreetMap contributors",
          "http://creativecommons.org/licenses/by-sa/2.0/",
          0,
          18,
          256,
          SHUMATE_MAP_PROJECTION_MERCATOR,
          "http://tile.opencyclemap.org/cycle/{z}/{x}/{y}.png"
        )
      );
    }

  if (!shumate_map_source_registry_get_by_id (self, SHUMATE_MAP_SOURCE_OSM_TRANSPORT_MAP))
    {
      g_ptr_array_add (self->map_sources,
        shumate_raster_renderer_new_full_from_url (
          SHUMATE_MAP_SOURCE_OSM_TRANSPORT_MAP,
          "OpenStreetMap Transport Map",
          "Map data is CC-BY-SA 2.0 OpenStreetMap contributors",
          "http://creativecommons.org/licenses/by-sa/2.0/",
          0,
          18,
          256,
          SHUMATE_MAP_PROJECTION_MERCATOR,
          "http://tile.xn--pnvkarte-m4a.de/tilegen/{z}/{x}/{y}.png"
        )
      );
    }

  if (!shumate_map_source_registry_get_by_id (self, SHUMATE_MAP_SOURCE_MFF_RELIEF))
    {
      g_ptr_array_add (self->map_sources,
        shumate_raster_renderer_new_full_from_url (
          SHUMATE_MAP_SOURCE_MFF_RELIEF,
          "Maps for Free Relief",
          "Map data available under GNU Free Documentation license, Version 1.2 or later",
          "http://www.gnu.org/copyleft/fdl.html",
          0,
          11,
          256,
          SHUMATE_MAP_PROJECTION_MERCATOR,
          "http://maps-for-free.com/layer/relief/z{z}/row{y}/{z}_{x}-{y}.jpg"
        )
      );
    }

  if (!shumate_map_source_registry_get_by_id (self, SHUMATE_MAP_SOURCE_OWM_CLOUDS))
    {
      g_ptr_array_add (self->map_sources,
        shumate_raster_renderer_new_full_from_url (
          SHUMATE_MAP_SOURCE_OWM_CLOUDS,
          "OpenWeatherMap cloud layer",
          "Map data is CC-BY-SA 2.0 OpenWeatherMap contributors",
          "http://creativecommons.org/licenses/by-sa/2.0/",
          0,
          18,
          256,
          SHUMATE_MAP_PROJECTION_MERCATOR,
          "http://tile.openweathermap.org/map/clouds/{z}/{x}/{y}.png"
        )
      );
    }

  if (!shumate_map_source_registry_get_by_id (self, SHUMATE_MAP_SOURCE_OWM_WIND))
    {
      g_ptr_array_add (self->map_sources,
        shumate_raster_renderer_new_full_from_url (
          SHUMATE_MAP_SOURCE_OWM_WIND,
          "OpenWeatherMap wind layer",
          "Map data is CC-BY-SA 2.0 OpenWeatherMap contributors",
          "http://creativecommons.org/licenses/by-sa/2.0/",
          0,
          18,
          256,
          SHUMATE_MAP_PROJECTION_MERCATOR,
          "http://tile.openweathermap.org/map/wind/{z}/{x}/{y}.png"
        )
      );
    }

  if (!shumate_map_source_registry_get_by_id (self, SHUMATE_MAP_SOURCE_OWM_TEMPERATURE))
    {
      g_ptr_array_add (self->map_sources,
        shumate_raster_renderer_new_full_from_url (
          SHUMATE_MAP_SOURCE_OWM_TEMPERATURE,
          "OpenWeatherMap temperature layer",
          "Map data is CC-BY-SA 2.0 OpenWeatherMap contributors",
          "http://creativecommons.org/licenses/by-sa/2.0/",
          0,
          18,
          256,
          SHUMATE_MAP_PROJECTION_MERCATOR,
          "http://tile.openweathermap.org/map/temp/{z}/{x}/{y}.png"
        )
      );
    }

  if (!shumate_map_source_registry_get_by_id (self, SHUMATE_MAP_SOURCE_OWM_PRECIPITATION))
    {
      g_ptr_array_add (self->map_sources,
        shumate_raster_renderer_new_full_from_url (
          SHUMATE_MAP_SOURCE_OWM_PRECIPITATION,
          "OpenWeatherMap precipitation layer",
          "Map data is CC-BY-SA 2.0 OpenWeatherMap contributors",
          "http://creativecommons.org/licenses/by-sa/2.0/",
          0,
          18,
          256,
          SHUMATE_MAP_PROJECTION_MERCATOR,
          "http://tile.openweathermap.org/map/precipitation/{z}/{x}/{y}.png"
        )
      );
    }

  if (!shumate_map_source_registry_get_by_id (self, SHUMATE_MAP_SOURCE_OWM_PRESSURE))
    {
      g_ptr_array_add (self->map_sources,
        shumate_raster_renderer_new_full_from_url (
          SHUMATE_MAP_SOURCE_OWM_PRESSURE,
          "OpenWeatherMap sea level pressure layer",
          "Map data is CC-BY-SA 2.0 OpenWeatherMap contributors",
          "http://creativecommons.org/licenses/by-sa/2.0/",
          0,
          18,
          256,
          SHUMATE_MAP_PROJECTION_MERCATOR,
          "http://tile.openweathermap.org/map/pressure/{z}/{x}/{y}.png"
        )
      );
    }

  if (self->map_sources->len - n_items > 0) {
    g_list_model_items_changed (G_LIST_MODEL (self), n_items, self->map_sources->len - n_items, 0);
  }
}

/**
 * shumate_map_source_registry_get_by_id:
 * @self: a #ShumateMapSourceRegistry
 * @id: the id of the #ShumateMapSource
 *
 * Find the #ShumateMapSource with the corresponding id
 *
 * Returns: (transfer none) (nullable): the #ShumateMapSource or %NULL if no
 * map source has been found
 */
ShumateMapSource *
shumate_map_source_registry_get_by_id (ShumateMapSourceRegistry *self,
                                       const gchar              *id)
{
  guint index;

  g_return_val_if_fail (SHUMATE_IS_MAP_SOURCE_REGISTRY (self), NULL);
  g_return_val_if_fail (id != NULL, NULL);

  if (g_ptr_array_find_with_equal_func (self->map_sources, id,
                                        (GEqualFunc) shumate_map_source_registry_find_by_id,
                                        &index))
    {
      return g_ptr_array_index (self->map_sources, index);
    }

  return NULL;
}

/**
 * shumate_map_source_registry_add:
 * @self: a #ShumateMapSourceRegistry
 * @map_source: (transfer full): a #ShumateMapSource
 *
 * Adds the #ShumateMapSource to the #ShumateMapSourceRegistry
 */
void shumate_map_source_registry_add (ShumateMapSourceRegistry *self,
                                      ShumateMapSource         *map_source)
{
  guint n_items;

  g_return_if_fail (SHUMATE_IS_MAP_SOURCE_REGISTRY (self));
  g_return_if_fail (SHUMATE_IS_MAP_SOURCE (map_source));

  if (!g_ptr_array_find_with_equal_func (self->map_sources, shumate_map_source_get_id (map_source),
                                         (GEqualFunc) shumate_map_source_registry_find_by_id,
                                         NULL))
    {
      n_items = self->map_sources->len;
      g_ptr_array_add (self->map_sources, map_source);

      g_list_model_items_changed (G_LIST_MODEL (self), n_items, 0, 1);
    }
}

/**
 * shumate_map_source_registry_remove:
 * @self: a #ShumateMapSourceRegistry
 * @id: a #ShumateMapSource id
 *
 * Removes the corresponding #ShumateMapSource from the registry.
 * If the source doesn't exist in the registry, this function does nothing.
 */
void shumate_map_source_registry_remove (ShumateMapSourceRegistry *self,
                                         const gchar              *id)
{
  guint index;

  g_return_if_fail (SHUMATE_IS_MAP_SOURCE_REGISTRY (self));
  g_return_if_fail (id != NULL);

  if (g_ptr_array_find_with_equal_func (self->map_sources, id,
                                        (GEqualFunc) shumate_map_source_registry_find_by_id,
                                        &index))
    {
      g_ptr_array_remove_index (self->map_sources, index);
      g_list_model_items_changed (G_LIST_MODEL (self), index, 1, 0);
    }
}
