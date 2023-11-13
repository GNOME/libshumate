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
 * ShumateMapSource:
 *
 * The base class for all map sources. Map sources fill [class@Tile] objects
 * with images from various sources: a web API, for example, or a test pattern
 * generated on demand.
 *
 * The most common map source is [class@RasterRenderer], which fetches tiles
 * using a [class@TileDownloader].
 */

#include "shumate-map-source.h"
#include "shumate-location.h"
#include "shumate-enum-types.h"

#include <math.h>

typedef struct {
  char *id;
  char *name;
  char *license;
  char *license_uri;
  guint min_zoom_level;
  guint max_zoom_level;
  guint tile_size;
  ShumateMapProjection projection;
} ShumateMapSourcePrivate;

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (ShumateMapSource, shumate_map_source, G_TYPE_OBJECT);

enum
{
  PROP_ID = 1,
  PROP_NAME,
  PROP_LICENSE,
  PROP_LICENSE_URI,
  PROP_MIN_ZOOM_LEVEL,
  PROP_MAX_ZOOM_LEVEL,
  PROP_TILE_SIZE,
  PROP_PROJECTION,
  N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static void
shumate_map_source_finalize (GObject *object)
{
  ShumateMapSource *map_source = SHUMATE_MAP_SOURCE (object);
  ShumateMapSourcePrivate *priv = shumate_map_source_get_instance_private (map_source);

  g_clear_pointer (&priv->id, g_free);
  g_clear_pointer (&priv->name, g_free);
  g_clear_pointer (&priv->license, g_free);
  g_clear_pointer (&priv->license_uri, g_free);

  G_OBJECT_CLASS (shumate_map_source_parent_class)->finalize (object);
}

static void
shumate_map_source_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  ShumateMapSource *map_source = SHUMATE_MAP_SOURCE (object);

  switch (prop_id)
    {
      case PROP_ID:
        g_value_set_string (value, shumate_map_source_get_id (map_source));
        break;

      case PROP_NAME:
        g_value_set_string (value, shumate_map_source_get_name (map_source));
        break;

      case PROP_LICENSE:
        g_value_set_string (value, shumate_map_source_get_license (map_source));
        break;

      case PROP_LICENSE_URI:
        g_value_set_string (value, shumate_map_source_get_license_uri (map_source));
        break;

      case PROP_MIN_ZOOM_LEVEL:
        g_value_set_uint (value, shumate_map_source_get_min_zoom_level (map_source));
        break;

      case PROP_MAX_ZOOM_LEVEL:
        g_value_set_uint (value, shumate_map_source_get_max_zoom_level (map_source));
        break;

      case PROP_TILE_SIZE:
        g_value_set_uint (value, shumate_map_source_get_tile_size (map_source));
        break;

      case PROP_PROJECTION:
        g_value_set_enum (value, shumate_map_source_get_projection (map_source));
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}


static void
shumate_map_source_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  ShumateMapSource *map_source = SHUMATE_MAP_SOURCE (object);

