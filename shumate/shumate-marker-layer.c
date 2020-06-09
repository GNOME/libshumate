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
 * SECTION:shumate-marker-layer
 * @short_description: A container for #ShumateMarker
 *
 * A ShumateMarkerLayer displays markers on the map. It is responsible for
 * positioning markers correctly, marker selections and group marker operations.
 */

#include "config.h"

#include "shumate-marker-layer.h"
#include "shumate-marker-private.h"

#include "shumate-defines.h"
#include "shumate-enum-types.h"
#include "shumate-view.h"

#include <cairo/cairo-gobject.h>
#include <glib.h>

enum
{
  PROP_SELECTION_MODE = 1,
  N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

typedef struct
{
  GtkSelectionMode mode;
  ShumateView *view;
} ShumateMarkerLayerPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (ShumateMarkerLayer, shumate_marker_layer, SHUMATE_TYPE_LAYER);

static void set_view (ShumateLayer *layer,
    ShumateView *view);

static ShumateBoundingBox *get_bounding_box (ShumateLayer *layer);

static void
set_selected_all_but_one (ShumateMarkerLayer *self,
    ShumateMarker *not_selected,
    gboolean select)
{
  ShumateMarkerLayerPrivate *priv = shumate_marker_layer_get_instance_private (self);
  GtkWidget *child;

  for (child = gtk_widget_get_first_child (GTK_WIDGET (self));
       child != NULL;
       child = gtk_widget_get_next_sibling (child))
    {
      ShumateMarker *marker = SHUMATE_MARKER (child);

      if (marker != not_selected)
        {
          shumate_marker_set_selected (marker, select);
          shumate_marker_set_selectable (marker, priv->mode != GTK_SELECTION_NONE);
        }
    }
}

static void
on_click_gesture_released (ShumateMarkerLayer *self,
                           gint                n_press,
                           gdouble             x,
                           gdouble             y,
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

  if (priv->mode != GTK_SELECTION_BROWSE || !shumate_marker_is_selected (marker))
    shumate_marker_set_selected (marker, !shumate_marker_is_selected (marker));

  if ((priv->mode == GTK_SELECTION_SINGLE || priv->mode == GTK_SELECTION_BROWSE) && shumate_marker_is_selected (marker))
    set_selected_all_but_one (self, marker, FALSE);
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
  ShumateMarkerLayerPrivate *priv = shumate_marker_layer_get_instance_private (self);

  if (priv->view != NULL)
    set_view (SHUMATE_LAYER (self), NULL);

  G_OBJECT_CLASS (shumate_marker_layer_parent_class)->dispose (object);
}

static void
shumate_marker_layer_class_init (ShumateMarkerLayerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  ShumateLayerClass *layer_class = SHUMATE_LAYER_CLASS (klass);

  object_class->dispose = shumate_marker_layer_dispose;
  object_class->get_property = shumate_marker_layer_get_property;
  object_class->set_property = shumate_marker_layer_set_property;

  layer_class->set_view = set_view;
  layer_class->get_bounding_box = get_bounding_box;

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
  
  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_FIXED_LAYOUT);
}


static void
shumate_marker_layer_init (ShumateMarkerLayer *self)
{
  ShumateMarkerLayerPrivate *priv = shumate_marker_layer_get_instance_private (self);
  GtkGesture *click_gesture;

  priv->mode = GTK_TYPE_SELECTION_MODE;
  priv->view = NULL;

  click_gesture = gtk_gesture_click_new ();
  gtk_widget_add_controller (GTK_WIDGET (self), GTK_EVENT_CONTROLLER (click_gesture));
  g_signal_connect_swapped (click_gesture, "released", G_CALLBACK (on_click_gesture_released), self);
}

/**
 * shumate_marker_layer_new:
 *
 * Creates a new instance of #ShumateMarkerLayer.
 *
 * Returns: a new #ShumateMarkerLayer ready to be used as a container for the markers.
 */
ShumateMarkerLayer *
shumate_marker_layer_new ()
{
  return g_object_new (SHUMATE_TYPE_MARKER_LAYER, NULL);
}


/**
 * shumate_marker_layer_new_full:
 * @mode: Selection mode
 *
 * Creates a new instance of #ShumateMarkerLayer with the specified selection mode.
 *
 * Returns: a new #ShumateMarkerLayer ready to be used as a container for the markers.
 */
ShumateMarkerLayer *
shumate_marker_layer_new_full (GtkSelectionMode mode)
{
  return g_object_new (SHUMATE_TYPE_MARKER_LAYER, "selection-mode", mode, NULL);
}

