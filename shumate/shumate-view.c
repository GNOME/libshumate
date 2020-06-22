/*
 * Copyright (C) 2008-2009 Pierre-Luc Beaudoin <pierre-luc@pierlux.com>
 * Copyright (C) 2010-2013 Jiri Techet <techet@gmail.com>
 * Copyright (C) 2012,2020 Collabora Ltd. (https://www.collabora.com/)
 * Copyright (C) 2019 Marcus Lundblad <ml@update.uu.se>
 * Copyright (C) 2020 Corentin Noël <corentin.noel@collabora.com>
 *
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
 * SECTION:shumate-view
 * @short_description: A #ClutterActor to display maps
 *
 * The #ShumateView is a ClutterActor to display maps.  It supports two modes
 * of scrolling:
 * <itemizedlist>
 *   <listitem><para>Push: the normal behavior where the maps don't move
 *   after the user stopped scrolling;</para></listitem>
 *   <listitem><para>Kinetic: the iPhone-like behavior where the maps
 *   decelerate after the user stopped scrolling.</para></listitem>
 * </itemizedlist>
 *
 * You can use the same #ShumateView to display many types of maps.  In
 * Shumate they are called map sources.  You can change the #map-source
 * property at anytime to replace the current displayed map.
 *
 * The maps are downloaded from Internet from open maps sources (like
 * <ulink role="online-location"
 * url="http://www.openstreetmap.org">OpenStreetMap</ulink>).  Maps are divided
 * in tiles for each zoom level.  When a tile is requested, #ShumateView will
 * first check if it is in cache (in the user's cache dir under shumate). If
 * an error occurs during download, an error tile will be displayed.
 *
 * The button-press-event and button-release-event signals are emitted each
 * time a mouse button is pressed and released on the @view.
 */

#include "config.h"

#include "shumate-view.h"

#define DEBUG_FLAG SHUMATE_DEBUG_VIEW
#include "shumate-debug.h"

#include "shumate.h"
#include "shumate-defines.h"
#include "shumate-enum-types.h"
#include "shumate-marshal.h"
#include "shumate-map-layer.h"
#include "shumate-map-source.h"
#include "shumate-map-source-factory.h"
#include "shumate-tile.h"
#include "shumate-license.h"
#include "shumate-viewport.h"

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <math.h>

enum
{
  /* normal signals */
  ANIMATION_COMPLETED,
  LAYER_RELOCATED,
  LAST_SIGNAL
};

enum
{
  PROP_DECELERATION = 1,
  PROP_KINETIC_MODE,
  PROP_ZOOM_ON_DOUBLE_CLICK,
  PROP_ANIMATE_ZOOM,
  PROP_STATE,
  PROP_GOTO_ANIMATION_DURATION,
  N_PROPERTIES
};

static guint signals[LAST_SIGNAL] = { 0, };

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

#define ZOOM_LEVEL_OUT_OF_RANGE(priv, level) \
  (level < shumate_viewport_get_min_zoom_level (priv->viewport) || \
           level > shumate_viewport_get_max_zoom_level (priv->viewport) || \
   level < shumate_map_source_get_min_zoom_level (priv->map_source) || \
           level > shumate_map_source_get_max_zoom_level (priv->map_source))

/* Between state values for go_to */
typedef struct
{
  ShumateView *view;
  //ClutterTimeline *timeline;
  gdouble to_latitude;
  gdouble to_longitude;
  gdouble from_latitude;
  gdouble from_longitude;
} GoToContext;


typedef struct
{
  ShumateView *view;
  ShumateMapSource *map_source;
  gint x;
  gint y;
  gint zoom_level;
  gint size;
} FillTileCallbackData;


typedef struct
{
  ShumateViewport *viewport;

  /* There are num_right_clones clones on the right, and one extra on the left */
  gint num_right_clones;
  GList *map_clones;
  /* There are num_right_clones + 2 user layer slots, overlayed on the map clones.
   * Initially, the first slot contains the left clone, the second slot
   * contains the real user layer, and the rest contain the right clones.
   * Whenever the cursor enters a clone slot, its content
   * is swapped with the real one so as to ensure reactiveness to events.
   */
  GList *user_layer_slots;

  gdouble viewport_x;
  gdouble viewport_y;
  gint viewport_width;
  gint viewport_height;

  GList *overlay_sources;

  gboolean location_updated;

  gint bg_offset_x;
  gint bg_offset_y;

  gboolean zoom_on_double_click;
  gboolean animate_zoom;

  gboolean kinetic_mode;

  ShumateState state; /* View's global state */

  // shumate_view_go_to's context, kept for stop_go_to
  GoToContext *goto_context;

  gint tiles_loading;

  guint redraw_timeout;
  guint zoom_timeout;

  guint goto_duration;

  gboolean animating_zoom;
  guint anim_start_zoom_level;
  gdouble zoom_actor_viewport_x;
  gdouble zoom_actor_viewport_y;
  guint zoom_actor_timeout;

  /* Zoom gesture */
  guint initial_gesture_zoom;
  gdouble focus_lat;
  gdouble focus_lon;
  gboolean zoom_started;
  gdouble accumulated_scroll_dy;
  gdouble drag_begin_lat;
  gdouble drag_begin_lon;
} ShumateViewPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (ShumateView, shumate_view, GTK_TYPE_WIDGET);

