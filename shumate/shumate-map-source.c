/*
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
 * SECTION:shumate-map-source
 * @short_description: A base class for map sources
 *
 * #ShumateMapSource is the base class for all map sources. Map sources fill
 * #ShumateTile objects with images from various sources: a web API, for
 * example, or a test pattern generated on demand.
 *
 * The most common map source is #ShumateNetworkMapSource, which fetches tiles
 * from an API.
 */

#include "shumate-map-source.h"
#include "shumate-location.h"

#include <math.h>

G_DEFINE_ABSTRACT_TYPE (ShumateMapSource, shumate_map_source, G_TYPE_OBJECT);

static void
shumate_map_source_class_init (ShumateMapSourceClass *klass)
{
  klass->get_id = NULL;
  klass->get_name = NULL;
  klass->get_license = NULL;
  klass->get_license_uri = NULL;
  klass->get_min_zoom_level = NULL;
  klass->get_max_zoom_level = NULL;
  klass->get_tile_size = NULL;
  klass->get_projection = NULL;

  klass->fill_tile_async = NULL;
}

static double
map_size (ShumateMapSource *self, double zoom_level)
{
  return shumate_map_source_get_column_count (self, zoom_level) * shumate_map_source_get_tile_size (self) * (fmod (zoom_level, 1.0) + 1.0);
}


static void
shumate_map_source_init (ShumateMapSource *map_source)
{
}

/**
 * shumate_map_source_get_id:
 * @map_source: a #ShumateMapSource
 *
 * Gets map source's id.
 *
 * Returns: the map source's id.
 */
const char *
shumate_map_source_get_id (ShumateMapSource *map_source)
{
  g_return_val_if_fail (SHUMATE_IS_MAP_SOURCE (map_source), NULL);

  return SHUMATE_MAP_SOURCE_GET_CLASS (map_source)->get_id (map_source);
}


/**
 * shumate_map_source_get_name:
 * @map_source: a #ShumateMapSource
 *
 * Gets map source's name.
 *
 * Returns: the map source's name.
 */
const char *
shumate_map_source_get_name (ShumateMapSource *map_source)
{
  g_return_val_if_fail (SHUMATE_IS_MAP_SOURCE (map_source), NULL);

  return SHUMATE_MAP_SOURCE_GET_CLASS (map_source)->get_name (map_source);
}


/**
 * shumate_map_source_get_license:
 * @map_source: a #ShumateMapSource
 *
 * Gets map source's license.
 *
 * Returns: the map source's license.
 */
const char *
shumate_map_source_get_license (ShumateMapSource *map_source)
{
  g_return_val_if_fail (SHUMATE_IS_MAP_SOURCE (map_source), NULL);

  return SHUMATE_MAP_SOURCE_GET_CLASS (map_source)->get_license (map_source);
}


/**
 * shumate_map_source_get_license_uri:
 * @map_source: a #ShumateMapSource
 *
 * Gets map source's license URI.
 *
 * Returns: the map source's license URI.
 */
const char *
shumate_map_source_get_license_uri (ShumateMapSource *map_source)
{
  g_return_val_if_fail (SHUMATE_IS_MAP_SOURCE (map_source), NULL);

  return SHUMATE_MAP_SOURCE_GET_CLASS (map_source)->get_license_uri (map_source);
}


/**
 * shumate_map_source_get_min_zoom_level:
 * @map_source: a #ShumateMapSource
 *
 * Gets map source's minimum zoom level.
 *
 * Returns: the miminum zoom level this map source supports
 */
guint
shumate_map_source_get_min_zoom_level (ShumateMapSource *map_source)
{
  g_return_val_if_fail (SHUMATE_IS_MAP_SOURCE (map_source), 0);

  return SHUMATE_MAP_SOURCE_GET_CLASS (map_source)->get_min_zoom_level (map_source);
}


/**
 * shumate_map_source_get_max_zoom_level:
 * @map_source: a #ShumateMapSource
 *
 * Gets map source's maximum zoom level.
 *
 * Returns: the maximum zoom level this map source supports
 */
guint
shumate_map_source_get_max_zoom_level (ShumateMapSource *map_source)
{
  g_return_val_if_fail (SHUMATE_IS_MAP_SOURCE (map_source), 0);

  return SHUMATE_MAP_SOURCE_GET_CLASS (map_source)->get_max_zoom_level (map_source);
}


