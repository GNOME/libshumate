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
 * SECTION:shumate-marker
 * @short_description: Base widget representing a marker
 *
 * Markers represent points of interest on a map. Markers need to be
 * placed on a layer (a #ShumateMarkerLayer). Layers have to be added to a
 * #ShumateView for the markers to show on the map.
 *
 * A marker is nothing more than a regular #GtkWidget. You can draw on it what
 * ever you want. Set the marker's position on the map using
 * shumate_location_set_location().
 *
 * This is a base class of all markers. libshumate has a more evoluted
 * type of markers with text and image support.
 */

#include "config.h"

#include "shumate-marker.h"
#include "shumate-marker-private.h"

#include "shumate.h"
#include "shumate-marshal.h"
#include "shumate-tile.h"

#include <glib.h>
#include <glib-object.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <cairo.h>
#include <math.h>

static GdkRGBA SELECTED_COLOR = { 0.0, 0.2, 0.8, 1.0 };
static GdkRGBA SELECTED_TEXT_COLOR = { 1.0, 1.0, 1.0, 1.0 };

enum
{
  PROP_SELECTABLE = 1,
  PROP_DRAGGABLE,
  PROP_CHILD,
  N_PROPERTIES,

  PROP_LONGITUDE,
  PROP_LATITUDE,
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

typedef struct
{
  double lon;
  double lat;

  guint selected    :1;

  gboolean selectable;
  gboolean draggable;

  float click_x;
  float click_y;
  gboolean moved;

  GtkWidget *child;
} ShumateMarkerPrivate;

static void location_interface_init (ShumateLocationInterface *iface);

G_DEFINE_TYPE_WITH_CODE (ShumateMarker, shumate_marker, GTK_TYPE_WIDGET,
    G_ADD_PRIVATE (ShumateMarker)
    G_IMPLEMENT_INTERFACE (SHUMATE_TYPE_LOCATION, location_interface_init));

/**
 * shumate_marker_set_selection_color:
 * @color: a #ClutterColor
 *
 * Changes the selection color, this is to ensure a better integration with
 * the desktop.
 */
void
shumate_marker_set_selection_color (GdkRGBA *color)
{
  SELECTED_COLOR.red = color->red;
  SELECTED_COLOR.green = color->green;
  SELECTED_COLOR.blue = color->blue;
  SELECTED_COLOR.alpha = color->alpha;
}


/**
 * shumate_marker_get_selection_color:
 *
 * Gets the selection color.
 *
 * Returns: the selection color. Should not be freed.
 */
const GdkRGBA *
shumate_marker_get_selection_color ()
{
  return &SELECTED_COLOR;
}


/**
 * shumate_marker_set_selection_text_color:
 * @color: a #ClutterColor
 *
 * Changes the selection text color, this is to ensure a better integration with
 * the desktop.
 */
void
shumate_marker_set_selection_text_color (GdkRGBA *color)
{
  SELECTED_TEXT_COLOR.red = color->red;
  SELECTED_TEXT_COLOR.green = color->green;
  SELECTED_TEXT_COLOR.blue = color->blue;
  SELECTED_TEXT_COLOR.alpha = color->alpha;
}


/**
 * shumate_marker_get_selection_text_color:
 *
 * Gets the selection text color.
 *
 * Returns: the selection text color. Should not be freed.
 */
const GdkRGBA *
shumate_marker_get_selection_text_color ()
{
  return &SELECTED_TEXT_COLOR;
}




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

    case PROP_SELECTABLE:
      g_value_set_boolean (value, priv->selectable);
      break;

    case PROP_DRAGGABLE:
      g_value_set_boolean (value, priv->draggable);
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

    case PROP_SELECTABLE:
      {
        gboolean bvalue = g_value_get_boolean (value);
        shumate_marker_set_selectable (marker, bvalue);
        break;
      }

    case PROP_DRAGGABLE:
      {
        gboolean bvalue = g_value_get_boolean (value);
        shumate_marker_set_draggable (marker, bvalue);
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
   * ShumateMarker:draggable:
   *
   * The draggable state of the marker
   */
  obj_properties[PROP_DRAGGABLE] =
    g_param_spec_boolean ("draggable",
                          "Draggable",
                          "The draggable state of the marker",
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPERTIES, obj_properties);

  g_object_class_override_property (object_class,
      PROP_LONGITUDE,
      "longitude");

  g_object_class_override_property (object_class,
      PROP_LATITUDE,
      "latitude");

  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
  gtk_widget_class_set_css_name (widget_class, g_intern_static_string ("map-marker"));
}

static void
shumate_marker_init (ShumateMarker *self)
{
  ShumateMarkerPrivate *priv = shumate_marker_get_instance_private (self);

  priv->lat = 0;
  priv->lon = 0;
  priv->selected = FALSE;
  priv->selectable = TRUE;
  priv->draggable = FALSE;
}

static void
location_interface_init (ShumateLocationInterface *iface)
{
  iface->get_latitude = shumate_marker_get_latitude;
  iface->get_longitude = shumate_marker_get_longitude;
  iface->set_location = shumate_marker_set_location;
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


/*
 * shumate_marker_set_selected:
 * @marker: a #ShumateMarker
 * @value: the selected state
 *
 * Sets the marker as selected or not. This will affect the "Selected" look
 * of the marker.
 */
void
shumate_marker_set_selected (ShumateMarker *marker,
                             gboolean       value)
{
  ShumateMarkerPrivate *priv = shumate_marker_get_instance_private (marker);

  g_return_if_fail (SHUMATE_IS_MARKER (marker));

  if (!priv->selectable)
    return;

  if (priv->selected != value)
    {
      priv->selected = value;
      if (value)
        gtk_widget_set_state_flags (GTK_WIDGET (marker),
                                    GTK_STATE_FLAG_SELECTED, FALSE);
      else
        gtk_widget_unset_state_flags (GTK_WIDGET (marker),
                                      GTK_STATE_FLAG_SELECTED);
    }
}


/**
 * shumate_marker_is_selected:
 * @marker: a #ShumateMarker
 *
 * Checks whether the marker is selected.
 *
 * Returns: the selected or not state of the marker.
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
 * shumate_marker_set_draggable:
 * @marker: a #ShumateMarker
 * @value: the draggable state
 *
 * Sets the marker as draggable or not.
 */
void
shumate_marker_set_draggable (ShumateMarker *marker,
                              gboolean       value)
{
  ShumateMarkerPrivate *priv = shumate_marker_get_instance_private (marker);

  g_return_if_fail (SHUMATE_IS_MARKER (marker));

  priv->draggable = value;

  g_object_notify_by_pspec (G_OBJECT (marker), obj_properties[PROP_DRAGGABLE]);
}


/**
 * shumate_marker_get_draggable:
 * @marker: a #ShumateMarker
 *
 * Checks whether the marker is draggable.
 *
 * Returns: the draggable or not state of the marker.
 */
gboolean
shumate_marker_get_draggable (ShumateMarker *marker)
{
  ShumateMarkerPrivate *priv = shumate_marker_get_instance_private (marker);

  g_return_val_if_fail (SHUMATE_IS_MARKER (marker), FALSE);

  return priv->draggable;
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
