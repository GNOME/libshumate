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
 * ShumateCoordinate:
 *
 * A simple object implementing [iface@Location].
 */

#include "shumate-coordinate.h"

#include "shumate-marshal.h"
#include "shumate-location.h"

enum
{
  PROP_LONGITUDE = 1,
  PROP_LATITUDE,
};


static void set_location (ShumateLocation *location,
    double latitude,
    double longitude);
static double get_latitude (ShumateLocation *location);
static double get_longitude (ShumateLocation *location);

static void location_interface_init (ShumateLocationInterface *iface);

typedef struct
{
  double longitude;
  double latitude;
} ShumateCoordinatePrivate;

G_DEFINE_TYPE_WITH_CODE (ShumateCoordinate, shumate_coordinate, G_TYPE_INITIALLY_UNOWNED,
                         G_ADD_PRIVATE (ShumateCoordinate)
                         G_IMPLEMENT_INTERFACE (SHUMATE_TYPE_LOCATION, location_interface_init));

static void
shumate_coordinate_get_property (GObject *object,
    guint prop_id,
    GValue *value,
    GParamSpec *pspec)
{
  ShumateCoordinate *coordinate = SHUMATE_COORDINATE (object);
  ShumateCoordinatePrivate *priv = shumate_coordinate_get_instance_private (coordinate);

  switch (prop_id)
    {
    case PROP_LONGITUDE:
      g_value_set_double (value, priv->longitude);
      break;

    case PROP_LATITUDE:
      g_value_set_double (value, priv->latitude);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}


static void
shumate_coordinate_set_property (GObject *object,
    guint prop_id,
    const GValue *value,
    GParamSpec *pspec)
{
  ShumateCoordinate *coordinate = SHUMATE_COORDINATE (object);
  ShumateCoordinatePrivate *priv = shumate_coordinate_get_instance_private (coordinate);

  switch (prop_id)
    {
    case PROP_LONGITUDE:
      {
        double longitude = g_value_get_double (value);
        set_location (SHUMATE_LOCATION (coordinate), priv->latitude, longitude);
        break;
      }

    case PROP_LATITUDE:
      {
        double latitude = g_value_get_double (value);
        set_location (SHUMATE_LOCATION (coordinate), latitude, priv->longitude);
        break;
      }

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}


static void
set_location (ShumateLocation *location,
    double latitude,
    double longitude)
{
  ShumateCoordinate *coordinate = SHUMATE_COORDINATE (location);
  ShumateCoordinatePrivate *priv = shumate_coordinate_get_instance_private (coordinate);

  g_return_if_fail (SHUMATE_IS_COORDINATE (location));

  priv->longitude = CLAMP (longitude, SHUMATE_MIN_LONGITUDE, SHUMATE_MAX_LONGITUDE);
  priv->latitude = CLAMP (latitude, SHUMATE_MIN_LATITUDE, SHUMATE_MAX_LATITUDE);

  g_object_notify (G_OBJECT (location), "latitude");
  g_object_notify (G_OBJECT (location), "longitude");
}


static double
get_latitude (ShumateLocation *location)
{
  ShumateCoordinate *coordinate = SHUMATE_COORDINATE (location);
  ShumateCoordinatePrivate *priv = shumate_coordinate_get_instance_private (coordinate);

  g_return_val_if_fail (SHUMATE_IS_COORDINATE (location), 0.0);

  return priv->latitude;
}


static double
get_longitude (ShumateLocation *location)
{
  ShumateCoordinate *coordinate = SHUMATE_COORDINATE (location);
  ShumateCoordinatePrivate *priv = shumate_coordinate_get_instance_private (coordinate);

  g_return_val_if_fail (SHUMATE_IS_COORDINATE (location), 0.0);

  return priv->longitude;
}


static void
location_interface_init (ShumateLocationInterface *iface)
{
  iface->get_latitude = get_latitude;
  iface->get_longitude = get_longitude;
  iface->set_location = set_location;
}

static void
shumate_coordinate_class_init (ShumateCoordinateClass *coordinate_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (coordinate_class);
  object_class->get_property = shumate_coordinate_get_property;
  object_class->set_property = shumate_coordinate_set_property;

  g_object_class_override_property (object_class,
      PROP_LONGITUDE,
      "longitude");

  g_object_class_override_property (object_class,
      PROP_LATITUDE,
      "latitude");
}


static void
shumate_coordinate_init (ShumateCoordinate *coordinate)
{
  ShumateCoordinatePrivate *priv = shumate_coordinate_get_instance_private (coordinate);

  priv->latitude = 0.0;
  priv->longitude = 0.0;
}


/**
 * shumate_coordinate_new:
 *
 * Creates a new instance of #ShumateCoordinate.
 *
 * Returns: the created instance.
 */
ShumateCoordinate *
shumate_coordinate_new ()
{
  return SHUMATE_COORDINATE (g_object_new (SHUMATE_TYPE_COORDINATE, NULL));
}


/**
 * shumate_coordinate_new_full:
 * @latitude: the latitude coordinate
 * @longitude: the longitude coordinate
 *
 * Creates a new instance of #ShumateCoordinate initialized with the given
 * coordinates.
 *
 * Returns: the created instance.
 */
ShumateCoordinate *
shumate_coordinate_new_full (double latitude,
    double longitude)
{
  return SHUMATE_COORDINATE (g_object_new (SHUMATE_TYPE_COORDINATE,
          "latitude", latitude,
          "longitude", longitude,
          NULL));
}
