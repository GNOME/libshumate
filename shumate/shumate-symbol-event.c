/*
 * Copyright (C) 2022 James Westman <james@jwestman.net>
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
 * License along with this library; if not, see <https://www.gnu.org/licenses/>.
 */

#include "shumate-location.h"
#include "shumate-symbol-event-private.h"

/**
 * ShumateSymbolEvent:
 *
 * An object containing the details of a map feature that has been clicked.
 * It is the argument of the [signal@MapLayer::symbol-clicked] and
 * [signal@SimpleMap::symbol-clicked] signals.
 *
 * When vector maps are rendered, they may contain labels and icons. When one
 * of these symbols is clicked, these signals are emitted to give the
 * application access to the original location and details of the map feature.
 *
 * [class@SymbolEvent] implements [iface@Location] so you can get the latitude
 * and longitude of the feature that was clicked.
 */

struct _ShumateSymbolEvent
{
  GObject parent_instance;

  char *layer;
  char *feature_id;
  double lat, lon;
  GHashTable *tags;
};

static void location_interface_init (ShumateLocationInterface *iface);

G_DEFINE_FINAL_TYPE_WITH_CODE (ShumateSymbolEvent, shumate_symbol_event, G_TYPE_OBJECT,
                               G_IMPLEMENT_INTERFACE (SHUMATE_TYPE_LOCATION, location_interface_init))

enum {
  PROP_0,
  PROP_LAYER,
  PROP_FEATURE_ID,
  N_PROPS,

  PROP_LONGITUDE,
  PROP_LATITUDE,
};

static GParamSpec *properties [N_PROPS];

static void
shumate_symbol_event_finalize (GObject *object)
{
  ShumateSymbolEvent *self = (ShumateSymbolEvent *)object;

  g_clear_pointer (&self->layer, g_free);
  g_clear_pointer (&self->feature_id, g_free);
  g_clear_pointer (&self->tags, g_hash_table_unref);

  G_OBJECT_CLASS (shumate_symbol_event_parent_class)->finalize (object);
}

static void
shumate_symbol_event_get_property (GObject    *object,
                                   guint       prop_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  ShumateSymbolEvent *self = SHUMATE_SYMBOL_EVENT (object);

  switch (prop_id)
    {
    case PROP_LAYER:
      g_value_set_string (value, self->layer);
      break;
    case PROP_FEATURE_ID:
      g_value_set_string (value, self->feature_id);
      break;
    case PROP_LATITUDE:
      g_value_set_double (value, self->lat);
      break;
    case PROP_LONGITUDE:
      g_value_set_double (value, self->lon);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
shumate_symbol_event_set_property (GObject      *object,
                                   guint         prop_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  switch (prop_id)
    {
    case PROP_LATITUDE:
    case PROP_LONGITUDE:
      g_warning ("Cannot change the location of a ShumateSymbolEvent");
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
shumate_symbol_event_class_init (ShumateSymbolEventClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = shumate_symbol_event_finalize;
  object_class->get_property = shumate_symbol_event_get_property;
  object_class->set_property = shumate_symbol_event_set_property;

  /**
   * ShumateSymbolEvent:layer:
   *
   * The ID of the style layer of the symbol that this event pertains to.
   */
  properties[PROP_LAYER] =
    g_param_spec_string ("layer",
                         "layer",
                         "layer",
                         NULL,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  /**
   * ShumateSymbolEvent:feature-id:
   *
   * The ID of the feature that this event pertains to, as it was given in the
   * data source.
   */
  properties[PROP_FEATURE_ID] =
    g_param_spec_string ("feature-id",
                         "Feature ID",
                         "Feature ID",
                         NULL,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);

  g_object_class_override_property (object_class,
      PROP_LONGITUDE,
      "longitude");

  g_object_class_override_property (object_class,
      PROP_LATITUDE,
      "latitude");
}

static void
shumate_symbol_event_init (ShumateSymbolEvent *self)
{
  self->tags = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
}

static double
shumate_symbol_event_get_latitude (ShumateLocation *location)
{
  ShumateSymbolEvent *self = (ShumateSymbolEvent *) location;
  return self->lat;
}

static double
shumate_symbol_event_get_longitude (ShumateLocation *location)
{
  ShumateSymbolEvent *self = (ShumateSymbolEvent *) location;
  return self->lon;
}

static void
shumate_symbol_event_set_location (ShumateLocation *location,
                                   double           lat,
                                   double           lon)
{
  g_warning ("Cannot change the location of a ShumateSymbolEvent");
}

static void
location_interface_init (ShumateLocationInterface *iface)
{
  iface->get_latitude = shumate_symbol_event_get_latitude;
  iface->get_longitude = shumate_symbol_event_get_longitude;
  iface->set_location = shumate_symbol_event_set_location;
}

/**
 * shumate_symbol_event_get_layer:
 * @self: a [class@SymbolEvent]
 *
 * Gets the name of the layer the clicked symbol is in, as named in the vector
 * stylesheet.
 *
 * Note that this is distinct from the name of the layer in the vector tile
 * schema. Some styles have multiple symbol layers derived from the same
 * data source layer.
 *
 * Returns: (transfer none): the layer name
 */
const char *
shumate_symbol_event_get_layer (ShumateSymbolEvent *self)
{
  g_return_val_if_fail (SHUMATE_IS_SYMBOL_EVENT (self), NULL);
  return self->layer;
}

/**
 * shumate_symbol_event_get_feature_id:
 * @self: a [class@SymbolEvent]
 *
 * Gets the feature ID as specified in the data source. The meaning of the
 * ID, if any, is up to the source.
 *
 * Feature IDs in Mapbox Vector Tile format are integers, but they are
 * formatted as a string here for futureproofing.
 *
 * Returns: (transfer none): the feature ID
 */
const char *
shumate_symbol_event_get_feature_id (ShumateSymbolEvent *self)
{
  g_return_val_if_fail (SHUMATE_IS_SYMBOL_EVENT (self), NULL);
  return self->feature_id;
}

/**
 * shumate_symbol_event_get_tag:
 * @self: a [class@SymbolEvent]
 * @tag_name: the tag to get
 *
 * Gets a tag from the source feature.
 *
 * The available tags depend on the vector tile schema and the source layer.
 * Check the documentation for the tiles you're using to see what information
 * is available.
 *
 * Returns: (transfer none): the tag value, formatted as a string
 */
const char *
shumate_symbol_event_get_tag (ShumateSymbolEvent *self,
                              const char         *tag_name)
{
  g_return_val_if_fail (SHUMATE_IS_SYMBOL_EVENT (self), NULL);
  return g_hash_table_lookup (self->tags, tag_name);
}

ShumateSymbolEvent *
shumate_symbol_event_new (const char *layer,
                          const char *feature_id,
                          GHashTable *tags)
{
  ShumateSymbolEvent *self = g_object_new (SHUMATE_TYPE_SYMBOL_EVENT, NULL);

  self->layer = g_strdup (layer);
  self->feature_id = g_strdup (feature_id);
  self->tags = g_hash_table_ref (tags);

  return self;
}

void
shumate_symbol_event_set_lat_lon (ShumateSymbolEvent *self,
                                  double              lat,
                                  double              lon)
{
  self->lat = lat;
  self->lon = lon;
}
