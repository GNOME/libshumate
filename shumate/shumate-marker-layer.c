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

#include "shumate-cairo-exportable.h"
#include "shumate-defines.h"
#include "shumate-enum-types.h"
#include "shumate-private.h"
#include "shumate-view.h"

#include <cairo/cairo-gobject.h>
#include <glib.h>

static void cairo_exportable_interface_init (ShumateCairoExportableInterface *iface);

#define GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), SHUMATE_TYPE_MARKER_LAYER, ShumateMarkerLayerPrivate))

enum
{
  /* normal signals */
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_SELECTION_MODE,
  PROP_SURFACE,
};


typedef struct
{
  ShumateSelectionMode mode;
  ShumateView *view;
} ShumateMarkerLayerPrivate;

G_DEFINE_TYPE_WITH_CODE (ShumateMarkerLayer, shumate_marker_layer, SHUMATE_TYPE_LAYER,
    G_ADD_PRIVATE (ShumateMarkerLayer)
    G_IMPLEMENT_INTERFACE (SHUMATE_TYPE_CAIRO_EXPORTABLE, cairo_exportable_interface_init));

static cairo_surface_t *get_surface (ShumateCairoExportable *exportable);

static void marker_selected_cb (ShumateMarker *marker,
    G_GNUC_UNUSED GParamSpec *arg1,
    ShumateMarkerLayer *layer);

static void set_view (ShumateLayer *layer,
    ShumateView *view);

static ShumateBoundingBox *get_bounding_box (ShumateLayer *layer);


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

    case PROP_SURFACE:
      g_value_set_boxed (value, get_surface (SHUMATE_CAIRO_EXPORTABLE (self)));
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
  g_object_class_install_property (object_class,
      PROP_SELECTION_MODE,
      g_param_spec_enum ("selection-mode",
          "Selection Mode",
          "Determines the type of selection that will be performed.",
          SHUMATE_TYPE_SELECTION_MODE,
          SHUMATE_SELECTION_NONE,
          SHUMATE_PARAM_READWRITE));

  /**
   * ShumateMarkerLayer:surface:
   *
   * The Cairo surface representation of the marker layer
   */
  g_object_class_install_property (object_class,
      PROP_SURFACE,
      g_param_spec_boxed ("surface",
          "Surface",
          "Cairo surface representaion",
          CAIRO_GOBJECT_TYPE_SURFACE,
          G_PARAM_READABLE));
}


static void
shumate_marker_layer_init (ShumateMarkerLayer *self)
{
  ShumateMarkerLayerPrivate *priv = shumate_marker_layer_get_instance_private (self);

  priv->mode = SHUMATE_SELECTION_NONE;
  priv->view = NULL;
}

static cairo_surface_t *
get_surface (ShumateCairoExportable *exportable)
{
  g_return_val_if_fail (SHUMATE_IS_MARKER_LAYER (exportable), NULL);

  //ClutterActorIter iter;
  //ClutterActor *child;
  ShumateMarkerLayer *layer = SHUMATE_MARKER_LAYER (exportable);
  ShumateMarkerLayerPrivate *priv = shumate_marker_layer_get_instance_private (layer);
  cairo_surface_t *surface = NULL;
  cairo_t *cr;
  gboolean has_marker = FALSE;

  /*
  clutter_actor_iter_init (&iter, CLUTTER_ACTOR (layer));
  while (clutter_actor_iter_next (&iter, &child))
    {
      ShumateMarker *marker = SHUMATE_MARKER (child);

      if (SHUMATE_IS_CAIRO_EXPORTABLE (marker))
        {
          gfloat x, y, tx, ty;
          gint origin_x, origin_y;
          cairo_surface_t *marker_surface;
          ShumateCairoExportable *exportable;

          if (!has_marker)
            {
              gfloat width, height;

              width = 256;
              height = 256;
              if (priv->view != NULL)
                clutter_actor_get_size (CLUTTER_ACTOR (priv->view),&width, &height);
              surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
              has_marker = TRUE;
            }

          exportable = SHUMATE_CAIRO_EXPORTABLE (marker);
          marker_surface = shumate_cairo_exportable_get_surface (exportable);

          shumate_view_get_viewport_origin (priv->view, &origin_x, &origin_y);
          clutter_actor_get_translation (CLUTTER_ACTOR (marker), &tx, &ty, NULL);
          clutter_actor_get_position (CLUTTER_ACTOR (marker), &x, &y);

          cr = cairo_create (surface);
          cairo_set_source_surface (cr, marker_surface,
                                    (x + tx) - origin_x,
                                    (y + ty) - origin_y);
          cairo_paint (cr);
          cairo_destroy (cr);
        }
    }
   */

  return surface;
}

