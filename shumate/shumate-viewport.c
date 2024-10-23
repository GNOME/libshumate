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
 * ShumateViewport:
 *
 * The object holding the coordinate, zoom-level, and rotation state of the current view.
 *
 * As the object implements [iface@Shumate.Location], the latitude and longitude are
 * accessible via the interface methods.
 */

#define DEFAULT_MIN_ZOOM 0
#define DEFAULT_MAX_ZOOM 20

struct _ShumateViewport
{
  GObject parent_instance;

  double lon;
  double lat;

  double zoom_level;
  guint min_zoom_level;
  guint max_zoom_level;
  double rotation;

  ShumateMapSource *ref_map_source;
};

static void shumate_viewport_shumate_location_interface_init (ShumateLocationInterface *iface);

G_DEFINE_TYPE_WITH_CODE (ShumateViewport, shumate_viewport, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (SHUMATE_TYPE_LOCATION, shumate_viewport_shumate_location_interface_init));

/* Remember to update shumate_viewport_copy() when adding properties */
enum
{
  PROP_ZOOM_LEVEL = 1,
  PROP_MIN_ZOOM_LEVEL,
  PROP_MAX_ZOOM_LEVEL,
  PROP_REFERENCE_MAP_SOURCE,
  PROP_ROTATION,
  N_PROPERTIES,

  PROP_LONGITUDE,
  PROP_LATITUDE,
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static double
shumate_viewport_get_latitude (ShumateLocation *location)
{
  ShumateViewport *self = (ShumateViewport *)location;

  g_assert (SHUMATE_IS_VIEWPORT (self));

  return self->lat;
}

static double
shumate_viewport_get_longitude (ShumateLocation *location)
{
  ShumateViewport *self = (ShumateViewport *)location;

  g_assert (SHUMATE_IS_VIEWPORT (self));

  return self->lon;
}

static void
shumate_viewport_set_location (ShumateLocation *location,
                               double           latitude,
                               double           longitude)
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
      g_value_set_double (value, self->zoom_level);
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

    case PROP_ROTATION:
      g_value_set_double (value, self->rotation);
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
      shumate_viewport_set_zoom_level (self, g_value_get_double (value));
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