/**
 * shumate_map_source_get_tile_size:
 * @map_source: a #ShumateMapSource
 *
 * Gets map source's tile size.
 *
 * Returns: the tile's size (width and height) in pixels for this map source
 */
guint
shumate_map_source_get_tile_size (ShumateMapSource *map_source)
{
  g_return_val_if_fail (SHUMATE_IS_MAP_SOURCE (map_source), 0);

  return SHUMATE_MAP_SOURCE_GET_CLASS (map_source)->get_tile_size (map_source);
}


/**
 * shumate_map_source_get_projection:
 * @map_source: a #ShumateMapSource
 *
 * Gets map source's projection.
 *
 * Returns: the map source's projection.
 */
ShumateMapProjection
shumate_map_source_get_projection (ShumateMapSource *map_source)
{
  g_return_val_if_fail (SHUMATE_IS_MAP_SOURCE (map_source), SHUMATE_MAP_PROJECTION_MERCATOR);

  return SHUMATE_MAP_SOURCE_GET_CLASS (map_source)->get_projection (map_source);
}


/**
 * shumate_map_source_get_x:
 * @map_source: a #ShumateMapSource
 * @zoom_level: the zoom level
 * @longitude: a longitude
 *
 * Gets the x position on the map using this map source's projection.
 * (0, 0) is located at the top left.
 *
 * Returns: the x position
 */
double
shumate_map_source_get_x (ShumateMapSource *map_source,
    double zoom_level,
    double longitude)
{
  g_return_val_if_fail (SHUMATE_IS_MAP_SOURCE (map_source), 0.0);

  longitude = CLAMP (longitude, SHUMATE_MIN_LONGITUDE, SHUMATE_MAX_LONGITUDE);

  /* FIXME: support other projections */
  return ((longitude + 180.0) / 360.0) * map_size (map_source, zoom_level);
}


/**
 * shumate_map_source_get_y:
 * @map_source: a #ShumateMapSource
 * @zoom_level: the zoom level
 * @latitude: a latitude
 *
 * Gets the y position on the map using this map source's projection.
 * (0, 0) is located at the top left.
 *
 * Returns: the y position
 */
double
shumate_map_source_get_y (ShumateMapSource *map_source,
    double zoom_level,
    double latitude)
{
  double sin_latitude;

  g_return_val_if_fail (SHUMATE_IS_MAP_SOURCE (map_source), 0.0);

  latitude = CLAMP (latitude, SHUMATE_MIN_LATITUDE, SHUMATE_MAX_LATITUDE);
  /* FIXME: support other projections */
  sin_latitude = sin (latitude * G_PI / 180.0);
  return (0.5 - log ((1.0 + sin_latitude) / (1.0 - sin_latitude)) / (4.0 * G_PI)) * map_size (map_source, zoom_level);
}


/**
 * shumate_map_source_get_longitude:
 * @map_source: a #ShumateMapSource
 * @zoom_level: the zoom level
 * @x: a x position
 *
 * Gets the longitude corresponding to this x position in the map source's
 * projection.
 *
 * Returns: the longitude
 */
double
shumate_map_source_get_longitude (ShumateMapSource *map_source,
    double zoom_level,
    double x)
{
  double longitude;

  g_return_val_if_fail (SHUMATE_IS_MAP_SOURCE (map_source), 0.0);

  /* FIXME: support other projections */
  longitude = x / map_size (map_source, zoom_level) * 360.0 - 180.0;

  return CLAMP (longitude, SHUMATE_MIN_LONGITUDE, SHUMATE_MAX_LONGITUDE);
}


/**
 * shumate_map_source_get_latitude:
 * @map_source: a #ShumateMapSource
 * @zoom_level: the zoom level
 * @y: a y position
 *
 * Gets the latitude corresponding to this y position in the map source's
 * projection.
 *
 * Returns: the latitude
 */
double
shumate_map_source_get_latitude (ShumateMapSource *map_source,
    double zoom_level,
    double y)
{
  double latitude, dy;

  g_return_val_if_fail (SHUMATE_IS_MAP_SOURCE (map_source), 0.0);
  /* FIXME: support other projections */
  dy = 0.5 - y / map_size (map_source, zoom_level);
  latitude = 90.0 - 360.0 / G_PI * atan (exp (-dy * 2.0 * G_PI));
  
  return CLAMP (latitude, SHUMATE_MIN_LATITUDE, SHUMATE_MAX_LATITUDE);
}