static void shumate_view_get_property (GObject *object,
    guint prop_id,
    GValue *value,
    GParamSpec *pspec);
static void shumate_view_set_property (GObject *object,
    guint prop_id,
    const GValue *value,
    GParamSpec *pspec);
static void shumate_view_dispose (GObject *object);
static void shumate_view_go_to_with_duration (ShumateView *view,
    gdouble latitude,
    gdouble longitude,
    guint duration);

/* static void */
/* view_relocated_cb (G_GNUC_UNUSED ShumateViewport *viewport, */
/*     ShumateView *view) */
/* { */
/*   ShumateViewPrivate *priv = view->priv; */
/*   gint anchor_x, anchor_y, new_width, new_height; */
/*   gint tile_size, column_count, row_count; */

/*   clutter_actor_destroy_all_children (priv->zoom_layer); */
/*   g_signal_emit_by_name (view, "layer-relocated", NULL); */

  /* Clutter clones need their source actor to have an explicitly set size to display properly */
/*   tile_size = shumate_map_source_get_tile_size (priv->map_source); */
/*   column_count = shumate_map_source_get_column_count (priv->map_source, priv->zoom_level); */
/*   row_count = shumate_map_source_get_row_count (priv->map_source, priv->zoom_level); */
/*   shumate_viewport_get_anchor (SHUMATE_VIEWPORT (priv->viewport), &anchor_x, &anchor_y); */

  /* The area containing tiles in the map layer is actually column_count * tile_size wide (same */
/*    * for height), but the viewport anchor acts as an offset for the tile actors, causing the map */
/*    * layer to contain some empty space as well. */
/*    */
/*   new_width = column_count * tile_size + anchor_x; */
/*   new_height = row_count * tile_size + anchor_y; */

/*   clutter_actor_set_size (priv->map_layer, new_width, new_height); */
/* } */

/*
static void
panning_completed (G_GNUC_UNUSED ShumateKineticScrollView *scroll,
    ShumateView *view)
{
  ShumateViewPrivate *priv = view->priv;
  gdouble x, y;

  if (priv->redraw_timeout != 0)
    {
      g_source_remove (priv->redraw_timeout);
      priv->redraw_timeout = 0;
    }

  shumate_viewport_get_origin (SHUMATE_VIEWPORT (priv->viewport), &x, &y);

  update_coords (view, x, y, TRUE);
}
*/

/*static gboolean
zoom_timeout_cb (gpointer data)
{
  ShumateView *view = data;
  ShumateViewPrivate *priv = shumate_view_get_instance_private (view);

  priv->accumulated_scroll_dy = 0;
  priv->zoom_timeout = 0;

  return FALSE;
}*/


/* static gboolean
scroll_event (G_GNUC_UNUSED ShumateView *this,
    GdkEvent *event,
    ShumateView *view)
{
  GdkScrollDirection direction;
  gdk_event_get_scroll_direction(event, &direction);
  gdouble x, y;
  gdk_event_get_coords(event, &x, &y);

  ShumateViewPrivate *priv = shumate_view_get_instance_private (view);

  guint zoom_level = priv->zoom_level;

  if (direction == GDK_SCROLL_UP)
    zoom_level = priv->zoom_level + 1;
  else if (direction == GDK_SCROLL_DOWN)
    zoom_level = priv->zoom_level - 1;
  else if (direction == GDK_SCROLL_SMOOTH)
    {
      gdouble dx, dy;
      gint steps;

      gdk_event_get_scroll_deltas (event, &dx, &dy);

      priv->accumulated_scroll_dy += dy;
      // add some small value to avoid missing step for values like 0.999999
      if (dy > 0)
        steps = (int) (priv->accumulated_scroll_dy + 0.01);
      else
        steps = (int) (priv->accumulated_scroll_dy - 0.01);
      zoom_level = priv->zoom_level - steps;
      priv->accumulated_scroll_dy -= steps;

      if (priv->zoom_timeout != 0)
        g_source_remove (priv->zoom_timeout);
      priv->zoom_timeout = g_timeout_add (1000, zoom_timeout_cb, view);
    }

  return view_set_zoom_level_at (view, zoom_level, TRUE, x, y);
}
*/