    case PROP_ROTATION:
      shumate_viewport_set_rotation (self, g_value_get_double (value));
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
    g_param_spec_double ("zoom-level",
                         "Zoom level",
                         "The level of zoom of the map",
                         0, 30, 3,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * ShumateViewport:min-zoom-level:
   *
   * The lowest allowed level of zoom of the content.
   */
  obj_properties[PROP_MIN_ZOOM_LEVEL] =
    g_param_spec_uint ("min-zoom-level",
                       "Min zoom level",
                       "The lowest allowed level of zoom",
                       0, 20, DEFAULT_MIN_ZOOM,
                       G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * ShumateViewport:max-zoom-level:
   *
   * The highest allowed level of zoom of the content.
   */
  obj_properties[PROP_MAX_ZOOM_LEVEL] =
    g_param_spec_uint ("max-zoom-level",
                       "Max zoom level",
                       "The highest allowed level of zoom",
                       0, 30, DEFAULT_MAX_ZOOM,
                       G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

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
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * ShumateViewport:rotation:
   *
   * The rotation of the map view, in radians clockwise from up being due north
   */
  obj_properties[PROP_ROTATION] =
    g_param_spec_double ("rotation",
                         "Rotation",
                         "The rotation of the map view in radians",
                         0, G_PI * 2.0, 0,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

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
  /* We need to set these here, otherwise the max_zoom >= min_zoom check in
   * the setter functions may fail if they're not called in the right order. */
  self->min_zoom_level = DEFAULT_MIN_ZOOM;
  self->max_zoom_level = DEFAULT_MAX_ZOOM;
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
                                 double           zoom_level)
{
  g_return_if_fail (SHUMATE_IS_VIEWPORT (self));

  zoom_level = CLAMP (zoom_level, self->min_zoom_level, self->max_zoom_level);

  if (self->zoom_level == zoom_level)
    return;

  self->zoom_level = zoom_level;
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
double
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
  g_return_if_fail (max_zoom_level >= self->min_zoom_level);

  if (self->max_zoom_level == max_zoom_level)
    return;

  self->max_zoom_level = max_zoom_level;
  g_object_notify_by_pspec (G_OBJECT (self), obj_properties[PROP_MAX_ZOOM_LEVEL]);

  if (self->zoom_level > max_zoom_level)
    shumate_viewport_set_zoom_level (self, max_zoom_level);
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
  g_return_if_fail (min_zoom_level <= self->max_zoom_level);

  if (self->min_zoom_level == min_zoom_level)
    return;

  self->min_zoom_level = min_zoom_level;
  g_object_notify_by_pspec (G_OBJECT (self), obj_properties[PROP_MIN_ZOOM_LEVEL]);

  if (self->zoom_level < min_zoom_level)
    shumate_viewport_set_zoom_level (self, min_zoom_level);
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
  g_return_if_fail (map_source == NULL || SHUMATE_IS_MAP_SOURCE (map_source));

  if (g_set_object (&self->ref_map_source, map_source))
    {
      if (map_source != NULL)
        {
          shumate_viewport_set_max_zoom_level (self, shumate_map_source_get_max_zoom_level (map_source));
          shumate_viewport_set_min_zoom_level (self, shumate_map_source_get_min_zoom_level (map_source));
        }

      g_object_notify_by_pspec (G_OBJECT (self), obj_properties[PROP_REFERENCE_MAP_SOURCE]);
    }

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
shumate_viewport_get_reference_map_source (ShumateViewport *self)
{
  g_return_val_if_fail (SHUMATE_IS_VIEWPORT (self), NULL);

  return self->ref_map_source;
}

/**
 * shumate_viewport_set_rotation:
 * @self: a #ShumateViewport
 * @rotation: the rotation
 *
 * Sets the rotation
 */
void
shumate_viewport_set_rotation (ShumateViewport *self,
                               double           rotation)
{
  g_return_if_fail (SHUMATE_IS_VIEWPORT (self));

  rotation = fmod (rotation, G_PI * 2.0);
  if (rotation < 0)
    rotation += G_PI * 2.0;

  if (self->rotation == rotation)
    return;

  self->rotation = rotation;
  g_object_notify_by_pspec (G_OBJECT (self), obj_properties[PROP_ROTATION]);
}

/**
 * shumate_viewport_get_rotation:
 * @self: a #ShumateViewport
 *
 * Gets the current rotation
 *
 * Returns: the current rotation
 */
double
shumate_viewport_get_rotation (ShumateViewport *self)
{
  g_return_val_if_fail (SHUMATE_IS_VIEWPORT (self), 0);

  return self->rotation;
}

static void
rotate_around_center (double *x, double *y, double width, double height, double angle)
{
  /* Rotate (x, y) around (width / 2, height / 2) */

  double old_x = *x;
  double old_y = *y;
  double center_x = width / 2.0;
  double center_y = height / 2.0;

  *x = cos(angle) * (old_x - center_x) - sin(angle) * (old_y - center_y) + center_x;
  *y = sin(angle) * (old_x - center_x) + cos(angle) * (old_y - center_y) + center_y;
}

static double
positive_mod (double i, double n)
{
  return fmod (fmod (i, n) + n, n);
}

/**
 * shumate_viewport_widget_coords_to_location:
 * @self: a #ShumateViewport
 * @widget: a #GtkWidget that uses @self as viewport
 * @x: the x coordinate
 * @y: the y coordinate
 * @latitude: (out): return location for the latitude
 * @longitude: (out): return location for the longitude
 *
 * Gets the latitude and longitude corresponding to a position on @widget.
 */
void
shumate_viewport_widget_coords_to_location (ShumateViewport *self,
                                            GtkWidget       *widget,
                                            double           x,
                                            double           y,
                                            double          *latitude,
                                            double          *longitude)
{
  double center_x, center_y;
  double width, height;
  double tile_size;
  double map_width, map_height;

  g_return_if_fail (SHUMATE_IS_VIEWPORT (self));
  g_return_if_fail (GTK_IS_WIDGET (widget));
  g_return_if_fail (latitude != NULL);
  g_return_if_fail (longitude != NULL);

  if (!self->ref_map_source)
    {
      g_critical ("A reference map source is required.");
      return;
    }

  width = gtk_widget_get_width (widget);
  height = gtk_widget_get_height (widget);
  rotate_around_center (&x, &y, width, height, -self->rotation);

  tile_size = shumate_map_source_get_tile_size_at_zoom (self->ref_map_source, self->zoom_level);
  map_width = tile_size * shumate_map_source_get_column_count (self->ref_map_source, self->zoom_level);
  map_height = tile_size * shumate_map_source_get_row_count (self->ref_map_source, self->zoom_level);

  center_x = shumate_map_source_get_x (self->ref_map_source, self->zoom_level, self->lon) - width/2 + x;
  center_y = shumate_map_source_get_y (self->ref_map_source, self->zoom_level, self->lat) - height/2 + y;

  center_x = positive_mod (center_x, map_width);
  center_y = positive_mod (center_y, map_height);

  *latitude = shumate_map_source_get_latitude (self->ref_map_source, self->zoom_level, center_y);
  *longitude = shumate_map_source_get_longitude (self->ref_map_source, self->zoom_level, center_x);
}

/**
 * shumate_viewport_location_to_widget_coords:
 * @self: a #ShumateViewport
 * @widget: a #GtkWidget that uses @self as viewport
 * @latitude: the latitude
 * @longitude: the longitude
 * @x: (out): return value for the x coordinate
 * @y: (out): return value for the y coordinate
 *
 * Gets the position on @widget that correspond to the given latitude and
 * longitude.
 */
void
shumate_viewport_location_to_widget_coords (ShumateViewport *self,
                                            GtkWidget       *widget,
                                            double           latitude,
                                            double           longitude,
                                            double          *x,
                                            double          *y)
{
  double center_latitude, center_longitude;
  double width, height;

  g_return_if_fail (SHUMATE_IS_VIEWPORT (self));
  g_return_if_fail (GTK_IS_WIDGET (widget));
  g_return_if_fail (x != NULL);
  g_return_if_fail (y != NULL);

  if (!self->ref_map_source)
    {
      g_critical ("A reference map source is required.");
      return;
    }

  width = gtk_widget_get_width (widget);
  height = gtk_widget_get_height (widget);

  *x = shumate_map_source_get_x (self->ref_map_source, self->zoom_level, longitude);
  *y = shumate_map_source_get_y (self->ref_map_source, self->zoom_level, latitude);

  center_latitude = shumate_location_get_latitude (SHUMATE_LOCATION (self));
  center_longitude = shumate_location_get_longitude (SHUMATE_LOCATION (self));
  *x -= shumate_map_source_get_x (self->ref_map_source, self->zoom_level, center_longitude) - width/2;
  *y -= shumate_map_source_get_y (self->ref_map_source, self->zoom_level, center_latitude) - height/2;

  rotate_around_center (x, y, width, height, self->rotation);
}

/**
 * shumate_viewport_copy:
 * @self: a [class@Viewport]
 *
 * Creates a copy of the viewport.
 *
 * Returns: (transfer full): a [class@Viewport] with the same values as @self
 */
ShumateViewport *
shumate_viewport_copy (ShumateViewport *self)
{
  g_return_val_if_fail (SHUMATE_IS_VIEWPORT (self), NULL);

  return g_object_new (SHUMATE_TYPE_VIEWPORT,
                       "latitude", self->lat,
                       "longitude", self->lon,
                       "min-zoom-level", self->min_zoom_level,
                       "max-zoom-level", self->max_zoom_level,
                       "rotation", self->rotation,
                       "reference-map-source", self->ref_map_source,
                       "zoom-level", self->zoom_level,
                       NULL);
}