static void
cairo_exportable_interface_init (ShumateCairoExportableInterface *iface)
{
  iface->get_surface = get_surface;
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
shumate_marker_layer_new_full (ShumateSelectionMode mode)
{
  return g_object_new (SHUMATE_TYPE_MARKER_LAYER, "selection-mode", mode, NULL);
}

static void
set_selected_all_but_one (ShumateMarkerLayer *layer,
    ShumateMarker *not_selected,
    gboolean select)
{
  /*
  ClutterActorIter iter;
  ClutterActor *child;

  clutter_actor_iter_init (&iter, CLUTTER_ACTOR (layer));
  while (clutter_actor_iter_next (&iter, &child))
    {
      ShumateMarker *marker = SHUMATE_MARKER (child);

      if (marker != not_selected)
        {
          g_signal_handlers_block_by_func (marker,
              G_CALLBACK (marker_selected_cb),
              layer);

          shumate_marker_set_selected (marker, select);
          shumate_marker_set_selectable (marker, layer->priv->mode != SHUMATE_SELECTION_NONE);

          g_signal_handlers_unblock_by_func (marker,
              G_CALLBACK (marker_selected_cb),
              layer);
        }
    }
   */
}


static void
marker_selected_cb (ShumateMarker *marker,
    G_GNUC_UNUSED GParamSpec *arg1,
    ShumateMarkerLayer *layer)
{
  ShumateMarkerLayerPrivate *priv = shumate_marker_layer_get_instance_private (layer);

  if (priv->mode == SHUMATE_SELECTION_SINGLE && shumate_marker_get_selected (marker))
    set_selected_all_but_one (layer, marker, FALSE);
}


static void
set_marker_position (ShumateMarkerLayer *layer, ShumateMarker *marker)
{
  ShumateMarkerLayerPrivate *priv = shumate_marker_layer_get_instance_private (layer);
  gint x, y, origin_x, origin_y;

  /* layer not yet added to the view */
  if (priv->view == NULL)
    return;

  shumate_view_get_viewport_origin (priv->view, &origin_x, &origin_y);
  x = shumate_view_longitude_to_x (priv->view,
        shumate_location_get_longitude (SHUMATE_LOCATION (marker))) + origin_x;
  y = shumate_view_latitude_to_y (priv->view,
        shumate_location_get_latitude (SHUMATE_LOCATION (marker))) + origin_y;

  //clutter_actor_set_position (CLUTTER_ACTOR (marker), x, y);
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

  shumate_marker_set_selectable (marker, priv->mode != SHUMATE_SELECTION_NONE);

  g_signal_connect (G_OBJECT (marker), "notify::selected",
      G_CALLBACK (marker_selected_cb), layer);

  g_signal_connect (G_OBJECT (marker), "notify::latitude",
      G_CALLBACK (marker_position_notify), layer);

  g_signal_connect (G_OBJECT (marker), "drag-motion",
      G_CALLBACK (marker_move_by_cb), layer);

  //clutter_actor_add_child (CLUTTER_ACTOR (layer), CLUTTER_ACTOR (marker));
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
          G_CALLBACK (marker_selected_cb), layer);

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
  GList *lst;

  //lst = clutter_actor_get_children (CLUTTER_ACTOR (layer));
  return g_list_reverse (lst);
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

  g_return_val_if_fail (SHUMATE_IS_MARKER_LAYER (layer), NULL);

  /*
  ClutterActorIter iter;
  ClutterActor *child;

  clutter_actor_iter_init (&iter, CLUTTER_ACTOR (layer));
  while (clutter_actor_iter_next (&iter, &child))
    {
      ShumateMarker *marker = SHUMATE_MARKER (child);

      if (shumate_marker_get_selected (marker))
        selected = g_list_prepend (selected, marker);
    }
   */

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

  g_signal_handlers_disconnect_by_func (G_OBJECT (marker),
      G_CALLBACK (marker_selected_cb), layer);

  g_signal_handlers_disconnect_by_func (G_OBJECT (marker),
      G_CALLBACK (marker_position_notify), layer);

  g_signal_handlers_disconnect_by_func (marker,
      G_CALLBACK (marker_move_by_cb), layer);

  //clutter_actor_remove_child (CLUTTER_ACTOR (layer), CLUTTER_ACTOR (marker));
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
 * @mode: a #ShumateSelectionMode value
 *
 * Sets the selection mode of the layer.
 *
 * NOTE: changing selection mode to SHUMATE_SELECTION_NONE or
 * SHUMATE_SELECTION_SINGLE will clear all previously selected markers.
 */
void
shumate_marker_layer_set_selection_mode (ShumateMarkerLayer *layer,
    ShumateSelectionMode mode)
{
  ShumateMarkerLayerPrivate *priv = shumate_marker_layer_get_instance_private (layer);

  g_return_if_fail (SHUMATE_IS_MARKER_LAYER (layer));

  if (priv->mode == mode)
    return;
  priv->mode = mode;

  if (mode != SHUMATE_SELECTION_MULTIPLE)
    set_selected_all_but_one (layer, NULL, FALSE);

  g_object_notify (G_OBJECT (layer), "selection-mode");
}


/**
 * shumate_marker_layer_get_selection_mode:
 * @layer: a #ShumateMarkerLayer
 *
 * Gets the selection mode of the layer.
 *
 * Returns: the selection mode of the layer.
 */
ShumateSelectionMode
shumate_marker_layer_get_selection_mode (ShumateMarkerLayer *layer)
{
  ShumateMarkerLayerPrivate *priv = shumate_marker_layer_get_instance_private (layer);

  g_return_val_if_fail (SHUMATE_IS_MARKER_LAYER (layer), SHUMATE_SELECTION_SINGLE);

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
