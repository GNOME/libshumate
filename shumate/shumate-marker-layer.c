/*
 * Copyright (C) 2008-2009 Pierre-Luc Beaudoin <pierre-luc@pierlux.com>
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
 * ShumateMarkerLayer:
 *
 * Displays markers on the map. It is responsible for positioning markers
 * correctly, marker selections and group marker operations.
 */

#include "shumate-marker-layer.h"
#include "shumate-marker-private.h"

#include "shumate-enum-types.h"

#include <cairo/cairo-gobject.h>
#include <glib.h>

enum
{
  PROP_SELECTION_MODE = 1,
  N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

enum
{
  MARKER_SELECTED,
  MARKER_UNSELECTED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];


typedef struct
{
  GtkSelectionMode mode;
  GList *selected;
} ShumateMarkerLayerPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (ShumateMarkerLayer, shumate_marker_layer, SHUMATE_TYPE_LAYER);

static void
on_click_gesture_released (ShumateMarkerLayer *self,
                           int                 n_press,
                           double              x,
                           double              y,
                           GtkGestureClick    *gesture)
{
  ShumateMarkerLayerPrivate *priv = shumate_marker_layer_get_instance_private (self);
  GtkWidget *self_widget = GTK_WIDGET (self);
  GtkWidget *child;
  ShumateMarker *marker;

  child = gtk_widget_pick (self_widget, x, y, GTK_PICK_DEFAULT);
  if (!child)
    return;

  while (child != NULL && gtk_widget_get_parent (child) != self_widget)
    child = gtk_widget_get_parent (child);

  marker = SHUMATE_MARKER (child);
  if (!marker)
    return;

  if (shumate_marker_is_selected (marker)) {
    if (priv->mode != GTK_SELECTION_BROWSE) {
      shumate_marker_layer_unselect_marker (self, marker);
    }
  }
  else {
    shumate_marker_layer_select_marker (self, marker);
  }
}

static void
update_marker_visibility (ShumateMarkerLayer *layer,
                          ShumateMarker      *marker)
{
  ShumateViewport *viewport;
  ShumateMapSource *map_source;
  gboolean within_viewport;
  double lon, lat;
  double x, y;
  int marker_width, marker_height;
  int width, height;

  g_assert (SHUMATE_IS_MARKER_LAYER (layer));

  viewport = shumate_layer_get_viewport (SHUMATE_LAYER (layer));
  map_source = shumate_viewport_get_reference_map_source (viewport);
  if (!map_source)
    return;

  lon = shumate_location_get_longitude (SHUMATE_LOCATION (marker));
  lat = shumate_location_get_latitude (SHUMATE_LOCATION (marker));

  width = gtk_widget_get_width (GTK_WIDGET (layer));
  height = gtk_widget_get_height (GTK_WIDGET (layer));

  gtk_widget_measure (GTK_WIDGET (marker), GTK_ORIENTATION_HORIZONTAL, -1, 0, &marker_width, NULL, NULL);
  gtk_widget_measure (GTK_WIDGET (marker), GTK_ORIENTATION_VERTICAL, -1, 0, &marker_height, NULL, NULL);

  shumate_viewport_location_to_widget_coords (viewport, GTK_WIDGET (layer), lat, lon, &x, &y);
  x = floorf (x - marker_width/2.f);
  y = floorf (y - marker_height/2.f);

  within_viewport = x > -marker_width && x <= width &&
                    y > -marker_height && y <= height &&
                    marker_width < width && marker_height < height;

  gtk_widget_set_child_visible (GTK_WIDGET (marker), within_viewport);

  if (within_viewport)
    {
      GtkAllocation marker_allocation;

      gtk_widget_get_allocation (GTK_WIDGET (marker), &marker_allocation);

      if (marker_allocation.x != (int)x || marker_allocation.y != (int)y)
        gtk_widget_queue_allocate (GTK_WIDGET (layer));
    }
}

static void
shumate_marker_layer_reposition_markers (ShumateMarkerLayer *self)
{
  GtkWidget *child;

  for (child = gtk_widget_get_first_child (GTK_WIDGET (self));
       child != NULL;
       child = gtk_widget_get_next_sibling (child))
    {
      update_marker_visibility (self, SHUMATE_MARKER (child));
    }
}

static void
on_view_longitude_changed (ShumateMarkerLayer *self,
                           GParamSpec      *pspec,
                           ShumateViewport *view)
{
  g_assert (SHUMATE_IS_MARKER_LAYER (self));

  shumate_marker_layer_reposition_markers (self);
}

static void
on_view_latitude_changed (ShumateMarkerLayer *self,
                          GParamSpec      *pspec,
                          ShumateViewport *view)
{
  g_assert (SHUMATE_IS_MARKER_LAYER (self));

  shumate_marker_layer_reposition_markers (self);
}

static void
on_view_zoom_level_changed (ShumateMarkerLayer *self,
                            GParamSpec      *pspec,
                            ShumateViewport *view)
{
  g_assert (SHUMATE_IS_MARKER_LAYER (self));

  shumate_marker_layer_reposition_markers (self);
}

static void
on_view_rotation_changed (ShumateMarkerLayer *self,
                          GParamSpec      *pspec,
                          ShumateViewport *view)
{
  g_assert (SHUMATE_IS_MARKER_LAYER (self));

  shumate_marker_layer_reposition_markers (self);
}

static void
shumate_marker_layer_size_allocate (GtkWidget *widget,
                                    int        width,
                                    int        height,
                                    int        baseline)
{
  ShumateMarkerLayer *self = SHUMATE_MARKER_LAYER (widget);
  ShumateViewport *viewport;
  GtkAllocation allocation;
  GtkWidget *child;

  viewport = shumate_layer_get_viewport (SHUMATE_LAYER (self));

  for (child = gtk_widget_get_first_child (GTK_WIDGET (self));
       child != NULL;
       child = gtk_widget_get_next_sibling (child))
    {
      gboolean within_viewport;
      double lon, lat;
      double x, y;
      int marker_width, marker_height;

      if (!gtk_widget_should_layout (child))
        continue;

      lon = shumate_location_get_longitude (SHUMATE_LOCATION (child));
      lat = shumate_location_get_latitude (SHUMATE_LOCATION (child));

      gtk_widget_measure (child, GTK_ORIENTATION_HORIZONTAL, -1, 0, &marker_width, NULL, NULL);
      gtk_widget_measure (child, GTK_ORIENTATION_VERTICAL, -1, 0, &marker_height, NULL, NULL);

      shumate_viewport_location_to_widget_coords (viewport, widget, lat, lon, &x, &y);
      x = floorf (x - marker_width/2.f);
      y = floorf (y - marker_height/2.f);

      allocation.x = x;
      allocation.y = y;
      allocation.width = marker_width;
      allocation.height = marker_height;

      within_viewport = x > -allocation.width && x <= width &&
                        y > -allocation.height && y <= height &&
                        allocation.width < width && allocation.height < height;

      gtk_widget_set_child_visible (child, within_viewport);

      if (within_viewport)
        gtk_widget_size_allocate (child, &allocation, -1);
    }
}

static void
shumate_marker_layer_get_property (GObject *object,
    guint property_id,
    G_GNUC_UNUSED GValue *value,
    GParamSpec *pspec)
{
  ShumateMarkerLayer *self = SHUMATE_MARKER_LAYER (object);
  ShumateMarkerLayerPrivate *priv = shumate_marker_layer_get_instance_private (self);

  switch (property_id)
    {
    case PROP_SELECTION_MODE:
      g_value_set_enum (value, priv->mode);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}


static void
shumate_marker_layer_set_property (GObject *object,
    guint property_id,
    G_GNUC_UNUSED const GValue *value,
    GParamSpec *pspec)
{
  ShumateMarkerLayer *self = SHUMATE_MARKER_LAYER (object);

  switch (property_id)
    {
    case PROP_SELECTION_MODE:
      shumate_marker_layer_set_selection_mode (self, g_value_get_enum (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}


static void
shumate_marker_layer_dispose (GObject *object)
{
  ShumateMarkerLayer *self = SHUMATE_MARKER_LAYER (object);
  ShumateViewport *viewport = shumate_layer_get_viewport (SHUMATE_LAYER (self));
  GtkWidget *child;

  g_signal_handlers_disconnect_by_data (viewport, self);

  while ((child = gtk_widget_get_first_child (GTK_WIDGET (object))))
    gtk_widget_unparent (child);

  G_OBJECT_CLASS (shumate_marker_layer_parent_class)->dispose (object);
}


static void
shumate_marker_layer_finalize (GObject *object)
{
  ShumateMarkerLayer *self = SHUMATE_MARKER_LAYER (object);
  ShumateMarkerLayerPrivate *priv = shumate_marker_layer_get_instance_private (self);

  g_list_free (priv->selected);

  G_OBJECT_CLASS (shumate_marker_layer_parent_class)->finalize (object);
}

static void
shumate_marker_layer_constructed (GObject *object)
{
  ShumateMarkerLayer *self = SHUMATE_MARKER_LAYER (object);
  ShumateViewport *viewport;

  G_OBJECT_CLASS (shumate_marker_layer_parent_class)->constructed (object);

  viewport = shumate_layer_get_viewport (SHUMATE_LAYER (self));
  g_signal_connect_swapped (viewport, "notify::longitude", G_CALLBACK (on_view_longitude_changed), self);
  g_signal_connect_swapped (viewport, "notify::latitude", G_CALLBACK (on_view_latitude_changed), self);
  g_signal_connect_swapped (viewport, "notify::zoom-level", G_CALLBACK (on_view_zoom_level_changed), self);
  g_signal_connect_swapped (viewport, "notify::rotation", G_CALLBACK (on_view_rotation_changed), self);

}

static void
shumate_marker_layer_class_init (ShumateMarkerLayerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = shumate_marker_layer_dispose;
  object_class->finalize = shumate_marker_layer_finalize;
  object_class->get_property = shumate_marker_layer_get_property;
  object_class->set_property = shumate_marker_layer_set_property;
  object_class->constructed = shumate_marker_layer_constructed;

  widget_class->size_allocate = shumate_marker_layer_size_allocate;

  /**
   * ShumateMarkerLayer:selection-mode:
   *
   * Determines the type of selection that will be performed.
   */
  obj_properties[PROP_SELECTION_MODE] =
    g_param_spec_enum ("selection-mode",
                       "Selection Mode",
                       "Determines the type of selection that will be performed.",
                       GTK_TYPE_SELECTION_MODE,
                       GTK_SELECTION_NONE,
                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPERTIES, obj_properties);

  /**
   * ShumateMarkerLayer::marker-selected:
   * @self: The marker layer emitting the signal
   * @marker: The marker that was selected
   *
   * Emitted when a marker in the layer is selected.
   */
  signals[MARKER_SELECTED] =
    g_signal_new ("marker-selected",
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL, NULL,
                  G_TYPE_NONE,
                  1, SHUMATE_TYPE_MARKER);

  /**
   * ShumateMarkerLayer::marker-unselected:
   * @self: The marker layer emitting the signal
   * @marker: The marker that was unselected
   *
   * Emitted when a marker in the layer is unselected.
   */
  signals[MARKER_UNSELECTED] =
    g_signal_new ("marker-unselected",
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL, NULL,
                  G_TYPE_NONE,
                  1, SHUMATE_TYPE_MARKER);
}


static void
shumate_marker_layer_init (ShumateMarkerLayer *self)
{
  ShumateMarkerLayerPrivate *priv = shumate_marker_layer_get_instance_private (self);
  GtkGesture *click_gesture;

  priv->mode = GTK_SELECTION_NONE;

  click_gesture = gtk_gesture_click_new ();
  gtk_widget_add_controller (GTK_WIDGET (self), GTK_EVENT_CONTROLLER (click_gesture));
  g_signal_connect_swapped (click_gesture, "released", G_CALLBACK (on_click_gesture_released), self);
}

/**
 * shumate_marker_layer_new:
 * @viewport: the @ShumateViewport
 *
 * Creates a new instance of #ShumateMarkerLayer.
 *
 * Returns: a new #ShumateMarkerLayer ready to be used as a container for the markers.
 */
ShumateMarkerLayer *
shumate_marker_layer_new (ShumateViewport *viewport)
{
  return g_object_new (SHUMATE_TYPE_MARKER_LAYER,
                       "viewport", viewport,
                       NULL);
}


/**
 * shumate_marker_layer_new_full:
 * @viewport: the @ShumateViewport
 * @mode: Selection mode
 *
 * Creates a new instance of #ShumateMarkerLayer with the specified selection mode.
 *
 * Returns: a new #ShumateMarkerLayer ready to be used as a container for the markers.
 */
ShumateMarkerLayer *
shumate_marker_layer_new_full (ShumateViewport *viewport,
                               GtkSelectionMode mode)
{
  return g_object_new (SHUMATE_TYPE_MARKER_LAYER,
                       "selection-mode", mode,
                       "viewport", viewport,
                       NULL);
}


static void
marker_position_notify (ShumateMarker *marker,
    G_GNUC_UNUSED GParamSpec *pspec,
    ShumateMarkerLayer *layer)
{
  update_marker_visibility (layer, marker);
}


static void
marker_move_by_cb (ShumateMarker *marker,
    double dx,
    double dy,
    GdkEvent *event,
    ShumateMarkerLayer *layer)
{
  /*ShumateMarkerLayerPrivate *priv = shumate_marker_layer_get_instance_private (layer);
  ShumateView *view = priv->view;
  double x, y, lat, lon;

  x = shumate_view_longitude_to_x (view, shumate_location_get_longitude (SHUMATE_LOCATION (marker)));
  y = shumate_view_latitude_to_y (view, shumate_location_get_latitude (SHUMATE_LOCATION (marker)));

  x += dx;
  y += dy;

  lon = shumate_view_x_to_longitude (view, x);
  lat = shumate_view_y_to_latitude (view, y);

  shumate_location_set_location (SHUMATE_LOCATION (marker), lat, lon);*/
}


/**
 * shumate_marker_layer_add_marker:
 * @layer: a #ShumateMarkerLayer
 * @marker: a #ShumateMarker
 *
 * Adds the marker to the layer.
 */
void
shumate_marker_layer_add_marker (ShumateMarkerLayer *layer,
    ShumateMarker *marker)
{
  g_return_if_fail (SHUMATE_IS_MARKER_LAYER (layer));
  g_return_if_fail (SHUMATE_IS_MARKER (marker));

  g_signal_connect_object (G_OBJECT (marker), "notify::latitude",
      G_CALLBACK (marker_position_notify), layer, 0);
  g_signal_connect_object (G_OBJECT (marker), "notify::longitude",
      G_CALLBACK (marker_position_notify), layer, 0);

  /*g_signal_connect (G_OBJECT (marker), "drag-motion",
      G_CALLBACK (marker_move_by_cb), layer);*/

  shumate_marker_set_selected (marker, FALSE);

  gtk_widget_insert_before (GTK_WIDGET(marker), GTK_WIDGET (layer), NULL);
  update_marker_visibility (layer, marker);
}


/**
 * shumate_marker_layer_remove_all:
 * @layer: a #ShumateMarkerLayer
 *
 * Removes all markers from the layer.
 */
void
shumate_marker_layer_remove_all (ShumateMarkerLayer *layer)
{
  GtkWidget *child;

  g_return_if_fail (SHUMATE_IS_MARKER_LAYER (layer));

  child = gtk_widget_get_first_child (GTK_WIDGET (layer));
  while (child)
    {
      GtkWidget *next = gtk_widget_get_next_sibling (child);

      g_signal_handlers_disconnect_by_data (child, layer);
      gtk_widget_unparent (child);

      child = next;
    }
}


/**
 * shumate_marker_layer_get_markers:
 * @layer: a #ShumateMarkerLayer
 *
 * Gets a copy of the list of all markers inserted into the layer. You should
 * free the list but not its contents.
 *
 * Returns: (transfer container) (element-type ShumateMarker): the list
 */
GList *
shumate_marker_layer_get_markers (ShumateMarkerLayer *layer)
{
  GList *list = NULL;
  GtkWidget *child;

  g_return_val_if_fail (SHUMATE_IS_MARKER_LAYER (layer), NULL);

  for (child = gtk_widget_get_last_child (GTK_WIDGET (layer));
       child != NULL;
       child = gtk_widget_get_prev_sibling (child))
    {
      ShumateMarker *marker = SHUMATE_MARKER (child);
      list = g_list_prepend (list, marker);
    }

  return list;
}


/**
 * shumate_marker_layer_get_selected:
 * @layer: a #ShumateMarkerLayer
 *
 * Gets a list of selected markers in the layer.
 *
 * Returns: (transfer container) (element-type ShumateMarker): the list
 */
GList *
shumate_marker_layer_get_selected (ShumateMarkerLayer *self)
{
  ShumateMarkerLayerPrivate *priv;
  g_return_val_if_fail (SHUMATE_IS_MARKER_LAYER (self), FALSE);
  priv = shumate_marker_layer_get_instance_private (self);
  return g_list_copy (priv->selected);
}


/**
 * shumate_marker_layer_select_marker:
 * @self: a #ShumateMarkerLayer
 * @marker: a #ShumateMarker that is a child of @self
 *
 * Selects a marker in this layer.
 *
 * If #ShumateMarkerLayer:selection-mode is %GTK_SELECTION_SINGLE or
 * %GTK_SELECTION_BROWSE, all other markers will be unselected. If the mode is
 * %GTK_SELECTION_NONE or @marker is not selectable, nothing will happen.
 *
 * Returns: %TRUE if the marker is now selected, otherwise %FALSE
 */
gboolean
shumate_marker_layer_select_marker (ShumateMarkerLayer *self, ShumateMarker *marker)
{
  ShumateMarkerLayerPrivate *priv;

  g_return_val_if_fail (SHUMATE_IS_MARKER_LAYER (self), FALSE);
  g_return_val_if_fail (SHUMATE_IS_MARKER (marker), FALSE);
  g_return_val_if_fail (gtk_widget_get_parent (GTK_WIDGET (marker)) == GTK_WIDGET (self), FALSE);

  priv = shumate_marker_layer_get_instance_private (self);

  if (!shumate_marker_get_selectable (marker)) {
    return FALSE;
  }

  if (shumate_marker_is_selected (marker)) {
    return TRUE;
  }

  switch (priv->mode) {
    case GTK_SELECTION_NONE:
      return FALSE;

    case GTK_SELECTION_BROWSE:
    case GTK_SELECTION_SINGLE:
      shumate_marker_layer_unselect_all_markers (self);

      /* fall through */
    case GTK_SELECTION_MULTIPLE:
      priv->selected = g_list_prepend (priv->selected, marker);
      shumate_marker_set_selected (marker, TRUE);
      g_signal_emit (self, signals[MARKER_SELECTED], 0, marker);
      return TRUE;

    default:
      g_assert_not_reached ();
  }
}


/**
 * shumate_marker_layer_unselect_marker:
 * @self: a #ShumateMarkerLayer
 * @marker: a #ShumateMarker that is a child of @self
 *
 * Unselects a marker in this layer.
 *
 * This works even if #ShumateMarkerLayer:selection-mode is
 * %GTK_SELECTION_BROWSE. Browse mode only prevents user interaction, not the
 * program, from unselecting a marker.
 */
void
shumate_marker_layer_unselect_marker (ShumateMarkerLayer *self, ShumateMarker *marker)
{
  ShumateMarkerLayerPrivate *priv;

  g_return_if_fail (SHUMATE_IS_MARKER_LAYER (self));
  g_return_if_fail (SHUMATE_IS_MARKER (marker));
  g_return_if_fail (gtk_widget_get_parent (GTK_WIDGET (marker)) == GTK_WIDGET (self));

  priv = shumate_marker_layer_get_instance_private (self);

  if (!shumate_marker_is_selected (marker)) {
    return;
  }

  priv->selected = g_list_remove (priv->selected, marker);
  shumate_marker_set_selected (marker, FALSE);
  g_signal_emit (self, signals[MARKER_UNSELECTED], 0, marker);
}


/**
 * shumate_marker_layer_remove_marker:
 * @layer: a #ShumateMarkerLayer
 * @marker: a #ShumateMarker
 *
 * Removes the marker from the layer.
 */
void
shumate_marker_layer_remove_marker (ShumateMarkerLayer *layer,
    ShumateMarker *marker)
{
  g_return_if_fail (SHUMATE_IS_MARKER_LAYER (layer));
  g_return_if_fail (SHUMATE_IS_MARKER (marker));
  g_return_if_fail (gtk_widget_get_parent (GTK_WIDGET (marker)) == GTK_WIDGET (layer));

  g_signal_handlers_disconnect_by_func (G_OBJECT (marker),
      G_CALLBACK (marker_position_notify), layer);

  g_signal_handlers_disconnect_by_func (marker,
      G_CALLBACK (marker_move_by_cb), layer);

  if (shumate_marker_is_selected (marker)) {
    shumate_marker_layer_unselect_marker (layer, marker);
  }

  gtk_widget_unparent (GTK_WIDGET (marker));
}


/**
 * shumate_marker_layer_animate_in_all_markers:
 * @layer: a #ShumateMarkerLayer
 *
 * Fade in all markers in the layer with an animation
 */
void
shumate_marker_layer_animate_in_all_markers (ShumateMarkerLayer *layer)
{
  /*
  ClutterActorIter iter;
  ClutterActor *child;
  guint delay = 0;

  g_return_if_fail (SHUMATE_IS_MARKER_LAYER (layer));

  clutter_actor_iter_init (&iter, CLUTTER_ACTOR (layer));
  while (clutter_actor_iter_next (&iter, &child))
    {
      ShumateMarker *marker = SHUMATE_MARKER (child);

      shumate_marker_animate_in_with_delay (marker, delay);
      delay += 50;
    }
   */
}


/**
 * shumate_marker_layer_animate_out_all_markers:
 * @layer: a #ShumateMarkerLayer
 *
 * Fade out all markers in the layer with an animation
 */
void
shumate_marker_layer_animate_out_all_markers (ShumateMarkerLayer *layer)
{
  /*
  ClutterActorIter iter;
  ClutterActor *child;
  guint delay = 0;

  g_return_if_fail (SHUMATE_IS_MARKER_LAYER (layer));

  clutter_actor_iter_init (&iter, CLUTTER_ACTOR (layer));
  while (clutter_actor_iter_next (&iter, &child))
    {
      ShumateMarker *marker = SHUMATE_MARKER (child);

      shumate_marker_animate_out_with_delay (marker, delay);
      delay += 50;
    }
   */
}


/**
 * shumate_marker_layer_show_all_markers:
 * @layer: a #ShumateMarkerLayer
 *
 * Shows all markers in the layer
 */
void
shumate_marker_layer_show_all_markers (ShumateMarkerLayer *layer)
{
  /*
  ClutterActorIter iter;
  ClutterActor *child;

  g_return_if_fail (SHUMATE_IS_MARKER_LAYER (layer));

  clutter_actor_iter_init (&iter, CLUTTER_ACTOR (layer));
  while (clutter_actor_iter_next (&iter, &child))
    {
      ClutterActor *actor = CLUTTER_ACTOR (child);

      clutter_actor_show (actor);
    }
   */
}


/**
 * shumate_marker_layer_hide_all_markers:
 * @layer: a #ShumateMarkerLayer
 *
 * Hides all the markers in the layer
 */
void
shumate_marker_layer_hide_all_markers (ShumateMarkerLayer *layer)
{
  /*
  ClutterActorIter iter;
  ClutterActor *child;

  g_return_if_fail (SHUMATE_IS_MARKER_LAYER (layer));

  clutter_actor_iter_init (&iter, CLUTTER_ACTOR (layer));
  while (clutter_actor_iter_next (&iter, &child))
    {
      ClutterActor *actor = CLUTTER_ACTOR (child);

      clutter_actor_hide (actor);
    }
   */
}


/**
 * shumate_marker_layer_set_all_markers_draggable:
 * @layer: a #ShumateMarkerLayer
 *
 * Sets all markers draggable in the layer
 */
void
shumate_marker_layer_set_all_markers_draggable (ShumateMarkerLayer *layer)
{
  /*
  ClutterActorIter iter;
  ClutterActor *child;

  g_return_if_fail (SHUMATE_IS_MARKER_LAYER (layer));

  clutter_actor_iter_init (&iter, CLUTTER_ACTOR (layer));
  while (clutter_actor_iter_next (&iter, &child))
    {
      ShumateMarker *marker = SHUMATE_MARKER (child);

      shumate_marker_set_draggable (marker, TRUE);
    }
   */
}


/**
 * shumate_marker_layer_set_all_markers_undraggable:
 * @layer: a #ShumateMarkerLayer
 *
 * Sets all markers undraggable in the layer
 */
void
shumate_marker_layer_set_all_markers_undraggable (ShumateMarkerLayer *layer)
{
  /*
  ClutterActorIter iter;
  ClutterActor *child;

  g_return_if_fail (SHUMATE_IS_MARKER_LAYER (layer));

  clutter_actor_iter_init (&iter, CLUTTER_ACTOR (layer));
  while (clutter_actor_iter_next (&iter, &child))
    {
      ShumateMarker *marker = SHUMATE_MARKER (child);

      shumate_marker_set_draggable (marker, FALSE);
    }
   */
}


/**
 * shumate_marker_layer_unselect_all_markers:
 * @layer: a #ShumateMarkerLayer
 *
 * Unselects all markers in the layer.
 */
void
shumate_marker_layer_unselect_all_markers (ShumateMarkerLayer *self)
{
  ShumateMarkerLayerPrivate *priv;
  g_autoptr(GList) prev_selected = NULL;

  g_return_if_fail (SHUMATE_IS_MARKER_LAYER (self));

  priv = shumate_marker_layer_get_instance_private (self);
  prev_selected = g_list_copy (priv->selected);

  for (GList *l = prev_selected; l != NULL; l = l->next) {
    shumate_marker_layer_unselect_marker (self, SHUMATE_MARKER (l->data));
  }
}


/**
 * shumate_marker_layer_select_all_markers:
 * @layer: a #ShumateMarkerLayer
 *
 * Selects all selectable markers in the layer.
 */
void
shumate_marker_layer_select_all_markers (ShumateMarkerLayer *self)
{
  g_autoptr(GList) children = NULL;

  g_return_if_fail (SHUMATE_IS_MARKER_LAYER (self));

  children = shumate_marker_layer_get_markers (self);

  for (GList *l = children; l != NULL; l = l->next) {
    shumate_marker_layer_select_marker (self, SHUMATE_MARKER (l->data));
  }
}


/**
 * shumate_marker_layer_set_selection_mode:
 * @layer: a #ShumateMarkerLayer
 * @mode: a #GtkSelectionMode value
 *
 * Sets the selection mode of the layer.
 *
 * NOTE: changing selection mode to %GTK_SELECTION_NONE, %GTK_SELECTION_SINGLE
 * or %GTK_SELECTION_BROWSE will clear all previously selected markers.
 */
void
shumate_marker_layer_set_selection_mode (ShumateMarkerLayer *layer,
                                         GtkSelectionMode    mode)
{
  ShumateMarkerLayerPrivate *priv = shumate_marker_layer_get_instance_private (layer);

  g_return_if_fail (SHUMATE_IS_MARKER_LAYER (layer));

  if (priv->mode == mode)
    return;

  priv->mode = mode;

  if (mode != GTK_SELECTION_MULTIPLE)
    shumate_marker_layer_unselect_all_markers (layer);

  g_object_notify_by_pspec (G_OBJECT (layer), obj_properties[PROP_SELECTION_MODE]);
}


/**
 * shumate_marker_layer_get_selection_mode:
 * @layer: a #ShumateMarkerLayer
 *
 * Gets the selection mode of the layer.
 *
 * Returns: the selection mode of the layer.
 */
GtkSelectionMode
shumate_marker_layer_get_selection_mode (ShumateMarkerLayer *layer)
{
  ShumateMarkerLayerPrivate *priv = shumate_marker_layer_get_instance_private (layer);

  g_return_val_if_fail (SHUMATE_IS_MARKER_LAYER (layer), GTK_SELECTION_NONE);

  return priv->mode;
}
