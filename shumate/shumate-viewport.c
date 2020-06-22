/* shumate-viewport.c: Viewport
 *
 * Copyright (C) 2008 OpenedHand
 * Copyright (C) 2011-2013 Jiri Techet <techet@gmail.com>
 * Copyright (C) 2019 Marcus Lundblad <ml@update.uu.se>
 * Copyright (C) 2020 Collabora, Ltd. (https://www.collabora.com)
 * Copyright (C) 2020 Corentin Noël <corentin.noel@collabora.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Written by: Chris Lord <chris@openedhand.com>
 */

#include "shumate-viewport.h"
#include "shumate-location.h"

/**
 * SECTION:shumate-viewport
 * @short_description: The Object holding the coordinate and zoom-level state of
 * the current view.
 */

struct _ShumateViewport
{
  GObject parent_instance;

  gdouble lon;
  gdouble lat;

  guint zoom_level;
  guint min_zoom_level;
  guint max_zoom_level;

  ShumateMapSource *ref_map_source;
};

static void shumate_viewport_shumate_location_interface_init (ShumateLocationInterface *iface);

G_DEFINE_TYPE_WITH_CODE (ShumateViewport, shumate_viewport, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (SHUMATE_TYPE_LOCATION, shumate_viewport_shumate_location_interface_init));

enum
{
  PROP_ZOOM_LEVEL = 1,
  PROP_MIN_ZOOM_LEVEL,
  PROP_MAX_ZOOM_LEVEL,
  PROP_REFERENCE_MAP_SOURCE,
  N_PROPERTIES,

  PROP_LONGITUDE,
  PROP_LATITUDE,
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static gdouble
shumate_viewport_get_latitude (ShumateLocation *location)
{
  ShumateViewport *self = (ShumateViewport *)location;

  g_assert (SHUMATE_IS_VIEWPORT (self));

  return self->lat;
}

static gdouble
shumate_viewport_get_longitude (ShumateLocation *location)
{
  ShumateViewport *self = (ShumateViewport *)location;

  g_assert (SHUMATE_IS_VIEWPORT (self));

  return self->lon;
}

static void
shumate_viewport_set_location (ShumateLocation *location,
                               gdouble          latitude,
                               gdouble          longitude)
{
  ShumateViewport *self = (ShumateViewport *)location;

  g_assert (SHUMATE_IS_VIEWPORT (self));

  self->lon = CLAMP (longitude, SHUMATE_MIN_LONGITUDE, SHUMATE_MAX_LONGITUDE);
  self->lat = CLAMP (latitude, SHUMATE_MIN_LATITUDE, SHUMATE_MAX_LATITUDE);
  g_object_notify (G_OBJECT (self), "longitude");
  g_object_notify (G_OBJECT (self), "latitude");
}

static void
shumate_viewport_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  ShumateViewport *self = SHUMATE_VIEWPORT (object);