static void
set_marker_position (ShumateMarkerLayer *layer,
                     ShumateMarker      *marker)
{
  ShumateMarkerLayerPrivate *priv = shumate_marker_layer_get_instance_private (layer);
  gint x, y, origin_x, origin_y;
  int marker_width, marker_height;
  int width, height;

  g_assert (SHUMATE_IS_MARKER_LAYER (layer));

  /* layer not yet added to the view */
  if (priv->view == NULL)
    return;

  shumate_view_get_viewport_origin (priv->view, &origin_x, &origin_y);
  x = shumate_view_longitude_to_x (priv->view,
        shumate_location_get_longitude (SHUMATE_LOCATION (marker))) + origin_x;
  y = shumate_view_latitude_to_y (priv->view,
        shumate_location_get_latitude (SHUMATE_LOCATION (marker))) + origin_y;

  x -= origin_x;
  y -= origin_y;
  marker_width = gtk_widget_get_width (GTK_WIDGET (marker));
  marker_height = gtk_widget_get_height (GTK_WIDGET (marker));
  width = gtk_widget_get_width (GTK_WIDGET (layer));
  height = gtk_widget_get_height (GTK_WIDGET (layer));
  if (x < 0 && -x < marker_width/2)
    {
      gtk_widget_set_visible (GTK_WIDGET (marker), FALSE);
      return;
    }
  else if (y < 0 && -y < marker_height/2)
    {
      gtk_widget_set_visible (GTK_WIDGET (marker), FALSE);
      return;
    }
  else if (x > width + marker_width/2)
    {
      gtk_widget_set_visible (GTK_WIDGET (marker), FALSE);
      return;
    }
  else if (y > height + marker_height/2)
    {
      gtk_widget_set_visible (GTK_WIDGET (marker), FALSE);
      return;
    }
  else
    {
      GtkLayoutManager *layout;
      GtkLayoutChild *layout_child;
      GskTransform *transform;

      layout = gtk_widget_get_layout_manager (GTK_WIDGET (layer));
      layout_child = gtk_layout_manager_get_layout_child (layout, GTK_WIDGET (marker));
      transform = gsk_transform_translate (NULL, &GRAPHENE_POINT_INIT (x - marker_width/2, y - marker_height/2));
      gtk_fixed_layout_child_set_transform (GTK_FIXED_LAYOUT_CHILD (layout_child), transform);
      gtk_widget_set_visible (GTK_WIDGET (marker), TRUE);
    }
}


static void
marker_position_notify (ShumateMarker *marker,
    G_GNUC_UNUSED GParamSpec *pspec,
    ShumateMarkerLayer *layer)
{
  set_marker_position (layer, marker);
}