  switch (prop_id)
    {
      case PROP_ID:
        shumate_map_source_set_id (map_source,
                                   g_value_get_string (value));
        break;

      case PROP_NAME:
        shumate_map_source_set_name (map_source,
                                     g_value_get_string (value));
        break;

      case PROP_LICENSE:
        shumate_map_source_set_license (map_source,
                                        g_value_get_string (value));
        break;

      case PROP_LICENSE_URI:
        shumate_map_source_set_license_uri (map_source,
                                            g_value_get_string (value));
        break;

      case PROP_MIN_ZOOM_LEVEL:
        shumate_map_source_set_min_zoom_level (map_source,
                                               g_value_get_uint (value));
        break;

      case PROP_MAX_ZOOM_LEVEL:
        shumate_map_source_set_max_zoom_level (map_source,
                                               g_value_get_uint (value));
        break;

      case PROP_TILE_SIZE:
        shumate_map_source_set_tile_size (map_source,
                                          g_value_get_uint (value));
        break;

      case PROP_PROJECTION:
        shumate_map_source_set_projection (map_source,
                                           g_value_get_enum (value));
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
shumate_map_source_class_init (ShumateMapSourceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = shumate_map_source_set_property;
  object_class->get_property = shumate_map_source_get_property;
  object_class->finalize = shumate_map_source_finalize;

  klass->fill_tile_async = NULL;

  /**
   * ShumateMapSource:id:
   *
   * The id of the map source
   */
  obj_properties[PROP_ID] =
    g_param_spec_string ("id",
                         "Id",
                         "The id of the map source",
                         NULL,
                         G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * ShumateMapSource:name:
   *
   * The name of the map source
   */
  obj_properties[PROP_NAME] =
    g_param_spec_string ("name",
                         "Name",
                         "The name of the map source",
                         NULL,
                         G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * ShumateMapSource:license:
   *
   * The usage license of the map source
   */
  obj_properties[PROP_LICENSE] =
    g_param_spec_string ("license",
                         "License",
                         "The usage license of the map source",
                         NULL,
                         G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * ShumateMapSource:license-uri:
   *
   * The usage license's uri for more information
   */
  obj_properties[PROP_LICENSE_URI] =
    g_param_spec_string ("license-uri",
                         "License-uri",
                         "The usage license's uri for more information",
                         NULL,
                         G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * ShumateMapSource:min-zoom-level:
   *
   * The minimum zoom level
   */
  obj_properties[PROP_MIN_ZOOM_LEVEL] =
    g_param_spec_uint ("min-zoom-level",
                       "Minimum Zoom Level",
                       "The minimum zoom level",
                       0, 50, 0,
                       G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * ShumateMapSource:max-zoom-level:
   *
   * The maximum zoom level
   */
  obj_properties[PROP_MAX_ZOOM_LEVEL] =
    g_param_spec_uint ("max-zoom-level",
                       "Maximum Zoom Level",
                       "The maximum zoom level",
                       0, 50, 18,
                       G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * ShumateMapSource:tile-size:
   *
   * The tile size of the map source
   */
  obj_properties[PROP_TILE_SIZE] =
    g_param_spec_uint ("tile-size",
                       "Tile Size",
                       "The map size",
                       0, 2048, 256,
                       G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * ShumateMapSource:projection:
   *
   * The map projection of the map source
   */
  obj_properties[PROP_PROJECTION] =
    g_param_spec_enum ("projection",
                       "Projection",
                       "The map projection",
                       SHUMATE_TYPE_MAP_PROJECTION,
                       SHUMATE_MAP_PROJECTION_MERCATOR,
                       G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class,
                                     N_PROPERTIES,
                                     obj_properties);
}

static double
map_size (ShumateMapSource *self, double zoom_level)
{
  return shumate_map_source_get_column_count (self, zoom_level) * shumate_map_source_get_tile_size_at_zoom (self, zoom_level);
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
  ShumateMapSourcePrivate *priv = shumate_map_source_get_instance_private (map_source);

  g_return_val_if_fail (SHUMATE_IS_MAP_SOURCE (map_source), NULL);

  return priv->id;
}

/**
 * shumate_map_source_set_id:
 * @map_source: a #ShumateMapSource
 * @id: an id
 *
 * Sets the map source's id.
 */
void
shumate_map_source_set_id (ShumateMapSource *map_source,
                           const char       *id)
{
  ShumateMapSourcePrivate *priv = shumate_map_source_get_instance_private (map_source);

  g_return_if_fail (SHUMATE_IS_MAP_SOURCE (map_source));

  if (g_strcmp0 (priv->id, id) != 0)
    {
      g_free (priv->id);
      priv->id = g_strdup (id);

      g_object_notify_by_pspec (G_OBJECT (map_source), obj_properties[PROP_ID]);
    }
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
  ShumateMapSourcePrivate *priv = shumate_map_source_get_instance_private (map_source);

  g_return_val_if_fail (SHUMATE_IS_MAP_SOURCE (map_source), NULL);

  return priv->name;
}

/**
 * shumate_map_source_set_name:
 * @map_source: a #ShumateMapSource
 * @name: a name
 *
 * Sets the map source's name.
 */
void
shumate_map_source_set_name (ShumateMapSource *map_source,
                             const char       *name)
{
  ShumateMapSourcePrivate *priv = shumate_map_source_get_instance_private (map_source);

  g_return_if_fail (SHUMATE_IS_MAP_SOURCE (map_source));

  if (g_strcmp0 (priv->name, name) != 0)
    {
      g_free (priv->name);
      priv->name = g_strdup (name);

      g_object_notify_by_pspec (G_OBJECT (map_source), obj_properties[PROP_NAME]);
    }
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
  ShumateMapSourcePrivate *priv = shumate_map_source_get_instance_private (map_source);

  g_return_val_if_fail (SHUMATE_IS_MAP_SOURCE (map_source), NULL);

  return priv->license;
}

/**
 * shumate_map_source_set_license:
 * @map_source: a #ShumateMapSource
 * @license: the licence
 *
 * Sets the map source's license.
 */
void
shumate_map_source_set_license (ShumateMapSource *map_source,
                                const char       *license)
{
  ShumateMapSourcePrivate *priv = shumate_map_source_get_instance_private (map_source);

  g_return_if_fail (SHUMATE_IS_MAP_SOURCE (map_source));

  if (g_strcmp0 (priv->license, license) != 0)
    {
      g_free (priv->license);
      priv->license = g_strdup (license);

      g_object_notify_by_pspec (G_OBJECT (map_source), obj_properties[PROP_LICENSE]);
    }
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
  ShumateMapSourcePrivate *priv = shumate_map_source_get_instance_private (map_source);

  g_return_val_if_fail (SHUMATE_IS_MAP_SOURCE (map_source), NULL);

  return priv->license_uri;
}

/**
 * shumate_map_source_set_license_uri:
 * @map_source: a #ShumateMapSource
 * @license_uri: the licence URI
 *
 * Sets the map source's license URI.
 */
void
shumate_map_source_set_license_uri (ShumateMapSource *map_source,
                                    const char       *license_uri)
{
  ShumateMapSourcePrivate *priv = shumate_map_source_get_instance_private (map_source);

  g_return_if_fail (SHUMATE_IS_MAP_SOURCE (map_source));

  if (g_strcmp0 (priv->license_uri, license_uri) != 0)
    {
      g_free (priv->license_uri);
      priv->license_uri = g_strdup (license_uri);

      g_object_notify_by_pspec (G_OBJECT (map_source), obj_properties[PROP_LICENSE_URI]);
    }
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
  ShumateMapSourcePrivate *priv = shumate_map_source_get_instance_private (map_source);

  g_return_val_if_fail (SHUMATE_IS_MAP_SOURCE (map_source), 0);

  return priv->min_zoom_level;
}

/**
 * shumate_map_source_set_min_zoom_level:
 * @map_source: a #ShumateMapSource
 * @zoom_level: the minimal zoom level
 *
 * Sets the map source's minimal zoom level.
 */
void
shumate_map_source_set_min_zoom_level (ShumateMapSource *map_source,
                                       guint             zoom_level)
{
  ShumateMapSourcePrivate *priv = shumate_map_source_get_instance_private (map_source);

  g_return_if_fail (SHUMATE_IS_MAP_SOURCE (map_source));

  if (priv->min_zoom_level != zoom_level)
    {
      priv->min_zoom_level = zoom_level;

      g_object_notify_by_pspec (G_OBJECT (map_source), obj_properties[PROP_MIN_ZOOM_LEVEL]);
    }
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
  ShumateMapSourcePrivate *priv = shumate_map_source_get_instance_private (map_source);

  g_return_val_if_fail (SHUMATE_IS_MAP_SOURCE (map_source), 0);

  return priv->max_zoom_level;
}

/**
 * shumate_map_source_set_max_zoom_level:
 * @map_source: a #ShumateMapSource
 * @zoom_level: the maximum zoom level
 *
 * Sets the map source's maximum zoom level.
 */
void
shumate_map_source_set_max_zoom_level (ShumateMapSource *map_source,
                                       guint             zoom_level)
{
  ShumateMapSourcePrivate *priv = shumate_map_source_get_instance_private (map_source);

  g_return_if_fail (SHUMATE_IS_MAP_SOURCE (map_source));

  if (priv->max_zoom_level != zoom_level)
    {
      priv->max_zoom_level = zoom_level;

      g_object_notify_by_pspec (G_OBJECT (map_source), obj_properties[PROP_MAX_ZOOM_LEVEL]);
    }
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
  ShumateMapSourcePrivate *priv = shumate_map_source_get_instance_private (map_source);

  g_return_val_if_fail (SHUMATE_IS_MAP_SOURCE (map_source), 0);

  return priv->tile_size;
}

/**
 * shumate_map_source_get_tile_size_at_zoom:
 * @map_source: a #ShumateMapSource
 * @zoom_level: a zoom level
 *
 * Gets the apparent size of the map tiles at the given fractional zoom level.
 *
 * As the map is zoomed in, a tile gets bigger and bigger until, at the next
 * integer zoom level, it "splits" into four tiles at the next zoom level.
 * Thus, the size increase follows an exponential curve, base 2.
 *
 * Returns: the tile's size (width and height) in pixels for this map source
 * at this zoom level
 */
double
shumate_map_source_get_tile_size_at_zoom (ShumateMapSource *map_source,
                                          double            zoom_level)
{
  ShumateMapSourcePrivate *priv = shumate_map_source_get_instance_private (map_source);

  g_return_val_if_fail (SHUMATE_IS_MAP_SOURCE (map_source), 0);

  return priv->tile_size * pow (2.0, fmod (zoom_level, 1.0));
}

/**
 * shumate_map_source_set_tile_size:
 * @map_source: a #ShumateMapSource
 * @tile_size: the tile size
 *
 * Sets the map source's tile size.
 */
void
shumate_map_source_set_tile_size (ShumateMapSource *map_source,
                                  guint             tile_size)
{
  ShumateMapSourcePrivate *priv = shumate_map_source_get_instance_private (map_source);

  g_return_if_fail (SHUMATE_IS_MAP_SOURCE (map_source));

  if (priv->tile_size != tile_size)
    {
      priv->tile_size = tile_size;

      g_object_notify_by_pspec (G_OBJECT (map_source), obj_properties[PROP_TILE_SIZE]);
    }
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
  ShumateMapSourcePrivate *priv = shumate_map_source_get_instance_private (map_source);

  g_return_val_if_fail (SHUMATE_IS_MAP_SOURCE (map_source), SHUMATE_MAP_PROJECTION_MERCATOR);

  return priv->projection;
}

/**
 * shumate_map_source_set_projection:
 * @map_source: a #ShumateMapSource
 * @projection: a #ShumateMapProjection
 *
 * Sets the map source's projection.
 */
void
shumate_map_source_set_projection (ShumateMapSource     *map_source,
                                   ShumateMapProjection  projection)
{
  ShumateMapSourcePrivate *priv = shumate_map_source_get_instance_private (map_source);

  g_return_if_fail (SHUMATE_IS_MAP_SOURCE (map_source));

  if (priv->projection != projection)
    {
      priv->projection = projection;

      g_object_notify_by_pspec (G_OBJECT (map_source), obj_properties[PROP_PROJECTION]);
    }
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
shumate_map_source_fill_tile_async (ShumateMapSource    *self,
                                    ShumateTile         *tile,
                                    GCancellable        *cancellable,
                                    GAsyncReadyCallback  callback,
                                    gpointer             user_data)
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
shumate_map_source_fill_tile_finish (ShumateMapSource  *self,
                                     GAsyncResult      *result,
                                     GError           **error)
{
  g_return_val_if_fail (SHUMATE_IS_MAP_SOURCE (self), FALSE);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (result), FALSE);

  return SHUMATE_MAP_SOURCE_GET_CLASS (self)->fill_tile_finish (self, result, error);
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
                          double            zoom_level,
                          double            longitude)
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
                          double            zoom_level,
                          double            latitude)
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
                                  double            zoom_level,
                                  double            x)
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
                                 double            zoom_level,
                                 double            y)
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
                                  guint             zoom_level)
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
                                     guint             zoom_level)
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
shumate_map_source_get_meters_per_pixel (ShumateMapSource    *map_source,
                                         double               zoom_level,
                                         double               latitude,
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