/**
 * shumate_map_source_get_row_count:
 * @map_source: a #ShumateMapSource
 * @zoom_level: the zoom level
 *
 * Gets the number of tiles in a row at this zoom level for this map source.
 *
 * Returns: the number of tiles in a row
 */
guint
shumate_map_source_get_row_count (ShumateMapSource *map_source,
    guint zoom_level)
{
  g_return_val_if_fail (SHUMATE_IS_MAP_SOURCE (map_source), 0);
  /* FIXME: support other projections */
  return (zoom_level != 0) ? 2 << (zoom_level - 1) : 1;
}


/**
 * shumate_map_source_get_column_count:
 * @map_source: a #ShumateMapSource
 * @zoom_level: the zoom level
 *
 * Gets the number of tiles in a column at this zoom level for this map
 * source.
 *
 * Returns: the number of tiles in a column
 */
guint
shumate_map_source_get_column_count (ShumateMapSource *map_source,
    guint zoom_level)
{
  g_return_val_if_fail (SHUMATE_IS_MAP_SOURCE (map_source), 0);
  /* FIXME: support other projections */
  return (zoom_level != 0) ? 2 << (zoom_level - 1) : 1;
}


#define EARTH_RADIUS 6378137.0 /* meters, Equatorial radius */

/**
 * shumate_map_source_get_meters_per_pixel:
 * @map_source: a #ShumateMapSource
 * @zoom_level: the zoom level
 * @latitude: a latitude
 * @longitude: a longitude
 *
 * Gets meters per pixel at the position on the map using this map source's projection.
 *
 * Returns: the meters per pixel
 */
double
shumate_map_source_get_meters_per_pixel (ShumateMapSource *map_source,
    double zoom_level,
    double latitude,
    G_GNUC_UNUSED double longitude)
{
  g_return_val_if_fail (SHUMATE_IS_MAP_SOURCE (map_source), 0.0);

  /*
   * Width is in pixels. (1 px)
   * m/px = radius_at_latitude / width_in_pixels
   * k = radius of earth = 6 378.1 km
   * radius_at_latitude = 2pi * k * sin (pi/2-theta)
   */

  /* FIXME: support other projections */
  return 2.0 * G_PI * EARTH_RADIUS * sin (G_PI / 2.0 - G_PI / 180.0 * latitude) / map_size (map_source, zoom_level);
}


/**
 * shumate_map_source_fill_tile_async:
 * @self: a #ShumateMapSource
 * @tile: a #ShumateTile
 * @cancellable: (nullable): a #GCancellable
 * @callback: a #GAsyncReadyCallback to execute upon completion
 * @user_data: closure data for @callback
 *
 * Asynchronous version of shumate_map_source_fill_tile().
 */
void
shumate_map_source_fill_tile_async (ShumateMapSource *self,
                                    ShumateTile *tile,
                                    GCancellable *cancellable,
                                    GAsyncReadyCallback callback,
                                    gpointer user_data)
{
  g_return_if_fail (SHUMATE_IS_MAP_SOURCE (self));
  g_return_if_fail (SHUMATE_IS_TILE (tile));
  g_return_if_fail (!cancellable || G_IS_CANCELLABLE (cancellable));

  return SHUMATE_MAP_SOURCE_GET_CLASS (self)->fill_tile_async (self, tile, cancellable, callback, user_data);
}

/**
 * shumate_map_source_fill_tile_finish:
 * @self: a #ShumateMapSource
 * @result: a #GAsyncResult provided to callback
 * @error: a location for a #GError, or %NULL
 *
 * Gets the success value of a completed shumate_map_source_fill_tile_async()
 * operation.
 *
 * Returns: %TRUE if the tile was filled with valid data, otherwise %FALSE
 */
gboolean
shumate_map_source_fill_tile_finish (ShumateMapSource *self,
                                     GAsyncResult *result,
                                     GError **error)
{
  g_return_val_if_fail (SHUMATE_IS_MAP_SOURCE (self), FALSE);
  g_return_val_if_fail (g_task_is_valid (result, self), FALSE);

  return g_task_propagate_boolean (G_TASK (result), error);
}
