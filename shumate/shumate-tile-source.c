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
 * SECTION:shumate-tile-source
 * @short_description: A base class of tile sources
 *
 * This class defines properties common to all tile sources.
 */

#include "shumate-tile-source.h"
#include "shumate-enum-types.h"

enum
{
  PROP_0,
  PROP_ID,
  PROP_NAME,
  PROP_LICENSE,
  PROP_LICENSE_URI,
  PROP_MIN_ZOOM_LEVEL,
  PROP_MAX_ZOOM_LEVEL,
  PROP_TILE_SIZE,
  PROP_MAP_PROJECTION,
};

typedef struct
{
  char *id;
  char *name;
  char *license;
  char *license_uri;
  guint min_zoom_level;
  guint max_zoom_level;
  guint tile_size;
  ShumateMapProjection map_projection;
} ShumateTileSourcePrivate;

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (ShumateTileSource, shumate_tile_source, SHUMATE_TYPE_MAP_SOURCE);

static const char *get_id (ShumateMapSource *map_source);
static const char *get_name (ShumateMapSource *map_source);
static const char *get_license (ShumateMapSource *map_source);
static const char *get_license_uri (ShumateMapSource *map_source);
static guint get_min_zoom_level (ShumateMapSource *map_source);
static guint get_max_zoom_level (ShumateMapSource *map_source);
static guint get_tile_size (ShumateMapSource *map_source);
static ShumateMapProjection get_projection (ShumateMapSource *map_source);

