/*
 * Copyright (C) 2009 Pierre-Luc Beaudoin <pierre-luc@pierlux.com>
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
 * SECTION:shumate-map-source-desc
 * @short_description: A class that describes map sources.
 *
 * A class that describes map sources.
 */

#include "shumate-map-source-desc.h"

#include "shumate-enum-types.h"

#include "shumate-network-tile-source.h"

enum
{
  PROP_ID = 1,
  PROP_NAME,
  PROP_LICENSE,
  PROP_LICENSE_URI,
  PROP_URI_FORMAT,
  PROP_MIN_ZOOM_LEVEL,
  PROP_MAX_ZOOM_LEVEL,
  PROP_TILE_SIZE,
  PROP_PROJECTION,
  N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

typedef struct
{
  char *id;
  char *name;
  char *license;
  char *license_uri;
  char *uri_format;
  guint min_zoom_level;
  guint max_zoom_level;
  guint tile_size;
  ShumateMapProjection projection;
} ShumateMapSourceDescPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (ShumateMapSourceDesc, shumate_map_source_desc, G_TYPE_OBJECT);

static ShumateMapSource *
shumate_map_source_desc_default_create_source (ShumateMapSourceDesc *self)
{
    return SHUMATE_MAP_SOURCE (shumate_network_tile_source_new_full (
        shumate_map_source_desc_get_id (self),
        shumate_map_source_desc_get_name (self),
        shumate_map_source_desc_get_license (self),
        shumate_map_source_desc_get_license_uri (self),
        shumate_map_source_desc_get_min_zoom_level (self),
        shumate_map_source_desc_get_max_zoom_level (self),
        shumate_map_source_desc_get_tile_size (self),
        shumate_map_source_desc_get_projection (self),
        shumate_map_source_desc_get_uri_format (self)));
}

static void
shumate_map_source_desc_get_property (GObject *object,
    guint prop_id,
    GValue *value,
    GParamSpec *pspec)
{
  ShumateMapSourceDesc *self = SHUMATE_MAP_SOURCE_DESC (object);
  ShumateMapSourceDescPrivate *priv = shumate_map_source_desc_get_instance_private (self);

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

    case PROP_URI_FORMAT:
      g_value_set_string (value, priv->uri_format);
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

    case PROP_PROJECTION:
      g_value_set_enum (value, priv->projection);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}


static void
shumate_map_source_desc_set_property (GObject *object,
    guint prop_id,
    const GValue *value,
    GParamSpec *pspec)
{
  ShumateMapSourceDesc *desc = SHUMATE_MAP_SOURCE_DESC (object);
  ShumateMapSourceDescPrivate *priv = shumate_map_source_desc_get_instance_private (desc);

  switch (prop_id)
    {
    case PROP_ID:
      priv->id = g_value_dup_string (value);
      break;

    case PROP_NAME:
      priv->name = g_value_dup_string (value);
      break;

    case PROP_LICENSE:
      priv->license = g_value_dup_string (value);
      break;

    case PROP_LICENSE_URI:
      priv->license_uri = g_value_dup_string (value);
      break;

    case PROP_URI_FORMAT:
      priv->uri_format = g_value_dup_string (value);
      break;

    case PROP_MIN_ZOOM_LEVEL:
      priv->min_zoom_level = g_value_get_uint (value);
      break;

    case PROP_MAX_ZOOM_LEVEL:
      priv->max_zoom_level = g_value_get_uint (value);
      break;

    case PROP_TILE_SIZE:
      priv->tile_size = g_value_get_uint (value);
      break;

    case PROP_PROJECTION:
      priv->projection = g_value_get_enum (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
shumate_map_source_desc_finalize (GObject *object)
{
  ShumateMapSourceDesc *self = SHUMATE_MAP_SOURCE_DESC (object);
  ShumateMapSourceDescPrivate *priv = shumate_map_source_desc_get_instance_private (self);

  g_clear_pointer (&priv->id, g_free);
  g_clear_pointer (&priv->name, g_free);
  g_clear_pointer (&priv->license, g_free);
  g_clear_pointer (&priv->license_uri, g_free);
  g_clear_pointer (&priv->uri_format, g_free);

  G_OBJECT_CLASS (shumate_map_source_desc_parent_class)->finalize (object);
}


static void
shumate_map_source_desc_class_init (ShumateMapSourceDescClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = shumate_map_source_desc_finalize;
  object_class->get_property = shumate_map_source_desc_get_property;
  object_class->set_property = shumate_map_source_desc_set_property;

  klass->create_source = shumate_map_source_desc_default_create_source;

  /**
   * ShumateMapSourceDesc:id:
   *
   * The id of the map source
   */
  obj_properties[PROP_ID] =
      g_param_spec_string ("id",
          "Map source id",
          "Map source id",
          "",
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  /**
   * ShumateMapSourceDesc:name:
   *
   * The name of the map source
   */
  obj_properties[PROP_NAME] =
      g_param_spec_string ("name",
          "Map source name",
          "Map source name",
          "",
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  /**
   * ShumateMapSourceDesc:license:
   *
   * The license of the map source
   */
  obj_properties[PROP_LICENSE] =
      g_param_spec_string ("license",
          "Map source license",
          "Map source license",
          "",
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  /**
   * ShumateMapSourceDesc:license-uri:
   *
   * The license's uri for more information
   */
  obj_properties[PROP_LICENSE_URI] =
      g_param_spec_string ("license-uri",
          "Map source license URI",
          "Map source license URI",
          "",
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  /**
   * ShumateMapSourceDesc:uri-format:
   *
   * The URI format of a network map source
   */
  obj_properties[PROP_URI_FORMAT] =
      g_param_spec_string ("uri-format",
          "Network map source URI format",
          "Network map source URI format",
          "",
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  /**
   * ShumateMapSourceDesc:min-zoom-level:
   *
   * The minimum zoom level
   */
  obj_properties[PROP_MIN_ZOOM_LEVEL] =
      g_param_spec_uint ("min-zoom-level",
          "Min zoom level",
          "The lowest allowed level of zoom",
          0,
          20,
          0,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  /**
   * ShumateMapSourceDesc:max-zoom-level:
   *
   * The maximum zoom level
   */
  obj_properties[PROP_MAX_ZOOM_LEVEL] =
      g_param_spec_uint ("max-zoom-level",
          "Max zoom level",
          "The highest allowed level of zoom",
          0,
          20,
          20,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  /**
   * ShumateMapSourceDesc:projection:
   *
   * The map projection of the map source
   */
  obj_properties[PROP_PROJECTION] =
      g_param_spec_enum ("projection",
          "Map source projection",
          "Map source projection",
          SHUMATE_TYPE_MAP_PROJECTION,
          SHUMATE_MAP_PROJECTION_MERCATOR,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  /**
   * ShumateMapSourceDesc:tile-size:
   *
   * The tile size of the map source
   */
  obj_properties[PROP_TILE_SIZE] =
      g_param_spec_uint ("tile-size",
          "Tile Size",
          "The size of the map source tile",
          0,
          G_MAXINT,
          256,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class,
                                     N_PROPERTIES,
                                     obj_properties);
}


static void
shumate_map_source_desc_init (ShumateMapSourceDesc *desc)
{
  ShumateMapSourceDescPrivate *priv = shumate_map_source_desc_get_instance_private (desc);

  priv->max_zoom_level = 20;
  priv->tile_size = 256;
  priv->projection = SHUMATE_MAP_PROJECTION_MERCATOR;
}


/**
 * shumate_map_source_desc_new:
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
 * Constructor of #ShumateMapSourceDesc which describes a #ShumateMapSource.
 * This is returned by #shumate_map_source_factory_get_registered
 *
 * Returns: a new #ShumateMapSourceDesc object
 */
ShumateMapSourceDesc *
shumate_map_source_desc_new (
    char *id,
    char *name,
    char *license,
    char *license_uri,
    guint min_zoom,
    guint max_zoom,
    guint tile_size,
    ShumateMapProjection projection,
    char *uri_format)
{
  return g_object_new (SHUMATE_TYPE_MAP_SOURCE_DESC,
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
}

/**
 * shumate_map_source_desc_get_id:
 * @desc: a #ShumateMapSourceDesc
 *
 * Gets map source's id.
 *
 * Returns: the map source's id.
 */
const char *
shumate_map_source_desc_get_id (ShumateMapSourceDesc *desc)
{
  ShumateMapSourceDescPrivate *priv = shumate_map_source_desc_get_instance_private (desc);

  g_return_val_if_fail (SHUMATE_IS_MAP_SOURCE_DESC (desc), NULL);

  return priv->id;
}


/**
 * shumate_map_source_desc_get_name:
 * @desc: a #ShumateMapSourceDesc
 *
 * Gets map source's name.
 *
 * Returns: the map source's name.
 */
const char *
shumate_map_source_desc_get_name (ShumateMapSourceDesc *desc)
{
  ShumateMapSourceDescPrivate *priv = shumate_map_source_desc_get_instance_private (desc);

  g_return_val_if_fail (SHUMATE_IS_MAP_SOURCE_DESC (desc), NULL);

  return priv->name;
}


/**
 * shumate_map_source_desc_get_license:
 * @desc: a #ShumateMapSourceDesc
 *
 * Gets map source's license.
 *
 * Returns: the map source's license.
 */
const char *
shumate_map_source_desc_get_license (ShumateMapSourceDesc *desc)
{
  ShumateMapSourceDescPrivate *priv = shumate_map_source_desc_get_instance_private (desc);

  g_return_val_if_fail (SHUMATE_IS_MAP_SOURCE_DESC (desc), NULL);

  return priv->license;
}


/**
 * shumate_map_source_desc_get_license_uri:
 * @desc: a #ShumateMapSourceDesc
 *
 * Gets map source's license URI.
 *
 * Returns: the map source's license URI.
 */
const char *
shumate_map_source_desc_get_license_uri (ShumateMapSourceDesc *desc)
{
  ShumateMapSourceDescPrivate *priv = shumate_map_source_desc_get_instance_private (desc);

  g_return_val_if_fail (SHUMATE_IS_MAP_SOURCE_DESC (desc), NULL);

  return priv->license_uri;
}


/**
 * shumate_map_source_desc_get_uri_format:
 * @desc: a #ShumateMapSourceDesc
 *
 * Gets network map source's URI format.
 *
 * Returns: the URI format.
 */
const char *
shumate_map_source_desc_get_uri_format (ShumateMapSourceDesc *desc)
{
  ShumateMapSourceDescPrivate *priv = shumate_map_source_desc_get_instance_private (desc);

  g_return_val_if_fail (SHUMATE_IS_MAP_SOURCE_DESC (desc), NULL);

  return priv->uri_format;
}


/**
 * shumate_map_source_desc_get_min_zoom_level:
 * @desc: a #ShumateMapSourceDesc
 *
 * Gets map source's minimum zoom level.
 *
 * Returns: the miminum zoom level this map source supports
 */
guint
shumate_map_source_desc_get_min_zoom_level (ShumateMapSourceDesc *desc)
{
  ShumateMapSourceDescPrivate *priv = shumate_map_source_desc_get_instance_private (desc);

  g_return_val_if_fail (SHUMATE_IS_MAP_SOURCE_DESC (desc), 0);

  return priv->min_zoom_level;
}


/**
 * shumate_map_source_desc_get_max_zoom_level:
 * @desc: a #ShumateMapSourceDesc
 *
 * Gets map source's maximum zoom level.
 *
 * Returns: the maximum zoom level this map source supports
 */
guint
shumate_map_source_desc_get_max_zoom_level (ShumateMapSourceDesc *desc)
{
  ShumateMapSourceDescPrivate *priv = shumate_map_source_desc_get_instance_private (desc);

  g_return_val_if_fail (SHUMATE_IS_MAP_SOURCE_DESC (desc), 0);

  return priv->max_zoom_level;
}


/**
 * shumate_map_source_desc_get_tile_size:
 * @desc: a #ShumateMapSourceDesc
 *
 * Gets map source's tile size.
 *
 * Returns: the tile's size (width and height) in pixels for this map source
 */
guint
shumate_map_source_desc_get_tile_size (ShumateMapSourceDesc *desc)
{
  ShumateMapSourceDescPrivate *priv = shumate_map_source_desc_get_instance_private (desc);

  g_return_val_if_fail (SHUMATE_IS_MAP_SOURCE_DESC (desc), 0);

  return priv->tile_size;
}


/**
 * shumate_map_source_desc_get_projection:
 * @desc: a #ShumateMapSourceDesc
 *
 * Gets map source's projection.
 *
 * Returns: the map source's projection.
 */
ShumateMapProjection
shumate_map_source_desc_get_projection (ShumateMapSourceDesc *desc)
{
  ShumateMapSourceDescPrivate *priv = shumate_map_source_desc_get_instance_private (desc);

  g_return_val_if_fail (SHUMATE_IS_MAP_SOURCE_DESC (desc), SHUMATE_MAP_PROJECTION_MERCATOR);

  return priv->projection;
}

/**
 * shumate_map_source_desc_create_source:
 * @self: a #ShumateMapSourceDesc
 *
 * Creates a #ShumateMapSource describes by @self.
 *
 * Returns: (transfer full): a newly created #ShumateMapSource.
 */
ShumateMapSource *
shumate_map_source_desc_create_source (ShumateMapSourceDesc *self)
{
  g_return_val_if_fail (SHUMATE_IS_MAP_SOURCE_DESC (self), NULL);

  return SHUMATE_MAP_SOURCE_DESC_GET_CLASS (self)->create_source (self);
}