  switch (prop_id)
    {
    case PROP_ZOOM_LEVEL:
      g_value_set_uint (value, self->zoom_level);
      break;

    case PROP_MIN_ZOOM_LEVEL:
      g_value_set_uint (value, self->min_zoom_level);
      break;

    case PROP_MAX_ZOOM_LEVEL:
      g_value_set_uint (value, self->max_zoom_level);
      break;

    case PROP_REFERENCE_MAP_SOURCE:
      g_value_set_object (value, self->ref_map_source);
      break;

    case PROP_LONGITUDE:
      g_value_set_double (value, self->lon);
      break;

    case PROP_LATITUDE:
      g_value_set_double (value, self->lat);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
shumate_viewport_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  ShumateViewport *self = SHUMATE_VIEWPORT (object);

  switch (prop_id)
    {
    case PROP_ZOOM_LEVEL:
      shumate_viewport_set_zoom_level (self, g_value_get_uint (value));
      break;

    case PROP_MIN_ZOOM_LEVEL:
      shumate_viewport_set_min_zoom_level (self, g_value_get_uint (value));
      break;

    case PROP_MAX_ZOOM_LEVEL:
      shumate_viewport_set_max_zoom_level (self, g_value_get_uint (value));
      break;

    case PROP_REFERENCE_MAP_SOURCE:
      shumate_viewport_set_reference_map_source (self, g_value_get_object (value));
      break;

    case PROP_LONGITUDE:
      self->lon = CLAMP (g_value_get_double (value), SHUMATE_MIN_LONGITUDE, SHUMATE_MAX_LONGITUDE);
      g_object_notify (object, "longitude");
      break;

    case PROP_LATITUDE:
      self->lat = CLAMP (g_value_get_double (value), SHUMATE_MIN_LATITUDE, SHUMATE_MAX_LATITUDE);
      g_object_notify (object, "latitude");
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
shumate_viewport_dispose (GObject *object)
{
  ShumateViewport *self = SHUMATE_VIEWPORT (object);

  g_clear_object (&self->ref_map_source);

  G_OBJECT_CLASS (shumate_viewport_parent_class)->dispose (object);
}

static void
shumate_viewport_class_init (ShumateViewportClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = shumate_viewport_get_property;
  object_class->set_property = shumate_viewport_set_property;
  object_class->dispose = shumate_viewport_dispose;

  /**
   * ShumateViewport:zoom-level:
   *
   * The level of zoom of the content.
   */
  obj_properties[PROP_ZOOM_LEVEL] =
    g_param_spec_uint ("zoom-level",
                       "Zoom level",
                       "The level of zoom of the map",
                       0, 20, 3,
                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * ShumateViewport:min-zoom-level:
   *
   * The lowest allowed level of zoom of the content.
   */
  obj_properties[PROP_MIN_ZOOM_LEVEL] =
    g_param_spec_uint ("min-zoom-level",
                       "Min zoom level",
                       "The lowest allowed level of zoom",
                       0, 20, 0,
                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * ShumateViewport:max-zoom-level:
   *
   * The highest allowed level of zoom of the content.
   */
  obj_properties[PROP_MAX_ZOOM_LEVEL] =
    g_param_spec_uint ("max-zoom-level",
                       "Max zoom level",
                       "The highest allowed level of zoom",
                       0, 20, 20,
                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * ShumateViewport:reference-map-source:
   *
   * The reference #ShumateMapSource being displayed
   */
  obj_properties[PROP_REFERENCE_MAP_SOURCE] =
    g_param_spec_object ("reference-map-source",
                         "Reference Map Source",
                         "The reference map source being displayed",
                         SHUMATE_TYPE_MAP_SOURCE,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  
  g_object_class_install_properties (object_class,
                                     N_PROPERTIES,
                                     obj_properties);

  g_object_class_override_property (object_class,
      PROP_LONGITUDE,
      "longitude");

  g_object_class_override_property (object_class,
      PROP_LATITUDE,
      "latitude");
}

static void
shumate_viewport_init (ShumateViewport *self)
{
}

static void
shumate_viewport_shumate_location_interface_init (ShumateLocationInterface *iface)
{
  iface->get_latitude = shumate_viewport_get_latitude;
  iface->get_longitude = shumate_viewport_get_longitude;
  iface->set_location = shumate_viewport_set_location;
}

/**
 * shumate_viewport_new:
 *
 * Creates a new #ShumateViewport
 *
 * Returns: A new #ShumateViewport
 */
ShumateViewport *
shumate_viewport_new (void)
{
  return g_object_new (SHUMATE_TYPE_VIEWPORT, NULL);
}

/**
 * shumate_viewport_set_zoom_level:
 * @self: a #ShumateViewport
 * @zoom_level: the zoom level
 *
 * Set the zoom level
 */
void
shumate_viewport_set_zoom_level (ShumateViewport *self,
                                 guint            zoom_level)
{
  g_return_if_fail (SHUMATE_IS_VIEWPORT (self));

  self->zoom_level = CLAMP (zoom_level, self->min_zoom_level, self->max_zoom_level);
  g_object_notify_by_pspec (G_OBJECT (self), obj_properties[PROP_ZOOM_LEVEL]);
}

/**
 * shumate_viewport_get_zoom_level:
 * @self: a #ShumateViewport
 *
 * Get the current zoom level
 * 
 * Returns: the current zoom level
 */
guint
shumate_viewport_get_zoom_level (ShumateViewport *self)
{
  g_return_val_if_fail (SHUMATE_IS_VIEWPORT (self), 0U);

  return self->zoom_level;
}

/**
 * shumate_viewport_set_max_zoom_level:
 * @self: a #ShumateViewport
 * @max_zoom_level: the maximal zoom level
 *
 * Set the maximal zoom level
 */
void
shumate_viewport_set_max_zoom_level (ShumateViewport *self,
                                     guint            max_zoom_level)
{
  g_return_if_fail (SHUMATE_IS_VIEWPORT (self));

  if (self->zoom_level > max_zoom_level)
    shumate_viewport_set_zoom_level (self, max_zoom_level);
  
  self->max_zoom_level = max_zoom_level;
  g_object_notify_by_pspec (G_OBJECT (self), obj_properties[PROP_MAX_ZOOM_LEVEL]);
}

/**
 * shumate_viewport_get_max_zoom_level:
 * @self: a #ShumateViewport
 *
 * Get the maximal zoom level
 * 
 * Returns: the maximal zoom level
 */
guint
shumate_viewport_get_max_zoom_level (ShumateViewport *self)
{
  g_return_val_if_fail (SHUMATE_IS_VIEWPORT (self), 0U);

  return self->max_zoom_level;
}

/**
 * shumate_viewport_set_min_zoom_level:
 * @self: a #ShumateViewport
 * @min_zoom_level: the minimal zoom level
 *
 * Set the minimal zoom level
 */
void
shumate_viewport_set_min_zoom_level (ShumateViewport *self,
                                     guint            min_zoom_level)
{
  g_return_if_fail (SHUMATE_IS_VIEWPORT (self));

  if (self->zoom_level > min_zoom_level)
    shumate_viewport_set_zoom_level (self, min_zoom_level);

  self->min_zoom_level = min_zoom_level;
  g_object_notify_by_pspec (G_OBJECT (self), obj_properties[PROP_MIN_ZOOM_LEVEL]);
}

/**
 * shumate_viewport_get_min_zoom_level:
 * @self: a #ShumateViewport
 *
 * Get the minimal zoom level
 * 
 * Returns: the minimal zoom level
 */
guint
shumate_viewport_get_min_zoom_level (ShumateViewport *self)
{
  g_return_val_if_fail (SHUMATE_IS_VIEWPORT (self), 0U);

  return self->min_zoom_level;
}

/**
 * shumate_viewport_zoom_in:
 * @self: a #ShumateViewport
 *
 * Increments the zoom level
 */
void shumate_viewport_zoom_in (ShumateViewport *self)
{
  g_return_if_fail (SHUMATE_IS_VIEWPORT (self));

  if (self->zoom_level == 0)
    return;

  shumate_viewport_set_zoom_level (self, self->zoom_level + 1);
}

/**
 * shumate_viewport_zoom_out:
 * @self: a #ShumateViewport
 *
 * Decrements the zoom level
 */
void shumate_viewport_zoom_out (ShumateViewport *self)
{
  g_return_if_fail (SHUMATE_IS_VIEWPORT (self));

  shumate_viewport_set_zoom_level (self, self->zoom_level - 1);
}

/**
 * shumate_viewport_set_reference_map_source:
 * @self: a #ShumateViewport
 * @map_source: (nullable): a #ShumateMapSource or %NULL to set none.
 *
 * Set the reference map source
 */
void
shumate_viewport_set_reference_map_source (ShumateViewport  *self,
                                           ShumateMapSource *map_source)
{
  g_return_if_fail (SHUMATE_IS_VIEWPORT (self));

  shumate_viewport_set_max_zoom_level (self, shumate_map_source_get_max_zoom_level (map_source));
  shumate_viewport_set_min_zoom_level (self, shumate_map_source_get_min_zoom_level (map_source));

  if (g_set_object (&self->ref_map_source, map_source))
    g_object_notify_by_pspec (G_OBJECT (self), obj_properties[PROP_REFERENCE_MAP_SOURCE]);
}

/**
 * shumate_viewport_get_reference_map_source:
 * @self: a #ShumateViewport
 *
 * Get the reference map source
 * 
 * Returns: (transfer none) (nullable): the reference #ShumateMapSource or %NULL
 * when none has been set.
 */
ShumateMapSource *
shumate_viewport_get_reference_map_source (ShumateViewport  *self)
{
  g_return_val_if_fail (SHUMATE_IS_VIEWPORT (self), NULL);

  return self->ref_map_source;
}

/**
 * shumate_viewport_widget_x_to_longitude:
 * @self: a #ShumateViewport
 * @widget: a #GtkWidget that uses @self as viewport
 * @x: the x coordinate
 *
 * Get the longitude from an x coordinate of a widget.
 * The widget is assumed to be using the viewport.
 * 
 * Returns: the longitude
 */
gdouble
shumate_viewport_widget_x_to_longitude (ShumateViewport *self,
                                        GtkWidget       *widget,
                                        gdouble          x)
{
  gdouble center_x;
  gint width;

  g_return_val_if_fail (SHUMATE_IS_VIEWPORT (self), 0.0);
  g_return_val_if_fail (GTK_IS_WIDGET (widget), 0.0);

  if (!self->ref_map_source)
    {
      g_critical ("A reference map source is required to compute the longitude from X.");
      return 0.0;
    }

  width = gtk_widget_get_width (widget);
  center_x = shumate_map_source_get_x (self->ref_map_source, self->zoom_level, self->lon);
  return shumate_map_source_get_longitude (self->ref_map_source, self->zoom_level, center_x - width/2 + x);
}

/**
 * shumate_viewport_widget_y_to_latitude:
 * @self: a #ShumateViewport
 * @widget: a #GtkWidget that uses @self as viewport
 * @y: the y coordinate
 *
 * Get the latitude from an y coordinate of a widget.
 * The widget is assumed to be using the viewport.
 * 
 * Returns: the latitude
 */
gdouble
shumate_viewport_widget_y_to_latitude (ShumateViewport *self,
                                       GtkWidget       *widget,
                                       gdouble          y)
{
  gdouble center_y;
  gint height;

  g_return_val_if_fail (SHUMATE_IS_VIEWPORT (self), 0.0);
  g_return_val_if_fail (GTK_IS_WIDGET (widget), 0.0);

  if (!self->ref_map_source)
    {
      g_critical ("A reference map source is required to compute Y from the latitude.");
      return 0.0;
    }

  height = gtk_widget_get_height (widget);
  center_y = shumate_map_source_get_y (self->ref_map_source, self->zoom_level, self->lat);
  return shumate_map_source_get_latitude (self->ref_map_source, self->zoom_level, center_y - height/2 + y);
}

/**
 * shumate_viewport_longitude_to_widget_x:
 * @self: a #ShumateViewport
 * @widget: a #GtkWidget that uses @self as viewport
 * @longitude: the longitude
 *
 * Get an x coordinate of a widget from the longitude.
 * The widget is assumed to be using the viewport.
 * 
 * Returns: the x coordinate
 */
gdouble
shumate_viewport_longitude_to_widget_x (ShumateViewport *self,
                                        GtkWidget       *widget,
                                        gdouble          longitude)
{
  gdouble center_longitude;
  gdouble left_x, x;
  gint width;

  g_return_val_if_fail (SHUMATE_IS_VIEWPORT (self), 0.0);
  g_return_val_if_fail (GTK_IS_WIDGET (widget), 0.0);

  if (!self->ref_map_source)
    {
      g_critical ("A reference map source is required to compute X from the longitude.");
      return 0.0;
    }

  width = gtk_widget_get_width (widget);
  center_longitude = shumate_location_get_longitude (SHUMATE_LOCATION (self));
  left_x = shumate_map_source_get_x (self->ref_map_source, self->zoom_level, center_longitude) - width/2;
  x = shumate_map_source_get_x (self->ref_map_source, self->zoom_level, longitude);
  return x - left_x;
}

/**
 * shumate_viewport_latitude_to_widget_y:
 * @self: a #ShumateViewport
 * @widget: a #GtkWidget that uses @self as viewport
 * @latitude: the latitude
 *
 * Get an y coordinate of a widget from the latitude.
 * The widget is assumed to be using the viewport.
 * 
 * Returns: the y coordinate
 */
gdouble
shumate_viewport_latitude_to_widget_y (ShumateViewport *self,
                                       GtkWidget       *widget,
                                       gdouble          latitude)
{
  gdouble center_latitude;
  gdouble top_y, y;
  gint height;

  g_return_val_if_fail (SHUMATE_IS_VIEWPORT (self), 0.0);
  g_return_val_if_fail (GTK_IS_WIDGET (widget), 0.0);

  if (!self->ref_map_source)
    {
      g_critical ("A reference map source is required to compute the latitude from Y.");
      return 0.0;
    }

  height = gtk_widget_get_height (widget);
  center_latitude = shumate_location_get_latitude (SHUMATE_LOCATION (self));
  top_y = shumate_map_source_get_y (self->ref_map_source, self->zoom_level, center_latitude) - height/2;
  y = shumate_map_source_get_y (self->ref_map_source, self->zoom_level, latitude);
  return y - top_y;
}
