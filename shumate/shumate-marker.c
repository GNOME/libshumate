/*
 * Copyright (C) 2008 Pierre-Luc Beaudoin <pierre-luc@pierlux.com>
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
 * ShumateMarker:
 *
 * Markers represent points of interest on a map. Markers need to be
 * placed on a layer (a [class@MarkerLayer]). Layers have to be added to a
 * [class@Map] for the markers to show on the map.
 *
 * A marker is nothing more than a regular [class@Gtk.Widget]. You can draw on
 * it what ever you want. Set the marker's position on the map using
 * [method@Location.set_location].
 *
 * This is a base class of all markers. A typical usage of a marker is for
 * instance to add a [class@Gtk.Image] with a pin image and add the
 * [class@Gtk.GestureClick] controller to listen to click events and show
 * a [class@Gtk.Popover] with the description of the marker.
 *
 * ## CSS nodes
 *
 * `ShumateMarker` has a single CSS node with the name “map-marker”.
 */

#include "shumate-marker.h"
#include "shumate-marker-private.h"

#include "shumate.h"
#include "shumate-marshal.h"
#include "shumate-tile.h"

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

enum
{
  PROP_SELECTABLE = 1,
  PROP_CHILD,
  PROP_X_HOTSPOT,
  PROP_Y_HOTSPOT,
  N_PROPERTIES,