static void
shumate_tile_source_get_property (GObject *object,
    guint prop_id,
    GValue *value,
    GParamSpec *pspec)
{
  ShumateTileSource *tile_source = SHUMATE_TILE_SOURCE (object);
  ShumateTileSourcePrivate *priv = shumate_tile_source_get_instance_private (tile_source);

  switch (prop_id)
    {
    case PROP_ID:
      g_value_set_string (value, priv->id);
      break;

    case PROP_NAME:
      g_value_set_string (value, priv->name);
      break;

    case PROP_LICENSE:
      g_value_set_string (value, priv->license);
      break;

    case PROP_LICENSE_URI:
      g_value_set_string (value, priv->license_uri);
      break;

    case PROP_MIN_ZOOM_LEVEL:
      g_value_set_uint (value, priv->min_zoom_level);
      break;

    case PROP_MAX_ZOOM_LEVEL:
      g_value_set_uint (value, priv->max_zoom_level);
      break;

    case PROP_TILE_SIZE:
      g_value_set_uint (value, priv->tile_size);
      break;

    case PROP_MAP_PROJECTION:
      g_value_set_enum (value, priv->map_projection);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}


static void
shumate_tile_source_set_property (GObject *object,
    guint prop_id,
    const GValue *value,
    GParamSpec *pspec)
{
  ShumateTileSource *tile_source = SHUMATE_TILE_SOURCE (object);

  switch (prop_id)
    {
    case PROP_ID:
      shumate_tile_source_set_id (tile_source,
          g_value_get_string (value));

    case PROP_NAME:
      shumate_tile_source_set_name (tile_source,
          g_value_get_string (value));
      break;

    case PROP_LICENSE:
      shumate_tile_source_set_license (tile_source,
          g_value_get_string (value));
      break;

    case PROP_LICENSE_URI:
      shumate_tile_source_set_license_uri (tile_source,
          g_value_get_string (value));
      break;

    case PROP_MIN_ZOOM_LEVEL:
      shumate_tile_source_set_min_zoom_level (tile_source,
          g_value_get_uint (value));
      break;

    case PROP_MAX_ZOOM_LEVEL:
      shumate_tile_source_set_max_zoom_level (tile_source,
          g_value_get_uint (value));
      break;

    case PROP_TILE_SIZE:
      shumate_tile_source_set_tile_size (tile_source,
          g_value_get_uint (value));
      break;

    case PROP_MAP_PROJECTION:
      shumate_tile_source_set_projection (tile_source,
          g_value_get_enum (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}


static void
shumate_tile_source_finalize (GObject *object)
{
  ShumateTileSource *tile_source = SHUMATE_TILE_SOURCE (object);
  ShumateTileSourcePrivate *priv = shumate_tile_source_get_instance_private (tile_source);

  g_clear_pointer (&priv->id, g_free);
  g_clear_pointer (&priv->name, g_free);
  g_clear_pointer (&priv->license, g_free);
  g_clear_pointer (&priv->license_uri, g_free);

  G_OBJECT_CLASS (shumate_tile_source_parent_class)->finalize (object);
}

static void
shumate_tile_source_class_init (ShumateTileSourceClass *klass)
{
  ShumateMapSourceClass *map_source_class = SHUMATE_MAP_SOURCE_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  object_class->finalize = shumate_tile_source_finalize;
  object_class->get_property = shumate_tile_source_get_property;
  object_class->set_property = shumate_tile_source_set_property;

  map_source_class->get_id = get_id;
  map_source_class->get_name = get_name;
  map_source_class->get_license = get_license;
  map_source_class->get_license_uri = get_license_uri;
  map_source_class->get_min_zoom_level = get_min_zoom_level;
  map_source_class->get_max_zoom_level = get_max_zoom_level;
  map_source_class->get_tile_size = get_tile_size;
  map_source_class->get_projection = get_projection;

  map_source_class->fill_tile = NULL;

  /**
   * ShumateTileSource:id:
   *
   * The id of the tile source
   */
  pspec = g_param_spec_string ("id",
        "Id",
        "The id of the tile source",
        "",
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_ID, pspec);

  /**
   * ShumateTileSource:name:
   *
   * The name of the tile source
   */
  pspec = g_param_spec_string ("name",
        "Name",
        "The name of the tile source",
        "",
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_NAME, pspec);

  /**
   * ShumateTileSource:license:
   *
   * The usage license of the tile source
   */
  pspec = g_param_spec_string ("license",
        "License",
        "The usage license of the tile source",
        "",
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_LICENSE, pspec);

  /**
   * ShumateTileSource:license-uri:
   *
   * The usage license's uri for more information
   */
  pspec = g_param_spec_string ("license-uri",
        "License-uri",
        "The usage license's uri for more information",
        "",
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_LICENSE_URI, pspec);

  /**
   * ShumateTileSource:min-zoom-level:
   *
   * The minimum zoom level
   */
  pspec = g_param_spec_uint ("min-zoom-level",
        "Minimum Zoom Level",
        "The minimum zoom level",
        0,
        50,
        0,
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_MIN_ZOOM_LEVEL, pspec);

  /**
   * ShumateTileSource:max-zoom-level:
   *
   * The maximum zoom level
   */
  pspec = g_param_spec_uint ("max-zoom-level",
        "Maximum Zoom Level",
        "The maximum zoom level",
        0,
        50,
        18,
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_MAX_ZOOM_LEVEL, pspec);

  /**
   * ShumateTileSource:tile-size:
   *
   * The tile size of the tile source
   */
  pspec = g_param_spec_uint ("tile-size",
        "Tile Size",
        "The tile size",
        0,
        2048,
        256,
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_TILE_SIZE, pspec);

  /**
   * ShumateTileSource:projection:
   *
   * The map projection of the tile source
   */
  pspec = g_param_spec_enum ("projection",
        "Projection",
        "The map projection",
        SHUMATE_TYPE_MAP_PROJECTION,
        SHUMATE_MAP_PROJECTION_MERCATOR,
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_MAP_PROJECTION, pspec);
}


static void
shumate_tile_source_init (ShumateTileSource *tile_source)
{
  ShumateTileSourcePrivate *priv = shumate_tile_source_get_instance_private (tile_source);

  priv->id = NULL;
  priv->name = NULL;
  priv->license = NULL;
  priv->license_uri = NULL;
  priv->min_zoom_level = 0;
  priv->max_zoom_level = 0;
  priv->tile_size = 0;
  priv->map_projection = SHUMATE_MAP_PROJECTION_MERCATOR;
}


static const char *
get_id (ShumateMapSource *map_source)
{
  ShumateTileSource *tile_source = SHUMATE_TILE_SOURCE (map_source);
  ShumateTileSourcePrivate *priv = shumate_tile_source_get_instance_private (tile_source);

  g_return_val_if_fail (SHUMATE_IS_TILE_SOURCE (map_source), NULL);

  return priv->id;
}


static const char *
get_name (ShumateMapSource *map_source)
{
  ShumateTileSource *tile_source = SHUMATE_TILE_SOURCE (map_source);
  ShumateTileSourcePrivate *priv = shumate_tile_source_get_instance_private (tile_source);

  g_return_val_if_fail (SHUMATE_IS_TILE_SOURCE (map_source), NULL);

  return priv->name;
}


static const char *
get_license (ShumateMapSource *map_source)
{
  ShumateTileSource *tile_source = SHUMATE_TILE_SOURCE (map_source);
  ShumateTileSourcePrivate *priv = shumate_tile_source_get_instance_private (tile_source);

  g_return_val_if_fail (SHUMATE_IS_TILE_SOURCE (map_source), NULL);

  return priv->license;
}


static const char *
get_license_uri (ShumateMapSource *map_source)
{
  ShumateTileSource *tile_source = SHUMATE_TILE_SOURCE (map_source);
  ShumateTileSourcePrivate *priv = shumate_tile_source_get_instance_private (tile_source);

  g_return_val_if_fail (SHUMATE_IS_TILE_SOURCE (map_source), NULL);

  return priv->license_uri;
}


static guint
get_min_zoom_level (ShumateMapSource *map_source)
{
  ShumateTileSource *tile_source = SHUMATE_TILE_SOURCE (map_source);
  ShumateTileSourcePrivate *priv = shumate_tile_source_get_instance_private (tile_source);

  g_return_val_if_fail (SHUMATE_IS_TILE_SOURCE (map_source), 0);

  return priv->min_zoom_level;
}


static guint
get_max_zoom_level (ShumateMapSource *map_source)
{
  ShumateTileSource *tile_source = SHUMATE_TILE_SOURCE (map_source);
  ShumateTileSourcePrivate *priv = shumate_tile_source_get_instance_private (tile_source);

  g_return_val_if_fail (SHUMATE_IS_TILE_SOURCE (map_source), 0);

  return priv->max_zoom_level;
}


static guint
get_tile_size (ShumateMapSource *map_source)
{
  ShumateTileSource *tile_source = SHUMATE_TILE_SOURCE (map_source);
  ShumateTileSourcePrivate *priv = shumate_tile_source_get_instance_private (tile_source);

  g_return_val_if_fail (SHUMATE_IS_TILE_SOURCE (map_source), 0);

  return priv->tile_size;
}


static ShumateMapProjection
get_projection (ShumateMapSource *map_source)
{
  ShumateTileSource *tile_source = SHUMATE_TILE_SOURCE (map_source);
  ShumateTileSourcePrivate *priv = shumate_tile_source_get_instance_private (tile_source);

  g_return_val_if_fail (SHUMATE_IS_TILE_SOURCE (map_source), 0);

  return priv->map_projection;
}


/**
 * shumate_tile_source_set_id:
 * @tile_source: a #ShumateTileSource
 * @id: an id
 *
 * Sets the tile source's id.
 */
void
shumate_tile_source_set_id (ShumateTileSource *tile_source,
    const char *id)
{
  ShumateTileSourcePrivate *priv = shumate_tile_source_get_instance_private (tile_source);

  g_return_if_fail (SHUMATE_IS_TILE_SOURCE (tile_source));

  g_free (priv->id);
  priv->id = g_strdup (id);

  g_object_notify (G_OBJECT (tile_source), "id");
}


/**
 * shumate_tile_source_set_name:
 * @tile_source: a #ShumateTileSource
 * @name: a name
 *
 * Sets the tile source's name.
 */
void
shumate_tile_source_set_name (ShumateTileSource *tile_source,
    const char *name)
{
  ShumateTileSourcePrivate *priv = shumate_tile_source_get_instance_private (tile_source);

  g_return_if_fail (SHUMATE_IS_TILE_SOURCE (tile_source));

  g_free (priv->name);
  priv->name = g_strdup (name);

  g_object_notify (G_OBJECT (tile_source), "name");
}


/**
 * shumate_tile_source_set_license:
 * @tile_source: a #ShumateTileSource
 * @license: the licence
 *
 * Sets the tile source's license.
 */
void
shumate_tile_source_set_license (ShumateTileSource *tile_source,
    const char *license)
{
  ShumateTileSourcePrivate *priv = shumate_tile_source_get_instance_private (tile_source);

  g_return_if_fail (SHUMATE_IS_TILE_SOURCE (tile_source));

  g_free (priv->license);
  priv->license = g_strdup (license);

  g_object_notify (G_OBJECT (tile_source), "license");
}


/**
 * shumate_tile_source_set_license_uri:
 * @tile_source: a #ShumateTileSource
 * @license_uri: the licence URI
 *
 * Sets the tile source's license URI.
 */
void
shumate_tile_source_set_license_uri (ShumateTileSource *tile_source,
    const char *license_uri)
{
  ShumateTileSourcePrivate *priv = shumate_tile_source_get_instance_private (tile_source);

  g_return_if_fail (SHUMATE_IS_TILE_SOURCE (tile_source));

  g_free (priv->license_uri);
  priv->license_uri = g_strdup (license_uri);

  g_object_notify (G_OBJECT (tile_source), "license-uri");
}


/**
 * shumate_tile_source_set_min_zoom_level:
 * @tile_source: a #ShumateTileSource
 * @zoom_level: the minimal zoom level
 *
 * Sets the tile source's minimal zoom level.
 */
void
shumate_tile_source_set_min_zoom_level (ShumateTileSource *tile_source,
    guint zoom_level)
{
  ShumateTileSourcePrivate *priv = shumate_tile_source_get_instance_private (tile_source);

  g_return_if_fail (SHUMATE_IS_TILE_SOURCE (tile_source));

  priv->min_zoom_level = zoom_level;

  g_object_notify (G_OBJECT (tile_source), "min-zoom-level");
}


/**
 * shumate_tile_source_set_max_zoom_level:
 * @tile_source: a #ShumateTileSource
 * @zoom_level: the maximum zoom level
 *
 * Sets the tile source's maximum zoom level.
 */
void
shumate_tile_source_set_max_zoom_level (ShumateTileSource *tile_source,
    guint zoom_level)
{
  ShumateTileSourcePrivate *priv = shumate_tile_source_get_instance_private (tile_source);

  g_return_if_fail (SHUMATE_IS_TILE_SOURCE (tile_source));

  priv->max_zoom_level = zoom_level;

  g_object_notify (G_OBJECT (tile_source), "max-zoom-level");
}


/**
 * shumate_tile_source_set_tile_size:
 * @tile_source: a #ShumateTileSource
 * @tile_size: the tile size
 *
 * Sets the tile source's tile size.
 */
void
shumate_tile_source_set_tile_size (ShumateTileSource *tile_source,
    guint tile_size)
{
  ShumateTileSourcePrivate *priv = shumate_tile_source_get_instance_private (tile_source);

  g_return_if_fail (SHUMATE_IS_TILE_SOURCE (tile_source));

  priv->tile_size = tile_size;

  g_object_notify (G_OBJECT (tile_source), "tile-size");
}


/**
 * shumate_tile_source_set_projection:
 * @tile_source: a #ShumateTileSource
 * @projection: a #ShumateMapProjection
 *
 * Sets the tile source's projection.
 */
void
shumate_tile_source_set_projection (ShumateTileSource *tile_source,
    ShumateMapProjection projection)
{
  ShumateTileSourcePrivate *priv = shumate_tile_source_get_instance_private (tile_source);

  g_return_if_fail (SHUMATE_IS_TILE_SOURCE (tile_source));

  priv->map_projection = projection;

  g_object_notify (G_OBJECT (tile_source), "projection");
}