static void
on_drag_gesture_drag_begin (ShumateView    *self,
                            gdouble         start_x,
                            gdouble         start_y,
                            GtkGestureDrag *gesture)
{
  ShumateViewPrivate *priv = shumate_view_get_instance_private (self);

  g_assert (SHUMATE_IS_VIEW (self));

  priv->drag_begin_lon = shumate_location_get_longitude (SHUMATE_LOCATION (priv->viewport));
  priv->drag_begin_lat = shumate_location_get_latitude (SHUMATE_LOCATION (priv->viewport));

  gtk_widget_set_cursor_from_name (GTK_WIDGET (self), "grabbing");
}

static void
on_drag_gesture_drag_update (ShumateView    *self,
                             gdouble         offset_x,
                             gdouble         offset_y,
                             GtkGestureDrag *gesture)
{
  ShumateViewPrivate *priv = shumate_view_get_instance_private (self);
  ShumateMapSource *map_source;
  double x, y;
  double lat, lon;
  guint zoom_level;
  guint tile_size, max_x, max_y;

  g_assert (SHUMATE_IS_VIEW (self));

  map_source = shumate_viewport_get_reference_map_source (priv->viewport);
  if (!map_source)
    return;

  zoom_level = shumate_viewport_get_zoom_level (priv->viewport);
  x = shumate_map_source_get_x (map_source, zoom_level, priv->drag_begin_lon) - offset_x;
  y = shumate_map_source_get_y (map_source, zoom_level, priv->drag_begin_lat) - offset_y;

  tile_size = shumate_map_source_get_tile_size (map_source);
  max_x = shumate_map_source_get_column_count (map_source, zoom_level) * tile_size;
  max_y = shumate_map_source_get_row_count (map_source, zoom_level) * tile_size;

  x = fmod (x, max_x);
  if (x < 0)
    x += max_x;

  y = fmod (y, max_y);
  if (y < 0)
    y += max_y;

  lat = shumate_map_source_get_latitude (map_source, zoom_level, y);
  lon = shumate_map_source_get_longitude (map_source, zoom_level, x);

  shumate_location_set_location (SHUMATE_LOCATION (priv->viewport), lat, lon);
}

static void
on_drag_gesture_drag_end (ShumateView    *self,
                          gdouble         offset_x,
                          gdouble         offset_y,
                          GtkGestureDrag *gesture)
{
  ShumateViewPrivate *priv = shumate_view_get_instance_private (self);
  ShumateMapSource *map_source;
  double x, y;
  double lat, lon;
  guint zoom_level;
  guint tile_size, max_x, max_y;

  g_assert (SHUMATE_IS_VIEW (self));

  gtk_widget_set_cursor_from_name (GTK_WIDGET (self), "grab");

  map_source = shumate_viewport_get_reference_map_source (priv->viewport);
  if (!map_source)
    return;

  zoom_level = shumate_viewport_get_zoom_level (priv->viewport);
  x = shumate_map_source_get_x (map_source, zoom_level, priv->drag_begin_lon) - offset_x;
  y = shumate_map_source_get_y (map_source, zoom_level, priv->drag_begin_lat) - offset_y;

  tile_size = shumate_map_source_get_tile_size (map_source);
  max_x = shumate_map_source_get_column_count (map_source, zoom_level) * tile_size;
  max_y = shumate_map_source_get_row_count (map_source, zoom_level) * tile_size;

  x = fmod (x, max_x);
  if (x < 0)
    x += max_x;

  y = fmod (y, max_y);
  if (y < 0)
    y += max_y;

  lat = shumate_map_source_get_latitude (map_source, zoom_level, y);
  lon = shumate_map_source_get_longitude (map_source, zoom_level, x);

  shumate_location_set_location (SHUMATE_LOCATION (priv->viewport), lat, lon);
  priv->drag_begin_lon = 0;
  priv->drag_begin_lat = 0;
}

static gboolean
on_scroll_controller_scroll (ShumateView              *self,
                             gdouble                   dx,
                             gdouble                   dy,
                             GtkEventControllerScroll *controller)
{
  ShumateViewPrivate *priv = shumate_view_get_instance_private (self);

  if (dy > 0)
    shumate_viewport_zoom_in (priv->viewport);
  else
    shumate_viewport_zoom_out (priv->viewport);

  return TRUE;
}

