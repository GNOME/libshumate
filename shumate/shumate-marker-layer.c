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
#include "shumate-inspector-settings-private.h"

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


struct _ShumateMarkerLayer
{
  ShumateLayer parent_instance;

  GtkSelectionMode mode;
  GList *selected;

  int n_children;
};

G_DEFINE_TYPE (ShumateMarkerLayer, shumate_marker_layer, SHUMATE_TYPE_LAYER);

static void
on_click_gesture_released (ShumateMarkerLayer *self,
                           int                 n_press,
                           double              x,
                           double              y,
                           GtkGestureClick    *gesture)
{
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
    if (self->mode != GTK_SELECTION_BROWSE) {
      shumate_marker_layer_unselect_marker (self, marker);
    }
  }
  else {
    shumate_marker_layer_select_marker (self, marker);
  }
}

static void
calculate_local_marker_offset (ShumateMarker *marker,
                               int marker_width, int marker_height,
                               gdouble *x, gdouble *y)
{
  double hotspot_x, hotspot_y;

  g_assert (SHUMATE_IS_MARKER (marker));

  shumate_marker_get_hotspot (marker, &hotspot_x, &hotspot_y);
  if (G_UNLIKELY (hotspot_x > marker_width)) {
    g_warning ("Marker x hotspot (%lf) is more than the marker width (%d).", hotspot_x, marker_width);
    hotspot_x = marker_width;
  }

  if (G_UNLIKELY (hotspot_y > marker_height)) {
    g_warning ("Marker y hotspot (%lf) is more than the marker height (%d).", hotspot_y, marker_height);
    hotspot_y = marker_height;
  }

  if (hotspot_x < 0) {
    switch (gtk_widget_get_halign (GTK_WIDGET (marker))) {
    case GTK_ALIGN_START:
      if (gtk_widget_get_direction (GTK_WIDGET (marker)) == GTK_TEXT_DIR_RTL)
        *x -= marker_width;
      break;
    case GTK_ALIGN_END:
      if (gtk_widget_get_direction (GTK_WIDGET (marker)) != GTK_TEXT_DIR_RTL)
        *x -= marker_width;
      break;
    default:
      *x = floorf (*x - marker_width/2.f);
      break;
    }
  } else {
    if (gtk_widget_get_direction (GTK_WIDGET (marker)) == GTK_TEXT_DIR_RTL)
      *x -= marker_width - hotspot_x;
    else
      *x -= hotspot_x;
  }

  if (hotspot_y < 0) {
    switch (gtk_widget_get_valign (GTK_WIDGET (marker))) {
    case GTK_ALIGN_START:
      break;
    case GTK_ALIGN_END:
      *y -= marker_height;
      break;
    default:
      *y = floorf (*y - marker_height/2.f);
      break;
    }
  } else {
    *y -= hotspot_y;
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
  calculate_local_marker_offset (marker, marker_width, marker_height, &x, &y);

  within_viewport = x > -marker_width && x <= width &&
                    y > -marker_height && y <= height &&
                    marker_width < width && marker_height < height;

  gtk_widget_set_child_visible (GTK_WIDGET (marker), within_viewport);

  if (within_viewport)
    {
      graphene_rect_t bounds;

      if (gtk_widget_compute_bounds (GTK_WIDGET (marker), GTK_WIDGET (layer), &bounds))
        if (bounds.origin.x != (int)x || bounds.origin.y != (int)y)
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
on_viewport_changed (ShumateMarkerLayer *self,
                     GParamSpec         *pspec,
                     ShumateViewport    *view)
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
      calculate_local_marker_offset (SHUMATE_MARKER (child), marker_width, marker_height, &x, &y);

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
shumate_marker_layer_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  ShumateMarkerLayer *self = SHUMATE_MARKER_LAYER (object);

  switch (property_id)
    {
    case PROP_SELECTION_MODE:
      g_value_set_enum (value, self->mode);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}


static void
shumate_marker_layer_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
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

  g_list_free (self->selected);

  G_OBJECT_CLASS (shumate_marker_layer_parent_class)->finalize (object);
}

static void
shumate_marker_layer_constructed (GObject *object)
{
  ShumateMarkerLayer *self = SHUMATE_MARKER_LAYER (object);
  ShumateViewport *viewport;

  G_OBJECT_CLASS (shumate_marker_layer_parent_class)->constructed (object);

  viewport = shumate_layer_get_viewport (SHUMATE_LAYER (self));
  g_signal_connect_swapped (viewport, "notify", G_CALLBACK (on_viewport_changed), self);
}

static char *
shumate_marker_layer_get_debug_text (ShumateLayer *layer)
{
  ShumateMarkerLayer *self = SHUMATE_MARKER_LAYER (layer);
  return g_strdup_printf ("markers: %d, %d selected\n", self->n_children, g_list_length (self->selected));
}

static void
shumate_marker_layer_class_init (ShumateMarkerLayerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  ShumateLayerClass *layer_class = SHUMATE_LAYER_CLASS (klass);

  object_class->dispose = shumate_marker_layer_dispose;
  object_class->finalize = shumate_marker_layer_finalize;
  object_class->get_property = shumate_marker_layer_get_property;
  object_class->set_property = shumate_marker_layer_set_property;
  object_class->constructed = shumate_marker_layer_constructed;

  widget_class->size_allocate = shumate_marker_layer_size_allocate;

  layer_class->get_debug_text = shumate_marker_layer_get_debug_text;

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
  GtkGesture *click_gesture;

  self->mode = GTK_SELECTION_NONE;

  click_gesture = gtk_gesture_click_new ();
  gtk_widget_add_controller (GTK_WIDGET (self), GTK_EVENT_CONTROLLER (click_gesture));
  g_signal_connect_swapped (click_gesture, "released", G_CALLBACK (on_click_gesture_released), self);
}

/**
 * shumate_marker_layer_new:
 * @viewport: the @ShumateViewport
 *
 * Creates a new instance of [class@MarkerLayer].
 *
 * Returns: a new [class@MarkerLayer] ready to be used as a container for the markers.
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
 * Creates a new instance of [class@MarkerLayer] with the specified selection mode.
 *
 * Returns: a new [class@MarkerLayer] ready to be used as a container for the markers.
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
  ShumateView *view = self->view;
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
 * @self: a [class@MarkerLayer]
 * @marker: a [class@Marker]
 *
 * Adds the marker to the layer.
 */
void
shumate_marker_layer_add_marker (ShumateMarkerLayer *self,
    ShumateMarker *marker)
{
  g_return_if_fail (SHUMATE_IS_MARKER_LAYER (self));
  g_return_if_fail (SHUMATE_IS_MARKER (marker));

  g_signal_connect_object (G_OBJECT (marker), "notify::latitude",
      G_CALLBACK (marker_position_notify), self, 0);
  g_signal_connect_object (G_OBJECT (marker), "notify::longitude",
      G_CALLBACK (marker_position_notify), self, 0);
  g_signal_connect_object (G_OBJECT (marker), "notify::x-hotspot",
      G_CALLBACK (marker_position_notify), self, 0);
  g_signal_connect_object (G_OBJECT (marker), "notify::y-hotspot",
      G_CALLBACK (marker_position_notify), self, 0);
  g_signal_connect_object (G_OBJECT (marker), "notify::halign",
      G_CALLBACK (marker_position_notify), self, 0);
  g_signal_connect_object (G_OBJECT (marker), "notify::valign",
      G_CALLBACK (marker_position_notify), self, 0);

  /*g_signal_connect (G_OBJECT (marker), "drag-motion",
      G_CALLBACK (marker_move_by_cb), layer);*/

  shumate_marker_set_selected (marker, FALSE);

  gtk_widget_insert_before (GTK_WIDGET(marker), GTK_WIDGET (self), NULL);
  update_marker_visibility (self, marker);
  self->n_children++;
}


/**
 * shumate_marker_layer_remove_all:
 * @self: a [class@MarkerLayer]
 *
 * Removes all markers from the layer.
 */
void
shumate_marker_layer_remove_all (ShumateMarkerLayer *self)
{
  GtkWidget *child;

  g_return_if_fail (SHUMATE_IS_MARKER_LAYER (self));

  child = gtk_widget_get_first_child (GTK_WIDGET (self));
  while (child)
    {
      GtkWidget *next = gtk_widget_get_next_sibling (child);

      g_signal_handlers_disconnect_by_data (child, self);
      gtk_widget_unparent (child);

      child = next;
    }

  self->n_children = 0;
}


/**
 * shumate_marker_layer_get_markers:
 * @self: a [class@MarkerLayer]
 *
 * Gets a copy of the list of all markers inserted into the layer. You should
 * free the list but not its contents.
 *
 * Returns: (transfer container) (element-type ShumateMarker): the list
 */
GList *
shumate_marker_layer_get_markers (ShumateMarkerLayer *self)
{
  GList *list = NULL;
  GtkWidget *child;

  g_return_val_if_fail (SHUMATE_IS_MARKER_LAYER (self), NULL);

  for (child = gtk_widget_get_last_child (GTK_WIDGET (self));
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
 * @self: a [class@MarkerLayer]
 *
 * Gets a list of selected markers in the layer.
 *
 * Returns: (transfer container) (element-type ShumateMarker): the list
 */
GList *
shumate_marker_layer_get_selected (ShumateMarkerLayer *self)
{
  g_return_val_if_fail (SHUMATE_IS_MARKER_LAYER (self), FALSE);
  return g_list_copy (self->selected);
}

static void
update_debug_text (ShumateMarkerLayer *self)
{
  if (shumate_inspector_settings_get_show_debug_overlay (shumate_inspector_settings_get_default ()))
    gtk_widget_queue_draw (GTK_WIDGET (self));
}

/**
 * shumate_marker_layer_select_marker:
 * @self: a [class@MarkerLayer]
 * @marker: a [class@Marker] that is a child of @self
 *
 * Selects a marker in this layer.
 *
 * If [class@MarkerLayer]:selection-mode is %GTK_SELECTION_SINGLE or
 * %GTK_SELECTION_BROWSE, all other markers will be unselected. If the mode is
 * %GTK_SELECTION_NONE or @marker is not selectable, nothing will happen.
 *
 * Returns: %TRUE if the marker is now selected, otherwise %FALSE
 */
gboolean
shumate_marker_layer_select_marker (ShumateMarkerLayer *self, ShumateMarker *marker)
{
  g_return_val_if_fail (SHUMATE_IS_MARKER_LAYER (self), FALSE);
  g_return_val_if_fail (SHUMATE_IS_MARKER (marker), FALSE);
  g_return_val_if_fail (gtk_widget_get_parent (GTK_WIDGET (marker)) == GTK_WIDGET (self), FALSE);

  if (!shumate_marker_get_selectable (marker)) {
    return FALSE;
  }

  if (shumate_marker_is_selected (marker)) {
    return TRUE;
  }

  switch (self->mode) {
    case GTK_SELECTION_NONE:
      return FALSE;

    case GTK_SELECTION_BROWSE:
    case GTK_SELECTION_SINGLE:
      shumate_marker_layer_unselect_all_markers (self);

      /* fall through */
    case GTK_SELECTION_MULTIPLE:
      self->selected = g_list_prepend (self->selected, marker);
      shumate_marker_set_selected (marker, TRUE);
      update_debug_text (self);
      g_signal_emit (self, signals[MARKER_SELECTED], 0, marker);
      return TRUE;

    default:
      g_assert_not_reached ();
  }
}


/**
 * shumate_marker_layer_unselect_marker:
 * @self: a [class@MarkerLayer]
 * @marker: a [class@Marker] that is a child of @self
 *
 * Unselects a marker in this layer.
 *
 * This works even if [class@MarkerLayer]:selection-mode is
 * %GTK_SELECTION_BROWSE. Browse mode only prevents user interaction, not the
 * program, from unselecting a marker.
 */
void
shumate_marker_layer_unselect_marker (ShumateMarkerLayer *self,
                                      ShumateMarker      *marker)
{
  g_return_if_fail (SHUMATE_IS_MARKER_LAYER (self));
  g_return_if_fail (SHUMATE_IS_MARKER (marker));
  g_return_if_fail (gtk_widget_get_parent (GTK_WIDGET (marker)) == GTK_WIDGET (self));

  if (!shumate_marker_is_selected (marker)) {
    return;
  }

  self->selected = g_list_remove (self->selected, marker);
  shumate_marker_set_selected (marker, FALSE);
  update_debug_text (self);
  g_signal_emit (self, signals[MARKER_UNSELECTED], 0, marker);
}


/**
 * shumate_marker_layer_remove_marker:
 * @self: a [class@MarkerLayer]
 * @marker: a [class@Marker]
 *
 * Removes the marker from the layer.
 */
void
shumate_marker_layer_remove_marker (ShumateMarkerLayer *self,
                                    ShumateMarker      *marker)
{
  g_return_if_fail (SHUMATE_IS_MARKER_LAYER (self));
  g_return_if_fail (SHUMATE_IS_MARKER (marker));
  g_return_if_fail (gtk_widget_get_parent (GTK_WIDGET (marker)) == GTK_WIDGET (self));

  g_signal_handlers_disconnect_by_func (G_OBJECT (marker),
      G_CALLBACK (marker_position_notify), self);

  g_signal_handlers_disconnect_by_func (marker,
      G_CALLBACK (marker_move_by_cb), self);

  if (shumate_marker_is_selected (marker)) {
    shumate_marker_layer_unselect_marker (self, marker);
  }

  gtk_widget_unparent (GTK_WIDGET (marker));
  self->n_children--;
}


/**
 * shumate_marker_layer_unselect_all_markers:
 * @self: a [class@MarkerLayer]
 *
 * Unselects all markers in the layer.
 */
void
shumate_marker_layer_unselect_all_markers (ShumateMarkerLayer *self)
{
  g_autoptr(GList) prev_selected = NULL;

  g_return_if_fail (SHUMATE_IS_MARKER_LAYER (self));

  prev_selected = g_list_copy (self->selected);

  for (GList *l = prev_selected; l != NULL; l = l->next) {
    shumate_marker_layer_unselect_marker (self, SHUMATE_MARKER (l->data));
  }
}


/**
 * shumate_marker_layer_select_all_markers:
 * @self: a [class@MarkerLayer]
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
 * @self: a [class@MarkerLayer]
 * @mode: a [enum@Gtk.SelectionMode] value
 *
 * Sets the selection mode of the layer.
 *
 * NOTE: changing selection mode to %GTK_SELECTION_NONE, %GTK_SELECTION_SINGLE
 * or %GTK_SELECTION_BROWSE will clear all previously selected markers.
 */
void
shumate_marker_layer_set_selection_mode (ShumateMarkerLayer *self,
                                         GtkSelectionMode    mode)
{
  g_return_if_fail (SHUMATE_IS_MARKER_LAYER (self));

  if (self->mode == mode)
    return;

  self->mode = mode;

  if (mode != GTK_SELECTION_MULTIPLE)
    shumate_marker_layer_unselect_all_markers (self);

  g_object_notify_by_pspec (G_OBJECT (self), obj_properties[PROP_SELECTION_MODE]);
}


/**
 * shumate_marker_layer_get_selection_mode:
 * @self: a [class@MarkerLayer]
 *
 * Gets the selection mode of the layer.
 *
 * Returns: the selection mode of the layer.
 */
GtkSelectionMode
shumate_marker_layer_get_selection_mode (ShumateMarkerLayer *self)
{
  g_return_val_if_fail (SHUMATE_IS_MARKER_LAYER (self), GTK_SELECTION_NONE);

  return self->mode;
}
