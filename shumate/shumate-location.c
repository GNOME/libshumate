/*
 * Copyright (C) 2011-2013 Jiri Techet <techet@gmail.com>
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
 * SECTION:shumate-location
 * @short_description: An interface common to objects having latitude and longitude
 *
 * By implementing #ShumateLocation the object declares that it has latitude
 * and longitude and can be used to specify location on the map.
 */

#include "shumate-location.h"

G_DEFINE_INTERFACE (ShumateLocation, shumate_location, G_TYPE_OBJECT);


static void
shumate_location_default_init (ShumateLocationInterface *iface)
{
  /**
   * ShumateLocation:longitude:
   *
   * The longitude coordonate
   */
  g_object_interface_install_property (iface,
      g_param_spec_double ("longitude",
          "Longitude",
          "The longitude coordonate",
          -180.0f,
          180.0f,
          0.0f,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * ShumateLocation:latitude:
   *
   * The latitude coordonate
   */
  g_object_interface_install_property (iface,
      g_param_spec_double ("latitude",
          "Latitude",
          "The latitude coordonate",
          -90.0f,
          90.0f,
          0.0f,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}


/**
 * shumate_location_set_location:
 * @location: a #ShumateLocation
 * @latitude: the latitude
 * @longitude: the longitude
 *
 * Sets the coordinates of the location
 */
void
shumate_location_set_location (ShumateLocation *location,
    gdouble latitude,
    gdouble longitude)
{
  SHUMATE_LOCATION_GET_IFACE (location)->set_location (location,
      latitude,
      longitude);
}


/**
 * shumate_location_get_latitude:
 * @location: a #ShumateLocation
 *
 * Gets the latitude coordinate.
 *
 * Returns: the latitude coordinate.
 */
gdouble
shumate_location_get_latitude (ShumateLocation *location)
{
  return SHUMATE_LOCATION_GET_IFACE (location)->get_latitude (location);
}


/**
 * shumate_location_get_longitude:
 * @location: a #ShumateLocation
 *
 * Gets the longitude coordinate.
 *
 * Returns: the longitude coordinate.
 */
gdouble
shumate_location_get_longitude (ShumateLocation *location)
{
  return SHUMATE_LOCATION_GET_IFACE (location)->get_longitude (location);
}