static void
shumate_view_get_property (GObject *object,
    guint prop_id,
    GValue *value,
    GParamSpec *pspec)
{
  ShumateView *view = SHUMATE_VIEW (object);
  ShumateViewPrivate *priv = shumate_view_get_instance_private (view);

  switch (prop_id)
    {
    case PROP_KINETIC_MODE:
      g_value_set_boolean (value, priv->kinetic_mode);
      break;

    case PROP_DECELERATION:
      {
        gdouble decel = 0.0;
        //g_object_get (priv->kinetic_scroll, "deceleration", &decel, NULL);
        g_value_set_double (value, decel);
        break;
      }

    case PROP_ZOOM_ON_DOUBLE_CLICK:
      g_value_set_boolean (value, priv->zoom_on_double_click);
      break;

    case PROP_ANIMATE_ZOOM:
      g_value_set_boolean (value, priv->animate_zoom);
      break;

    case PROP_STATE:
      g_value_set_enum (value, priv->state);
      break;

    case PROP_GOTO_ANIMATION_DURATION:
      g_value_set_uint (value, priv->goto_duration);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}


static void
shumate_view_set_property (GObject *object,
    guint prop_id,
    const GValue *value,
    GParamSpec *pspec)
{
  ShumateView *view = SHUMATE_VIEW (object);
  ShumateViewPrivate *priv = shumate_view_get_instance_private (view);

  switch (prop_id)
    {
    case PROP_KINETIC_MODE:
      shumate_view_set_kinetic_mode (view, g_value_get_boolean (value));
      break;

    case PROP_DECELERATION:
      shumate_view_set_deceleration (view, g_value_get_double (value));
      break;

    case PROP_ZOOM_ON_DOUBLE_CLICK:
      shumate_view_set_zoom_on_double_click (view, g_value_get_boolean (value));
      break;

    case PROP_ANIMATE_ZOOM:
      shumate_view_set_animate_zoom (view, g_value_get_boolean (value));
      break;

    case PROP_GOTO_ANIMATION_DURATION:
      priv->goto_duration = g_value_get_uint (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}


static void
shumate_view_dispose (GObject *object)
{
  ShumateView *view = SHUMATE_VIEW (object);
  ShumateViewPrivate *priv = shumate_view_get_instance_private (view);
  GtkWidget *child;

  if (priv->goto_context != NULL)
    shumate_view_stop_go_to (view);

  while ((child = gtk_widget_get_first_child (GTK_WIDGET (object))))
    gtk_widget_unparent (child);

  g_clear_object (&priv->viewport);

  g_list_free_full (priv->overlay_sources, g_object_unref);
  priv->overlay_sources = NULL;

  //g_clear_object (&priv->background_content);
  g_clear_handle_id (&priv->redraw_timeout, g_source_remove);
  //g_clear_handle_id (&priv->zoom_actor_timeout, g_source_remove);
  g_clear_handle_id (&priv->zoom_timeout, g_source_remove);

  /* if (priv->zoom_gesture)
   *   {
   *     clutter_actor_remove_action (CLUTTER_ACTOR (view),
   *                                  CLUTTER_ACTION (priv->zoom_gesture));
   *     priv->zoom_gesture = NULL;
   *   }
   */

  //priv->map_layer = NULL;
  //priv->license_actor = NULL;

  /* This is needed to prevent race condition see bug #760012 */
  //if (priv->user_layers)
  //    clutter_actor_remove_all_children (priv->user_layers);
  //priv->user_layers = NULL;
  //priv->zoom_layer = NULL;

  G_OBJECT_CLASS (shumate_view_parent_class)->dispose (object);
}


static void
shumate_view_finalize (GObject *object)
{
/*  ShumateViewPrivate *priv = SHUMATE_VIEW (object)->priv; */

  G_OBJECT_CLASS (shumate_view_parent_class)->finalize (object);
}

static void
shumate_view_class_init (ShumateViewClass *shumateViewClass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (shumateViewClass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (shumateViewClass);
  object_class->dispose = shumate_view_dispose;
  object_class->finalize = shumate_view_finalize;
  object_class->get_property = shumate_view_get_property;
  object_class->set_property = shumate_view_set_property;

  /**
   * ShumateView:kinetic-mode:
   *
   * Determines whether the view should use kinetic mode.
   */
  obj_properties[PROP_KINETIC_MODE] =
    g_param_spec_boolean ("kinetic-mode",
                          "Kinetic Mode",
                          "Determines whether the view should use kinetic mode.",
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * ShumateView:deceleration:
   *
   * The deceleration rate for the kinetic mode. The default value is 1.1.
   */
  obj_properties[PROP_DECELERATION] =
    g_param_spec_double ("deceleration",
                         "Deceleration rate",
                         "Rate at which the view will decelerate in kinetic mode.",
                         1.0001, 2.0, 1.1,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * ShumateView:zoom-on-double-click:
   *
   * Should the view zoom in and recenter when the user double click on the map.
   */
  obj_properties[PROP_ZOOM_ON_DOUBLE_CLICK] =
    g_param_spec_boolean ("zoom-on-double-click",
                          "Zoom in on double click",
                          "Zoom in and recenter on double click on the map",
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * ShumateView:animate-zoom:
   *
   * Animate zoom change when zooming in/out.
   */
  obj_properties[PROP_ANIMATE_ZOOM] =
    g_param_spec_boolean ("animate-zoom",
                          "Animate zoom level change",
                          "Animate zoom change when zooming in/out",
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * ShumateView:state:
   *
   * The view's global state. Useful to inform using if the view is busy loading
   * tiles or not.
   */
  obj_properties[PROP_STATE] =
    g_param_spec_enum ("state",
                       "View's state",
                       "View's global state",
                       SHUMATE_TYPE_STATE,
                       SHUMATE_STATE_NONE,
                       G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  /**
   * ShumateView:background-pattern:
   *
   * The pattern displayed in the background of the map.
   */
  /*
  g_object_class_install_property (object_class,
      PROP_BACKGROUND_PATTERN,
      g_param_spec_object ("background-pattern",
          "Background pattern",
          "The tile's background pattern",
          CLUTTER_TYPE_ACTOR,
          G_PARAM_READWRITE));
   */

  /**
   * ShumateView:goto-animation-mode:
   *
   * The mode of animation when going to a location.
   *
   * Please note that animation of #shumate_view_ensure_visible also
   * involves a 'goto' animation.
   *
   */
  /*
  g_object_class_install_property (object_class,
      PROP_GOTO_ANIMATION_MODE,
      g_param_spec_enum ("goto-animation-mode",
          "Go to animation mode",
          "The mode of animation when going to a location",
          CLUTTER_TYPE_ANIMATION_MODE,
          CLUTTER_EASE_IN_OUT_CIRC,
          G_PARAM_READWRITE));
   */

  /**
   * ShumateView:goto-animation-duration:
   *
   * The duration of an animation when going to a location.
   * A value of 0 means that the duration is calculated automatically for you.
   *
   * Please note that animation of #shumate_view_ensure_visible also
   * involves a 'goto' animation.
   *
   */
  obj_properties[PROP_GOTO_ANIMATION_DURATION] =
    g_param_spec_uint ("goto-animation-duration",
                       "Go to animation duration",
                       "The duration of an animation when going to a location",
                       0, G_MAXUINT, 0,
                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class,
                                     N_PROPERTIES,
                                     obj_properties);

  /**
   * ShumateView::animation-completed:
   *
   * The #ShumateView::animation-completed signal is emitted when any animation in the view
   * ends.  This is a detailed signal.  For example, if you want to be signaled
   * only for go-to animation, you should connect to
   * "animation-completed::go-to". And for zoom, connect to "animation-completed::zoom".
   */
  signals[ANIMATION_COMPLETED] =
    g_signal_new ("animation-completed",
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE,
                  0);

  /**
   * ShumateView::layer-relocated:
   *
   * Indicates that the layers have been "relocated". In practice this means that
   * every layer should connect to this signal and redraw itself when the signal is
   * emitted. Layer relocation happens when zooming in/out and when panning for more
   * than MAX_INT pixels.
   */
  signals[LAYER_RELOCATED] =
    g_signal_new ("layer-relocated",
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
  gtk_widget_class_set_css_name (widget_class, g_intern_static_string ("map-view"));
}

static void
shumate_view_init (ShumateView *view)
{
  ShumateViewPrivate *priv = shumate_view_get_instance_private (view);
  GtkGesture *drag_gesture;
  GtkEventController *scroll_controller;

  shumate_debug_set_flags (g_getenv ("SHUMATE_DEBUG"));

  priv->viewport = shumate_viewport_new ();
  priv->zoom_on_double_click = TRUE;
  priv->animate_zoom = TRUE;
  priv->kinetic_mode = FALSE;
  priv->viewport_x = 0;
  priv->viewport_y = 0;
  priv->viewport_width = 0;
  priv->viewport_height = 0;
  priv->state = SHUMATE_STATE_NONE;
  priv->goto_context = NULL;
  priv->tiles_loading = 0;
  priv->animating_zoom = FALSE;
  priv->bg_offset_x = 0;
  priv->bg_offset_y = 0;
  priv->location_updated = FALSE;
  priv->redraw_timeout = 0;
  priv->zoom_actor_timeout = 0;
  priv->goto_duration = 0;
  priv->num_right_clones = 0;
  priv->map_clones = NULL;
  priv->user_layer_slots = NULL;

  gtk_widget_set_cursor_from_name (GTK_WIDGET (view), "grab");

  /* Setup viewport */
  priv->viewport = shumate_viewport_new ();

  /* Setup license */
  drag_gesture = gtk_gesture_drag_new ();
  g_signal_connect_swapped (drag_gesture, "drag-begin", G_CALLBACK (on_drag_gesture_drag_begin), view);
  g_signal_connect_swapped (drag_gesture, "drag-update", G_CALLBACK (on_drag_gesture_drag_update), view);
  g_signal_connect_swapped (drag_gesture, "drag-end", G_CALLBACK (on_drag_gesture_drag_end), view);
  gtk_widget_add_controller (GTK_WIDGET (view), GTK_EVENT_CONTROLLER (drag_gesture));

  scroll_controller = gtk_event_controller_scroll_new (GTK_EVENT_CONTROLLER_SCROLL_VERTICAL|GTK_EVENT_CONTROLLER_SCROLL_DISCRETE);
  g_signal_connect_swapped (scroll_controller, "scroll", G_CALLBACK (on_scroll_controller_scroll), view);
  gtk_widget_add_controller (GTK_WIDGET (view), scroll_controller);
}

/**
 * shumate_view_new:
 *
 * Creates an instance of #ShumateView.
 *
 * Returns: a new #ShumateView ready to be used as a #GtkWidget.
 */
ShumateView *
shumate_view_new (void)
{
  return g_object_new (SHUMATE_TYPE_VIEW, NULL);
}


ShumateView *
shumate_view_new_simple (void)
{
  ShumateView *view = g_object_new (SHUMATE_TYPE_VIEW, NULL);
  ShumateMapSourceFactory *factory;
  ShumateMapSource *source;
  ShumateMapLayer *map_layer;
  ShumateViewport *viewport;
  
  viewport = shumate_view_get_viewport (view);
  factory = shumate_map_source_factory_dup_default ();
  source = shumate_map_source_factory_create_cached_source (factory, SHUMATE_MAP_SOURCE_OSM_MAPNIK);
  shumate_viewport_set_reference_map_source (viewport, source);
  map_layer = shumate_map_layer_new (source, viewport);
  shumate_view_add_layer (view, SHUMATE_LAYER (map_layer));

  return view;
}

/**
 * shumate_view_get_viewport:
 * @self: a #ShumateView
 *
 * Get the #ShumateViewport used by this view.
 * 
 * Returns: (transfer none): the #ShumateViewport
 */
ShumateViewport *
shumate_view_get_viewport (ShumateView *self)
{
  ShumateViewPrivate *priv = shumate_view_get_instance_private (self);

  g_return_val_if_fail (SHUMATE_IS_VIEW (self), NULL);

  return priv->viewport;
}

/**
 * shumate_view_center_on:
 * @view: a #ShumateView
 * @latitude: the longitude to center the map at
 * @longitude: the longitude to center the map at
 *
 * Centers the map on these coordinates.
 */
void
shumate_view_center_on (ShumateView *view,
    gdouble latitude,
    gdouble longitude)
{
  ShumateViewPrivate *priv = shumate_view_get_instance_private (view);

  g_return_if_fail (SHUMATE_IS_VIEW (view));

  shumate_location_set_location (SHUMATE_LOCATION (priv->viewport), latitude, longitude);
}

/**
 * shumate_view_stop_go_to:
 * @view: a #ShumateView
 *
 * Stop the go to animation.  The view will stay where it was when the
 * animation was stopped.
 */
void
shumate_view_stop_go_to (ShumateView *view)
{
  ShumateViewPrivate *priv = shumate_view_get_instance_private (view);

  g_return_if_fail (SHUMATE_IS_VIEW (view));

  if (priv->goto_context == NULL)
    return;

  //clutter_timeline_stop (priv->goto_context->timeline);

  //g_object_unref (priv->goto_context->timeline);

  g_slice_free (GoToContext, priv->goto_context);
  priv->goto_context = NULL;

  g_signal_emit_by_name (view, "animation-completed::go-to", NULL);
}


/**
 * shumate_view_go_to:
 * @view: a #ShumateView
 * @latitude: the longitude to center the map at
 * @longitude: the longitude to center the map at
 *
 * Move from the current position to these coordinates. All tiles in the
 * intermediate view WILL be loaded!
 */
void
shumate_view_go_to (ShumateView *view,
                    gdouble      latitude,
                    gdouble      longitude)
{
  ShumateViewPrivate *priv = shumate_view_get_instance_private (view);
  guint duration;

  g_return_if_fail (SHUMATE_IS_VIEW (view));

  duration = priv->goto_duration;
  if (duration == 0) /* calculate duration from zoom level */
      duration = 500 * shumate_viewport_get_zoom_level (priv->viewport) / 2.0;

  shumate_view_go_to_with_duration (view, latitude, longitude, duration);
}


static void
shumate_view_go_to_with_duration (ShumateView *view,
                                  gdouble      latitude,
                                  gdouble      longitude,
                                  guint        duration) /* In ms */
{
  ShumateViewPrivate *priv = shumate_view_get_instance_private (view);
  GoToContext *ctx;

  g_return_if_fail (SHUMATE_IS_VIEW (view));

  if (duration == 0)
    {
      shumate_view_center_on (view, latitude, longitude);
      return;
    }

  shumate_view_stop_go_to (view);

  ctx = g_slice_new (GoToContext);
  /*ctx->from_latitude = priv->latitude;
  ctx->from_longitude = priv->longitude;
  ctx->to_latitude = CLAMP (latitude, priv->world_bbox->bottom, priv->world_bbox->top);
  ctx->to_longitude = CLAMP (longitude, priv->world_bbox->left, priv->world_bbox->right);*/

  ctx->view = view;

  /* We keep a reference for stop */
  priv->goto_context = ctx;

  /* A ClutterTimeline will be responsible for the animation,
   * at each frame, the current position will be computer and set
   * using shumate_view_center_on.  Timelines skip frames if the
   * computer is not fast enough, so we just need to set the duration.
   *
   * To have a nice animation, the duration should be longer if the zoom level
   * is higher and if the points are far away
   */
  //ctx->timeline = clutter_timeline_new (duration);
  //clutter_timeline_set_progress_mode (ctx->timeline, priv->goto_mode);

  //g_signal_connect (ctx->timeline, "new-frame", G_CALLBACK (timeline_new_frame),
  //    ctx);
  //g_signal_connect (ctx->timeline, "completed", G_CALLBACK (timeline_completed),
  //    view);

  //clutter_timeline_start (ctx->timeline);
}

/**
 * shumate_view_add_layer:
 * @view: a #ShumateView
 * @layer: a #ShumateLayer
 *
 * Adds a new layer to the view
 */
void
shumate_view_add_layer (ShumateView  *view,
                        ShumateLayer *layer)
{
  g_return_if_fail (SHUMATE_IS_VIEW (view));
  g_return_if_fail (SHUMATE_IS_LAYER (layer));

  gtk_widget_insert_before (GTK_WIDGET (layer), GTK_WIDGET (view), NULL);
}


/**
 * shumate_view_remove_layer:
 * @view: a #ShumateView
 * @layer: a #ShumateLayer
 *
 * Removes the given layer from the view
 */
void
shumate_view_remove_layer (ShumateView  *view,
                           ShumateLayer *layer)
{
  g_return_if_fail (SHUMATE_IS_VIEW (view));
  g_return_if_fail (SHUMATE_IS_LAYER (layer));

  if (gtk_widget_get_parent (GTK_WIDGET (layer)) != GTK_WIDGET (view))
    {
      g_critical ("The given ShumateLayer isn't a child of the view");
      return;
    }

  gtk_widget_unparent (GTK_WIDGET (layer));
}

/**
 * shumate_view_set_map_source:
 * @view: a #ShumateView
 * @map_source: a #ShumateMapSource
 *
 * Changes the currently used map source. #g_object_unref() will be called on
 * the previous one.
 *
 * As a side effect, changing the primary map source will also clear all
 * secondary map sources.
 */
void
shumate_view_set_map_source (ShumateView      *view,
                             ShumateMapSource *source)
{
  ShumateViewPrivate *priv = shumate_view_get_instance_private (view);
  ShumateMapSource *ref_map_source;

  g_return_if_fail (SHUMATE_IS_VIEW (view));
  g_return_if_fail (SHUMATE_IS_MAP_SOURCE (source));

  ref_map_source = shumate_viewport_get_reference_map_source (priv->viewport);
  if (ref_map_source == source)
    return;

  shumate_viewport_set_reference_map_source (priv->viewport, source);
}


/**
 * shumate_view_set_deceleration:
 * @view: a #ShumateView
 * @rate: a #gdouble between 1.001 and 2.0
 *
 * The deceleration rate for the kinetic mode.
 */
void
shumate_view_set_deceleration (ShumateView *view,
                               gdouble      rate)
{
  g_return_if_fail (SHUMATE_IS_VIEW (view));
  g_return_if_fail (rate < 2.0 && rate > 1.0001);

  //g_object_set (view->priv->kinetic_scroll, "decel-rate", rate, NULL);
  g_object_notify_by_pspec (G_OBJECT (view), obj_properties[PROP_DECELERATION]);
}


/**
 * shumate_view_set_kinetic_mode:
 * @view: a #ShumateView
 * @kinetic: TRUE for kinetic mode, FALSE for push mode
 *
 * Determines the way the view reacts to scroll events.
 */
void
shumate_view_set_kinetic_mode (ShumateView *view,
                               gboolean     kinetic)
{
  ShumateViewPrivate *priv = shumate_view_get_instance_private (view);

  g_return_if_fail (SHUMATE_IS_VIEW (view));

  priv->kinetic_mode = kinetic;
  //g_object_set (view->priv->kinetic_scroll, "mode", kinetic, NULL);
  g_object_notify_by_pspec (G_OBJECT (view), obj_properties[PROP_KINETIC_MODE]);
}

/**
 * shumate_view_set_zoom_on_double_click:
 * @view: a #ShumateView
 * @value: a #gboolean
 *
 * Should the view zoom in and recenter when the user double click on the map.
 */
void
shumate_view_set_zoom_on_double_click (ShumateView *view,
                                       gboolean     value)
{
  ShumateViewPrivate *priv = shumate_view_get_instance_private (view);

  g_return_if_fail (SHUMATE_IS_VIEW (view));

  priv->zoom_on_double_click = value;
  g_object_notify_by_pspec (G_OBJECT (view), obj_properties[PROP_ZOOM_ON_DOUBLE_CLICK]);
}


/**
 * shumate_view_set_animate_zoom:
 * @view: a #ShumateView
 * @value: a #gboolean
 *
 * Should the view animate zoom level changes.
 */
void
shumate_view_set_animate_zoom (ShumateView *view,
                               gboolean     value)
{
  ShumateViewPrivate *priv = shumate_view_get_instance_private (view);

  g_return_if_fail (SHUMATE_IS_VIEW (view));

  priv->animate_zoom = value;
  g_object_notify_by_pspec (G_OBJECT (view), obj_properties[PROP_ANIMATE_ZOOM]);
}

/**
 * shumate_view_get_deceleration:
 * @view: a #ShumateView
 *
 * Gets the view's deceleration rate.
 *
 * Returns: the view's deceleration rate.
 */
gdouble
shumate_view_get_deceleration (ShumateView *view)
{
  g_return_val_if_fail (SHUMATE_IS_VIEW (view), 0.0);

  gdouble decel = 0.0;
  //g_object_get (view->priv->kinetic_scroll, "decel-rate", &decel, NULL);
  return decel;
}


/**
 * shumate_view_get_kinetic_mode:
 * @view: a #ShumateView
 *
 * Gets the view's scroll mode behaviour.
 *
 * Returns: TRUE for kinetic mode, FALSE for push mode.
 */
gboolean
shumate_view_get_kinetic_mode (ShumateView *view)
{
  ShumateViewPrivate *priv = shumate_view_get_instance_private (view);

  g_return_val_if_fail (SHUMATE_IS_VIEW (view), FALSE);

  return priv->kinetic_mode;
}

/**
 * shumate_view_get_zoom_on_double_click:
 * @view: a #ShumateView
 *
 * Checks whether the view zooms on double click.
 *
 * Returns: TRUE if the view zooms on double click, FALSE otherwise.
 */
gboolean
shumate_view_get_zoom_on_double_click (ShumateView *view)
{
  ShumateViewPrivate *priv = shumate_view_get_instance_private (view);

  g_return_val_if_fail (SHUMATE_IS_VIEW (view), FALSE);

  return priv->zoom_on_double_click;
}


/**
 * shumate_view_get_animate_zoom:
 * @view: a #ShumateView
 *
 * Checks whether the view animates zoom level changes.
 *
 * Returns: TRUE if the view animates zooms, FALSE otherwise.
 */
gboolean
shumate_view_get_animate_zoom (ShumateView *view)
{
  ShumateViewPrivate *priv = shumate_view_get_instance_private (view);

  g_return_val_if_fail (SHUMATE_IS_VIEW (view), FALSE);

  return priv->animate_zoom;
}

/**
 * shumate_view_get_state:
 * @view: a #ShumateView
 *
 * Gets the view's state.
 *
 * Returns: the state.
 */
ShumateState
shumate_view_get_state (ShumateView *view)
{
  ShumateViewPrivate *priv = shumate_view_get_instance_private (view);

  g_return_val_if_fail (SHUMATE_IS_VIEW (view), SHUMATE_STATE_NONE);

  return priv->state;
}

/**
 * shumate_view_add_overlay_source:
 * @view: a #ShumateView
 * @map_source: a #ShumateMapSource
 *
 * Adds a new overlay map source to render tiles on top of the ordinary map
 * source. Multiple overlay sources can be added.
 */
void
shumate_view_add_overlay_source (ShumateView      *view,
                                 ShumateMapSource *map_source)
{
  ShumateViewPrivate *priv = shumate_view_get_instance_private (view);

  g_return_if_fail (SHUMATE_IS_VIEW (view));
  g_return_if_fail (SHUMATE_IS_MAP_SOURCE (map_source));

  priv->overlay_sources = g_list_append (priv->overlay_sources, g_object_ref (map_source));
}


/**
 * shumate_view_remove_overlay_source:
 * @view: a #ShumateView
 * @map_source: a #ShumateMapSource
 *
 * Removes an overlay source from #ShumateView.
 */
void
shumate_view_remove_overlay_source (ShumateView      *view,
                                    ShumateMapSource *map_source)
{
  ShumateViewPrivate *priv = shumate_view_get_instance_private (view);

  g_return_if_fail (SHUMATE_IS_VIEW (view));
  g_return_if_fail (SHUMATE_IS_MAP_SOURCE (map_source));

  priv->overlay_sources = g_list_remove (priv->overlay_sources, map_source);
  g_object_unref (map_source);
}


/**
 * shumate_view_get_overlay_sources:
 * @view: a #ShumateView
 *
 * Gets a list of overlay sources.
 *
 * Returns: (transfer container) (element-type ShumateMapSource): the list
 */
GList *
shumate_view_get_overlay_sources (ShumateView *view)
{
  ShumateViewPrivate *priv = shumate_view_get_instance_private (view);

  g_return_val_if_fail (SHUMATE_IS_VIEW (view), NULL);

  return g_list_copy (priv->overlay_sources);
}