static void
marker_move_by_cb (ShumateMarker *marker,
    gdouble dx,
    gdouble dy,
    GdkEvent *event,
    ShumateMarkerLayer *layer)
{
  ShumateMarkerLayerPrivate *priv = shumate_marker_layer_get_instance_private (layer);
  ShumateView *view = priv->view;
  gdouble x, y, lat, lon;

  x = shumate_view_longitude_to_x (view, shumate_location_get_longitude (SHUMATE_LOCATION (marker)));
  y = shumate_view_latitude_to_y (view, shumate_location_get_latitude (SHUMATE_LOCATION (marker)));

  x += dx;
  y += dy;

  lon = shumate_view_x_to_longitude (view, x);
  lat = shumate_view_y_to_latitude (view, y);

  shumate_location_set_location (SHUMATE_LOCATION (marker), lat, lon);
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
  ShumateMarkerLayerPrivate *priv = shumate_marker_layer_get_instance_private (layer);

  g_return_if_fail (SHUMATE_IS_MARKER_LAYER (layer));
  g_return_if_fail (SHUMATE_IS_MARKER (marker));

  shumate_marker_set_selectable (marker, priv->mode != GTK_SELECTION_NONE);

  g_signal_connect (G_OBJECT (marker), "notify::latitude",
      G_CALLBACK (marker_position_notify), layer);

  /*g_signal_connect (G_OBJECT (marker), "drag-motion",
      G_CALLBACK (marker_move_by_cb), layer);*/

  gtk_widget_insert_before (GTK_WIDGET(marker), GTK_WIDGET (layer), NULL);
  set_marker_position (layer, marker);
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
  //ClutterActorIter iter;
  //ClutterActor *child;

  g_return_if_fail (SHUMATE_IS_MARKER_LAYER (layer));

  /*
  clutter_actor_iter_init (&iter, CLUTTER_ACTOR (layer));
  while (clutter_actor_iter_next (&iter, &child))
    {
      GObject *marker = G_OBJECT (child);

      g_signal_handlers_disconnect_by_func (marker,
          G_CALLBACK (marker_position_notify), layer);

      g_signal_handlers_disconnect_by_func (marker,
          G_CALLBACK (marker_move_by_cb), layer);

      clutter_actor_iter_remove (&iter);
    }
   */
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
shumate_marker_layer_get_selected (ShumateMarkerLayer *layer)
{
  GList *selected = NULL;
  GtkWidget *child;

  g_return_val_if_fail (SHUMATE_IS_MARKER_LAYER (layer), NULL);

  for (child = gtk_widget_get_last_child (GTK_WIDGET (layer));
       child != NULL;
       child = gtk_widget_get_prev_sibling (child))
    {
      ShumateMarker *marker = SHUMATE_MARKER (child);

      if (shumate_marker_is_selected (marker))
        selected = g_list_prepend (selected, marker);
    }

  return selected;
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
shumate_marker_layer_unselect_all_markers (ShumateMarkerLayer *layer)
{
  g_return_if_fail (SHUMATE_IS_MARKER_LAYER (layer));

  set_selected_all_but_one (layer, NULL, FALSE);
}


/**
 * shumate_marker_layer_select_all_markers:
 * @layer: a #ShumateMarkerLayer
 *
 * Selects all markers in the layer.
 */
void
shumate_marker_layer_select_all_markers (ShumateMarkerLayer *layer)
{
  g_return_if_fail (SHUMATE_IS_MARKER_LAYER (layer));

  set_selected_all_but_one (layer, NULL, TRUE);
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
    set_selected_all_but_one (layer, NULL, FALSE);

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


static void
reposition (ShumateMarkerLayer *layer)
{
  /*
  ClutterActorIter iter;
  ClutterActor *child;

  g_return_if_fail (SHUMATE_IS_MARKER_LAYER (layer));

  clutter_actor_iter_init (&iter, CLUTTER_ACTOR (layer));
  while (clutter_actor_iter_next (&iter, &child))
    {
      ShumateMarker *marker = SHUMATE_MARKER (child);

      set_marker_position (layer, marker);
    }
   */
}


static void
relocate_cb (G_GNUC_UNUSED GObject *gobject,
    ShumateMarkerLayer *layer)
{
  g_return_if_fail (SHUMATE_IS_MARKER_LAYER (layer));

  reposition (layer);
}


static void
zoom_reposition_cb (G_GNUC_UNUSED GObject *gobject,
    G_GNUC_UNUSED GParamSpec *arg1,
    ShumateMarkerLayer *layer)
{
  g_return_if_fail (SHUMATE_IS_MARKER_LAYER (layer));

  reposition (layer);
}


static void
set_view (ShumateLayer *layer,
    ShumateView *view)
{
  ShumateMarkerLayer *marker_layer = SHUMATE_MARKER_LAYER (layer);
  ShumateMarkerLayerPrivate *priv = shumate_marker_layer_get_instance_private (marker_layer);

  g_return_if_fail (SHUMATE_IS_MARKER_LAYER (layer));
  g_return_if_fail (SHUMATE_IS_VIEW (view) || view == NULL);


  if (priv->view != NULL)
    {
      g_signal_handlers_disconnect_by_func (priv->view,
          G_CALLBACK (relocate_cb), marker_layer);
      g_object_unref (priv->view);
    }

  priv->view = view;

  if (view != NULL)
    {
      g_object_ref (view);

      g_signal_connect (view, "layer-relocated",
          G_CALLBACK (relocate_cb), layer);

      g_signal_connect (view, "notify::zoom-level",
          G_CALLBACK (zoom_reposition_cb), layer);

      reposition (marker_layer);
    }
}


static ShumateBoundingBox *
get_bounding_box (ShumateLayer *layer)
{
  //ClutterActorIter iter;
  //ClutterActor *child;
  ShumateBoundingBox *bbox;

  g_return_val_if_fail (SHUMATE_IS_MARKER_LAYER (layer), NULL);

  bbox = shumate_bounding_box_new ();

  /*
  clutter_actor_iter_init (&iter, CLUTTER_ACTOR (layer));
  while (clutter_actor_iter_next (&iter, &child))
    {
      ShumateMarker *marker = SHUMATE_MARKER (child);
      gdouble lat, lon;

      lat = shumate_location_get_latitude (SHUMATE_LOCATION (marker));
      lon = shumate_location_get_longitude (SHUMATE_LOCATION (marker));

      shumate_bounding_box_extend (bbox, lat, lon);
    }
   */

  if (bbox->left == bbox->right)
    {
      bbox->left -= 0.0001;
      bbox->right += 0.0001;
    }

  if (bbox->bottom == bbox->top)
    {
      bbox->bottom -= 0.0001;
      bbox->top += 0.0001;
    }

  return bbox;
}
