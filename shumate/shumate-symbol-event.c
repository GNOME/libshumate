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
 *
 * Since: 1.1
 */

struct _ShumateSymbolEvent
{
  GObject parent_instance;

  char *layer;
  char *source_layer;
  char *feature_id;
  double lat, lon;
  GHashTable *tags;
  gint n_press;
};

static void location_interface_init (ShumateLocationInterface *iface);

G_DEFINE_FINAL_TYPE_WITH_CODE (ShumateSymbolEvent, shumate_symbol_event, G_TYPE_OBJECT,
                               G_IMPLEMENT_INTERFACE (SHUMATE_TYPE_LOCATION, location_interface_init))

enum {
  PROP_0,
  PROP_LAYER,
  PROP_SOURCE_LAYER,
  PROP_FEATURE_ID,
  PROP_N_PRESS,
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
  g_clear_pointer (&self->source_layer, g_free);
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
    case PROP_SOURCE_LAYER:
      g_value_set_string (value, self->source_layer);
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
    case PROP_N_PRESS:
      g_value_set_int (value, self->n_press);
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
  ShumateSymbolEvent *symbol_event = SHUMATE_SYMBOL_EVENT (object);

  switch (prop_id)
    {
    case PROP_LATITUDE:
    case PROP_LONGITUDE:
      g_warning ("Cannot change the location of a ShumateSymbolEvent");
      break;
    case PROP_N_PRESS:
      shumate_symbol_event_set_n_press (symbol_event, g_value_get_int (value));
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
   *
   * Since: 1.1
   */
  properties[PROP_LAYER] =
    g_param_spec_string ("layer",
                         "layer",
                         "layer",
                         NULL,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  /**
   * ShumateSymbolEvent:source-layer:
   *
   * The ID of the source layer of the symbol that this event pertains to.
   */
  properties[PROP_SOURCE_LAYER] =
    g_param_spec_string ("source-layer",
                         "source-layer",
                         "source-layer",
                         NULL,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  /**
   * ShumateSymbolEvent:feature-id:
   *
   * The ID of the feature that this event pertains to, as it was given in the
   * data source.
   *
   * Since: 1.1
   */
  properties[PROP_FEATURE_ID] =
    g_param_spec_string ("feature-id",
                         "Feature ID",
                         "Feature ID",
                         NULL,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  /**
   * ShumateSymbolEvent:n-press:
   *
   * The number of clicks/presses triggering the symbol event.
   *
   * Since: 1.5
   */
  properties[PROP_N_PRESS] =
    g_param_spec_uint ("n-press",
                       "Number of presses",
                       "Number of presses",
                       1,
                       INT_MAX,
                       1,
                       G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

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
 *
 * Since: 1.1
 */
const char *
shumate_symbol_event_get_layer (ShumateSymbolEvent *self)
{
  g_return_val_if_fail (SHUMATE_IS_SYMBOL_EVENT (self), NULL);
  return self->layer;
}

/**
 * shumate_symbol_event_get_source_layer:
 * @self: a [class@SymbolEvent]
 *
 * Gets the name of the source layer the clicked feature is in,
 * as named in the vector tile schema.
 *
 * Returns: (transfer none): the layer name
 *
 * Since: 1.1
 */
const char *
shumate_symbol_event_get_source_layer (ShumateSymbolEvent *self)
{
  g_return_val_if_fail (SHUMATE_IS_SYMBOL_EVENT (self), NULL);
  return self->source_layer;
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
 *
 * Since: 1.1
 */
const char *
shumate_symbol_event_get_feature_id (ShumateSymbolEvent *self)
{
  g_return_val_if_fail (SHUMATE_IS_SYMBOL_EVENT (self), NULL);
  return self->feature_id;
}

/**
 * shumate_symbol_event_get_keys:
 * @self: a [class@SymbolEvent]
 *
 * Gets a list of the keys of the source feature's tags.
 *
 * Returns: (transfer container): a list of the tag keys
 *
 * Since: 1.1
 */
const GStrv
shumate_symbol_event_get_keys (ShumateSymbolEvent *self)
{
  g_return_val_if_fail (SHUMATE_IS_SYMBOL_EVENT (self), NULL);
  return (GStrv)g_hash_table_get_keys_as_array (self->tags, NULL);
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
 *
 * Since: 1.1
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
                          const char *source_layer,
                          const char *feature_id,
                          GHashTable *tags)
{
  ShumateSymbolEvent *self = g_object_new (SHUMATE_TYPE_SYMBOL_EVENT, NULL);

  self->layer = g_strdup (layer);
  self->source_layer = g_strdup (source_layer);
  self->feature_id = g_strdup (feature_id);
  self->tags = g_hash_table_ref (tags);
  self->n_press = 1;

  return self;
}

ShumateSymbolEvent *
shumate_symbol_event_new_with_n_press (const char *layer,
                                       const char *source_layer,
                                       const char *feature_id,
                                       GHashTable *tags,
                                       gint n_press)
{
  ShumateSymbolEvent *self = g_object_new (SHUMATE_TYPE_SYMBOL_EVENT, NULL);

  self->layer = g_strdup (layer);
  self->source_layer = g_strdup (source_layer);
  self->feature_id = g_strdup (feature_id);
  self->tags = g_hash_table_ref (tags);
  self->n_press = n_press;

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

/**
 * shumate_symbol_event_get_n_press:
 * @self: a [class@SymbolEvent]
 *
 * Gets the number of clicks/presses that initiated the event.
 *
 * Returns: the number of presses
 *
 * Since: 1.5
 */
gint
shumate_symbol_event_get_n_press (ShumateSymbolEvent *self)
{
  return self->n_press;
}

/**
 * shumate_symbol_event_set_n_press:
 * @self: a [class@SymbolEvent]
 * @n_press: the number of presses of the event
 *
 * Sets the number of clicks/presses that initiated the event.
 *
 *
 * Since: 1.5
 */
void
shumate_symbol_event_set_n_press (ShumateSymbolEvent *self, gint n_press)
{
  self->n_press = n_press;
}