  PROP_LONGITUDE,
  PROP_LATITUDE,
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

typedef struct
{
  double lon;
  double lat;
  double x_hotspot;
  double y_hotspot;

  gboolean selected;

  gboolean selectable;

  float click_x;
  float click_y;
  gboolean moved;

  GtkWidget *child;
} ShumateMarkerPrivate;

static void location_interface_init (ShumateLocationInterface *iface);
static void buildable_interface_init (GtkBuildableIface *iface);

G_DEFINE_TYPE_WITH_CODE (ShumateMarker, shumate_marker, GTK_TYPE_WIDGET,
    G_ADD_PRIVATE (ShumateMarker)
    G_IMPLEMENT_INTERFACE (SHUMATE_TYPE_LOCATION, location_interface_init)
    G_IMPLEMENT_INTERFACE (GTK_TYPE_BUILDABLE, buildable_interface_init));

static GtkBuildableIface *parent_buildable_iface;

static void
shumate_marker_set_location (ShumateLocation *location,
                             double           latitude,
                             double           longitude)
{
  ShumateMarker *marker = (ShumateMarker *) location;
  ShumateMarkerPrivate *priv = shumate_marker_get_instance_private (marker);

  g_assert (SHUMATE_IS_MARKER (location));

  priv->lon = CLAMP (longitude, SHUMATE_MIN_LONGITUDE, SHUMATE_MAX_LONGITUDE);
  priv->lat = CLAMP (latitude, SHUMATE_MIN_LATITUDE, SHUMATE_MAX_LATITUDE);

  g_object_notify (G_OBJECT (location), "latitude");
  g_object_notify (G_OBJECT (location), "longitude");
}


static double
shumate_marker_get_latitude (ShumateLocation *location)
{
  ShumateMarker *marker = (ShumateMarker *) location;
  ShumateMarkerPrivate *priv = shumate_marker_get_instance_private (marker);

  g_assert (SHUMATE_IS_MARKER (location));

  return priv->lat;
}


static double
shumate_marker_get_longitude (ShumateLocation *location)
{
  ShumateMarker *marker = (ShumateMarker *) location;
  ShumateMarkerPrivate *priv = shumate_marker_get_instance_private (marker);

  g_assert (SHUMATE_IS_MARKER (location));

  return priv->lon;
}

static void
shumate_marker_add_child (GtkBuildable *buildable,
                          GtkBuilder   *builder,
                          GObject      *child,
                          const char   *type)
{
  if (GTK_IS_WIDGET (child))
    shumate_marker_set_child (SHUMATE_MARKER (buildable), GTK_WIDGET (child));
  else
    parent_buildable_iface->add_child (buildable, builder, child, type);
}

static void
shumate_marker_get_property (GObject *object,
    guint prop_id,
    GValue *value,
    GParamSpec *pspec)
{
  ShumateMarker *marker = SHUMATE_MARKER (object);
  ShumateMarkerPrivate *priv = shumate_marker_get_instance_private (marker);

  switch (prop_id)
    {
    case PROP_LONGITUDE:
      g_value_set_double (value, priv->lon);
      break;

    case PROP_LATITUDE:
      g_value_set_double (value, priv->lat);
      break;

    case PROP_X_HOTSPOT:
      g_value_set_double (value, priv->x_hotspot);
      break;

    case PROP_Y_HOTSPOT:
      g_value_set_double (value, priv->y_hotspot);
      break;

    case PROP_SELECTABLE:
      g_value_set_boolean (value, priv->selectable);
      break;

    case PROP_CHILD:
      g_value_set_object (value, priv->child);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}


static void
shumate_marker_set_property (GObject *object,
    guint prop_id,
    const GValue *value,
    GParamSpec *pspec)
{
  ShumateMarker *marker = SHUMATE_MARKER (object);
  ShumateMarkerPrivate *priv = shumate_marker_get_instance_private (marker);

  switch (prop_id)
    {
    case PROP_LONGITUDE:
      {
        double lon = g_value_get_double (value);
        shumate_marker_set_location (SHUMATE_LOCATION (marker), priv->lat, lon);
        break;
      }

    case PROP_LATITUDE:
      {
        double lat = g_value_get_double (value);
        shumate_marker_set_location (SHUMATE_LOCATION (marker), lat, priv->lon);
        break;
      }

    case PROP_X_HOTSPOT:
      shumate_marker_set_hotspot (marker, g_value_get_double (value), priv->y_hotspot);
      break;

    case PROP_Y_HOTSPOT:
      shumate_marker_set_hotspot (marker, priv->x_hotspot, g_value_get_double (value));
      break;

    case PROP_SELECTABLE:
      {
        gboolean bvalue = g_value_get_boolean (value);
        shumate_marker_set_selectable (marker, bvalue);
        break;
      }

    case PROP_CHILD:
      {
        GtkWidget *child = g_value_get_object (value);
        shumate_marker_set_child (marker, child);
        break;
      }

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
shumate_marker_dispose (GObject *object)
{
  ShumateMarker *marker = SHUMATE_MARKER (object);

  shumate_marker_set_child (marker, NULL);

  G_OBJECT_CLASS (shumate_marker_parent_class)->dispose (object);
}

static void
shumate_marker_class_init (ShumateMarkerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = shumate_marker_get_property;
  object_class->set_property = shumate_marker_set_property;
  object_class->dispose = shumate_marker_dispose;

  /**
   * ShumateMarker:child:
   *
   * The child widget of the marker
   */
  obj_properties[PROP_CHILD] =
    g_param_spec_object ("child",
                         "Child",
                          "The child widget of the marker",
                          GTK_TYPE_WIDGET,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * ShumateMarker:selectable:
   *
   * The selectable state of the marker
   */
  obj_properties[PROP_SELECTABLE] =
    g_param_spec_boolean ("selectable",
                          "Selectable",
                          "The draggable state of the marker",
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * ShumateMarker:x-hotspot:
   *
   * The x hotspot of the marker, a negative value means that the actual
   * x hotspot is calculated with the [property@Gtk.Widget:halign] property.
   * The x hotspot should not be more than the width of the widget.
   *
   * Since: 1.5
   */
  obj_properties[PROP_X_HOTSPOT] =
    g_param_spec_double ("x-hotspot", NULL, NULL,
                         -G_MAXDOUBLE, G_MAXDOUBLE, -1,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * ShumateMarker:y-hotspot:
   *
   * The y hotspot of the marker, a negative value means that the actual
   * y hotspot is calculated with the [property@Gtk.Widget:valign] property.
   * The y hotspot should not be more than the height of the widget.
   *
   * Since: 1.5
   */
  obj_properties[PROP_Y_HOTSPOT] =
    g_param_spec_double ("y-hotspot", NULL, NULL,
                         -G_MAXDOUBLE, G_MAXDOUBLE, -1,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPERTIES, obj_properties);

  g_object_class_override_property (object_class,
      PROP_LONGITUDE,
      "longitude");

  g_object_class_override_property (object_class,
      PROP_LATITUDE,
      "latitude");

  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
  gtk_widget_class_set_css_name (widget_class, "map-marker");
}

static void
shumate_marker_init (ShumateMarker *self)
{
  ShumateMarkerPrivate *priv = shumate_marker_get_instance_private (self);

  priv->lat = 0;
  priv->lon = 0;
  priv->selected = FALSE;
  priv->selectable = TRUE;
  priv->x_hotspot = -1;
  priv->y_hotspot = -1;
}

static void
location_interface_init (ShumateLocationInterface *iface)
{
  iface->get_latitude = shumate_marker_get_latitude;
  iface->get_longitude = shumate_marker_get_longitude;
  iface->set_location = shumate_marker_set_location;
}

static void
buildable_interface_init (GtkBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);
  iface->add_child = shumate_marker_add_child;
}

/**
 * shumate_marker_new:
 *
 * Creates an instance of #ShumateMarker.
 *
 * Returns: a new #ShumateMarker.
 */
ShumateMarker *
shumate_marker_new (void)
{
  return SHUMATE_MARKER (g_object_new (SHUMATE_TYPE_MARKER, NULL));
}

/**
 * shumate_marker_is_selected:
 * @marker: a #ShumateMarker
 *
 * Checks whether the marker is selected.
 *
 * Returns: %TRUE if the marker is selected, otherwise %FALSE
 */
gboolean
shumate_marker_is_selected (ShumateMarker *marker)
{
  ShumateMarkerPrivate *priv = shumate_marker_get_instance_private (marker);

  g_return_val_if_fail (SHUMATE_IS_MARKER (marker), FALSE);

  return priv->selected;
}


/**
 * shumate_marker_set_selectable:
 * @marker: a #ShumateMarker
 * @value: the selectable state
 *
 * Sets the marker as selectable or not.
 */
void
shumate_marker_set_selectable (ShumateMarker *marker,
                               gboolean       value)
{
  ShumateMarkerPrivate *priv = shumate_marker_get_instance_private (marker);

  g_return_if_fail (SHUMATE_IS_MARKER (marker));

  priv->selectable = value;

  g_object_notify_by_pspec (G_OBJECT (marker), obj_properties[PROP_SELECTABLE]);
}


/**
 * shumate_marker_get_selectable:
 * @marker: a #ShumateMarker
 *
 * Checks whether the marker is selectable.
 *
 * Returns: the selectable or not state of the marker.
 */
gboolean
shumate_marker_get_selectable (ShumateMarker *marker)
{
  ShumateMarkerPrivate *priv = shumate_marker_get_instance_private (marker);

  g_return_val_if_fail (SHUMATE_IS_MARKER (marker), FALSE);

  return priv->selectable;
}

/**
 * shumate_marker_get_child:
 * @marker: a #ShumateMarker
 *
 * Retrieves the current child of @marker.
 *
 * Returns: (transfer none) (nullable): a #GtkWidget.
 */
GtkWidget *
shumate_marker_get_child (ShumateMarker *marker)
{
  ShumateMarkerPrivate *priv = shumate_marker_get_instance_private (marker);

  g_return_val_if_fail (SHUMATE_IS_MARKER (marker), NULL);

  return priv->child;
}

/**
 * shumate_marker_set_child:
 * @marker: a #ShumateMarker
 * @child: (nullable): a #GtkWidget
 *
 * Sets the child widget of @marker.
 */
void
shumate_marker_set_child (ShumateMarker *marker,
                          GtkWidget     *child)
{
  ShumateMarkerPrivate *priv = shumate_marker_get_instance_private (marker);

  g_return_if_fail (SHUMATE_IS_MARKER (marker));

  if (priv->child == child)
    return;

  g_clear_pointer (&priv->child, gtk_widget_unparent);

  priv->child = child;

  if (priv->child)
    gtk_widget_set_parent (priv->child, GTK_WIDGET (marker));

  g_object_notify_by_pspec (G_OBJECT (marker), obj_properties[PROP_CHILD]);
}

/**
 * PRIVATE:shumate_marker_set_selected:
 * @marker: a #ShumateMarker
 * @value: %TRUE to select the marker, %FALSE to unselect it
 *
 * Sets the selected state flag of the marker widget.
 */
void
shumate_marker_set_selected (ShumateMarker *marker, gboolean value)
{
  ShumateMarkerPrivate *priv = shumate_marker_get_instance_private (marker);

  if (priv->selected == value)
    return;

  priv->selected = value;

  if (value) {
    gtk_widget_set_state_flags (GTK_WIDGET (marker),
                                GTK_STATE_FLAG_SELECTED, FALSE);
  } else {
    gtk_widget_unset_state_flags (GTK_WIDGET (marker),
                                  GTK_STATE_FLAG_SELECTED);
  }
}

/**
 * shumate_marker_set_hotspot:
 * @marker: a #ShumateMarker
 * @x_hotspot: the x hotspot
 * @y_hotspot: the y hotspot
 *
 * Set the hotspot point for the given marker. The value is in pixel relative
 * to the top-left corner. Use any negative value to fallback to the
 * [property@Gtk.Widget:halign] or [property@Gtk.Widget:valign] properties.
 *
 * Since: 1.5
 */
void
shumate_marker_set_hotspot (ShumateMarker *marker,
                            gdouble x_hotspot,
                            gdouble y_hotspot)
{
  ShumateMarkerPrivate *priv = shumate_marker_get_instance_private (marker);

  g_return_if_fail (SHUMATE_IS_MARKER (marker));

  if (x_hotspot < 0)
    x_hotspot = -1;

  if (y_hotspot < 0)
    y_hotspot = -1;

  g_object_freeze_notify (G_OBJECT (marker));
  if (x_hotspot != priv->x_hotspot) {
    priv->x_hotspot = x_hotspot;
    g_object_notify_by_pspec (G_OBJECT (marker), obj_properties[PROP_X_HOTSPOT]);
  }

  if (y_hotspot != priv->y_hotspot) {
    priv->y_hotspot = y_hotspot;
    g_object_notify_by_pspec (G_OBJECT (marker), obj_properties[PROP_Y_HOTSPOT]);
  }

  g_object_thaw_notify (G_OBJECT (marker));
}

/**
 * shumate_marker_get_hotspot:
 * @marker: a #ShumateMarker
 * @x_hotspot: (out) (optional): return value for the x hotspot
 * @y_hotspot: (out) (optional): return value for the y hotspot
 *
 * Get the hotspot point for the given marker. The value is in pixel relative
 * to the top-left corner. Any negative value means that the hotspot get
 * computed with the [property@Gtk.Widget:halign] or [property@Gtk.Widget:valign]
 * properties.
 *
 * Since: 1.5
 */
void shumate_marker_get_hotspot (ShumateMarker *marker,
                                 gdouble *x_hotspot,
                                 gdouble *y_hotspot)
{
  ShumateMarkerPrivate *priv = shumate_marker_get_instance_private (marker);

  g_return_if_fail (SHUMATE_IS_MARKER (marker));

  if (x_hotspot)
    *x_hotspot = priv->x_hotspot;

  if (y_hotspot)
    *y_hotspot = priv->y_hotspot;
}
