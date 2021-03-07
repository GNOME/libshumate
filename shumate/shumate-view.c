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
 * @short_description: A #GtkWidget to display maps
 *
 * The #ShumateView is a #GtkWidget to display maps.  It supports two modes
 * of scrolling:
 * <itemizedlist>
 *   <listitem><para>Push: the normal behavior where the maps don't move
 *   after the user stopped scrolling;</para></listitem>
 *   <listitem><para>Kinetic: the behavior where the maps decelerate after
 *   the user stopped scrolling.</para></listitem>
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
 */

#include "config.h"

#include "shumate-view.h"

#define DEBUG_FLAG SHUMATE_DEBUG_VIEW
#include "shumate-debug.h"

#include "shumate.h"
#include "shumate-enum-types.h"
#include "shumate-kinetic-scrolling-private.h"
#include "shumate-marshal.h"
#include "shumate-map-layer.h"
#include "shumate-map-source.h"
#include "shumate-map-source-factory.h"
#include "shumate-tile.h"
#include "shumate-license.h"
#include "shumate-location.h"
#include "shumate-viewport.h"

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <math.h>

#define DECELERATION_FRICTION 4.0

enum
{
  /* normal signals */
  ANIMATION_COMPLETED,
  LAST_SIGNAL
};

enum
{
  PROP_ZOOM_ON_DOUBLE_CLICK = 1,
  PROP_ANIMATE_ZOOM,
  PROP_STATE,
  PROP_GO_TO_DURATION,
  N_PROPERTIES
};

static guint signals[LAST_SIGNAL] = { 0, };

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };
static GQuark go_to_quark;

/* Between state values for go_to */
typedef struct
{
  ShumateView *view;
  int64_t duration_us;
  int64_t start_us;
  double to_latitude;
  double to_longitude;
  double from_latitude;
  double from_longitude;
  guint tick_id;
} GoToContext;


typedef struct
{
  ShumateView *view;
  ShumateMapSource *map_source;
  int x;
  int y;
  int zoom_level;
  int size;
} FillTileCallbackData;

typedef struct
{
  ShumateKineticScrolling *kinetic_scrolling;
  ShumateView *view;
  double start_lat;
  double start_lon;
  int64_t last_deceleration_time_us;
  graphene_vec2_t direction;
} KineticScrollData;

typedef struct
{
  ShumateViewport *viewport;

  /* There are num_right_clones clones on the right, and one extra on the left */
  int num_right_clones;
  GList *map_clones;
  /* There are num_right_clones + 2 user layer slots, overlayed on the map clones.
   * Initially, the first slot contains the left clone, the second slot
   * contains the real user layer, and the rest contain the right clones.
   * Whenever the cursor enters a clone slot, its content
   * is swapped with the real one so as to ensure reactiveness to events.
   */
  GList *user_layer_slots;

  GList *overlay_sources;

  gboolean zoom_on_double_click;
  gboolean animate_zoom;

  ShumateState state; /* View's global state */

  // shumate_view_go_to's context, kept for stop_go_to
  GoToContext *goto_context;

  guint deceleration_tick_id;

  int tiles_loading;

  guint zoom_timeout;

  guint go_to_duration;

  gboolean animating_zoom;
  guint anim_start_zoom_level;
  double zoom_actor_viewport_x;
  double zoom_actor_viewport_y;
  guint zoom_actor_timeout;

  double current_x;
  double current_y;

  /* Zoom gesture */
  guint initial_gesture_zoom;
  double focus_lat;
  double focus_lon;
  gboolean zoom_started;
  double accumulated_scroll_dy;
  double drag_begin_lat;
  double drag_begin_lon;
} ShumateViewPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (ShumateView, shumate_view, GTK_TYPE_WIDGET);

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
  double x, y;
  gdk_event_get_coords(event, &x, &y);

  ShumateViewPrivate *priv = shumate_view_get_instance_private (view);

  guint zoom_level = priv->zoom_level;

  if (direction == GDK_SCROLL_UP)
    zoom_level = priv->zoom_level + 1;
  else if (direction == GDK_SCROLL_DOWN)
    zoom_level = priv->zoom_level - 1;
  else if (direction == GDK_SCROLL_SMOOTH)
    {
      double dx, dy;
      int steps;

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
move_viewport_from_pixel_offset (ShumateView *self,
                                 double       latitude,
                                 double       longitude,
                                 double       offset_x,
                                 double       offset_y)
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
  x = shumate_map_source_get_x (map_source, zoom_level, longitude) - offset_x;
  y = shumate_map_source_get_y (map_source, zoom_level, latitude) - offset_y;

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
cancel_deceleration (ShumateView *self)
{
  ShumateViewPrivate *priv = shumate_view_get_instance_private (self);

  if (priv->deceleration_tick_id > 0)
    {
      gtk_widget_remove_tick_callback (GTK_WIDGET (self), priv->deceleration_tick_id);
      priv->deceleration_tick_id = 0;
    }
}

static gboolean
view_deceleration_tick_cb (GtkWidget     *widget,
                           GdkFrameClock *frame_clock,
                           gpointer       user_data)
{
  KineticScrollData *data = user_data;
  ShumateView *view = data->view;
  int64_t current_time_us;
  double elapsed_us;
  double position;

  g_assert (SHUMATE_IS_VIEW (view));

  current_time_us = gdk_frame_clock_get_frame_time (frame_clock);
  elapsed_us = current_time_us - data->last_deceleration_time_us;

  /* The frame clock can sometimes fire immediately after adding a tick callback,
   * in which case no time has passed, making it impossible to calculate the
   * kinetic factor. If this is the case, wait for the next tick.
   */
  if (G_APPROX_VALUE (elapsed_us, 0.0, FLT_EPSILON))
    return G_SOURCE_CONTINUE;

  data->last_deceleration_time_us = current_time_us;

  if (data->kinetic_scrolling &&
      shumate_kinetic_scrolling_tick (data->kinetic_scrolling, elapsed_us, &position))
    {
      graphene_vec2_t new_positions;

      graphene_vec2_init (&new_positions, position, position);
      graphene_vec2_multiply (&new_positions, &data->direction, &new_positions);

      move_viewport_from_pixel_offset (view,
                                       data->start_lat,
                                       data->start_lon,
                                       graphene_vec2_get_x (&new_positions),
                                       graphene_vec2_get_y (&new_positions));
    }
  else
    {
      g_clear_pointer (&data->kinetic_scrolling, shumate_kinetic_scrolling_free);
    }

  if (!data->kinetic_scrolling)
    {
      cancel_deceleration (view);
      return G_SOURCE_REMOVE;
    }

  return G_SOURCE_CONTINUE;
}


static void
kinetic_scroll_data_free (KineticScrollData *data)
{
  if (data == NULL)
    return;

  g_clear_pointer (&data->kinetic_scrolling, shumate_kinetic_scrolling_free);
  g_free (data);
}

static void
start_deceleration (ShumateView *self,
                    double       h_velocity,
                    double       v_velocity)
{
  ShumateViewPrivate *priv = shumate_view_get_instance_private (self);
  GdkFrameClock *frame_clock;
  KineticScrollData *data;
  graphene_vec2_t velocity;

  g_assert (priv->deceleration_tick_id == 0);

  frame_clock = gtk_widget_get_frame_clock (GTK_WIDGET (self));

  graphene_vec2_init (&velocity, h_velocity, v_velocity);

  data = g_new0 (KineticScrollData, 1);
  data->view = self;
  data->last_deceleration_time_us = gdk_frame_clock_get_frame_time (frame_clock);
  data->start_lat = shumate_location_get_latitude (SHUMATE_LOCATION (priv->viewport));
  data->start_lon = shumate_location_get_longitude (SHUMATE_LOCATION (priv->viewport));
  graphene_vec2_normalize (&velocity, &data->direction);
  data->kinetic_scrolling =
    shumate_kinetic_scrolling_new (DECELERATION_FRICTION,
                                   graphene_vec2_length (&velocity));

  priv->deceleration_tick_id =
    gtk_widget_add_tick_callback (GTK_WIDGET (self),
                                  view_deceleration_tick_cb,
                                  data,
                                  (GDestroyNotify) kinetic_scroll_data_free);
}

static inline double
ease_in_out_quad (double p)
{
  p = 2.0 * p;
  if (p < 1.0)
    return 0.5 * p * p;

  p -= 1.0;
  return -0.5 * (p * (p - 2) - 1);
}

static inline int64_t
ms_to_us (int64_t ms)
{
  return ms * 1000;
}

static gboolean
go_to_tick_cb (GtkWidget     *widget,
               GdkFrameClock *frame_clock,
               gpointer       user_data)
{
  GoToContext *ctx = user_data;
  ShumateViewPrivate *priv = shumate_view_get_instance_private (ctx->view);
  int64_t now_us;
  double latitude, longitude;
  double progress;

  g_assert (SHUMATE_IS_VIEW (ctx->view));
  g_assert (ctx->duration_us >= 0);

  now_us = g_get_monotonic_time ();
  gtk_widget_queue_allocate (widget);

  if (now_us >= ctx->start_us + ctx->duration_us)
    {
      shumate_location_set_location (SHUMATE_LOCATION (priv->viewport),
                                     ctx->to_latitude,
                                     ctx->to_longitude);
      shumate_view_stop_go_to (ctx->view);
      return G_SOURCE_REMOVE;
    }

  progress = (now_us - ctx->start_us) / (double) ctx->duration_us;
  g_assert (progress >= 0.0 && progress <= 1.0);

  /* Apply the ease function to the progress itself */
  progress = ease_in_out_quad (progress);

  /* Since progress already follows the easing curve, a simple LERP is guaranteed
   * to follow it too.
   */
  latitude = ctx->from_latitude + (ctx->to_latitude - ctx->from_latitude) * progress;
  longitude = ctx->from_longitude + (ctx->to_longitude - ctx->from_longitude) * progress;

  shumate_location_set_location (SHUMATE_LOCATION (priv->viewport), latitude, longitude);

  return G_SOURCE_CONTINUE;
}

static void
on_drag_gesture_drag_begin (ShumateView    *self,
                            double         start_x,
                            double         start_y,
                            GtkGestureDrag *gesture)
{
  ShumateViewPrivate *priv = shumate_view_get_instance_private (self);

  g_assert (SHUMATE_IS_VIEW (self));

  cancel_deceleration (self);

  priv->drag_begin_lon = shumate_location_get_longitude (SHUMATE_LOCATION (priv->viewport));
  priv->drag_begin_lat = shumate_location_get_latitude (SHUMATE_LOCATION (priv->viewport));

  gtk_widget_set_cursor_from_name (GTK_WIDGET (self), "grabbing");
}

static void
on_drag_gesture_drag_update (ShumateView    *self,
                             double         offset_x,
                             double         offset_y,
                             GtkGestureDrag *gesture)
{
  ShumateViewPrivate *priv = shumate_view_get_instance_private (self);

  move_viewport_from_pixel_offset (self,
                                   priv->drag_begin_lat,
                                   priv->drag_begin_lon,
                                   offset_x,
                                   offset_y);
}

static void
on_drag_gesture_drag_end (ShumateView    *self,
                          double         offset_x,
                          double         offset_y,
                          GtkGestureDrag *gesture)
{
  ShumateViewPrivate *priv = shumate_view_get_instance_private (self);

  g_assert (SHUMATE_IS_VIEW (self));

  gtk_widget_set_cursor_from_name (GTK_WIDGET (self), "grab");

  move_viewport_from_pixel_offset (self,
                                   priv->drag_begin_lat,
                                   priv->drag_begin_lon,
                                   offset_x,
                                   offset_y);

  priv->drag_begin_lon = 0;
  priv->drag_begin_lat = 0;
}

static void
view_swipe_cb (GtkGestureSwipe *swipe_gesture,
               double           velocity_x,
               double           velocity_y,
               ShumateView     *self)
{
  start_deceleration (self, velocity_x, velocity_y);
}

static gboolean
on_scroll_controller_scroll (ShumateView              *self,
                             double                   dx,
                             double                   dy,
                             GtkEventControllerScroll *controller)
{
  ShumateViewPrivate *priv = shumate_view_get_instance_private (self);
  ShumateMapSource *map_source;
  double scroll_latitude, scroll_longitude;
  double view_lon, view_lat;

  g_object_freeze_notify (G_OBJECT (priv->viewport));
  view_lon = shumate_location_get_longitude (SHUMATE_LOCATION (priv->viewport));
  view_lat = shumate_location_get_latitude (SHUMATE_LOCATION (priv->viewport));

  map_source = shumate_viewport_get_reference_map_source (priv->viewport);
  if (map_source)
    {
      scroll_longitude = shumate_viewport_widget_x_to_longitude (priv->viewport, GTK_WIDGET (self), priv->current_x);
      scroll_latitude = shumate_viewport_widget_y_to_latitude (priv->viewport, GTK_WIDGET (self), priv->current_y);
    }

  if (dy < 0)
    shumate_viewport_zoom_in (priv->viewport);
  else if (dy > 0)
    shumate_viewport_zoom_out (priv->viewport);

  if (map_source)
    {
      double scroll_map_x, scroll_map_y;
      double view_center_x, view_center_y;
      double x_offset, y_offset;
      guint zoom_level;

      scroll_map_x = shumate_viewport_longitude_to_widget_x (priv->viewport, GTK_WIDGET (self), scroll_longitude);
      scroll_map_y = shumate_viewport_latitude_to_widget_y (priv->viewport, GTK_WIDGET (self), scroll_latitude);

      zoom_level = shumate_viewport_get_zoom_level (priv->viewport);
      view_center_x = shumate_map_source_get_x (map_source, zoom_level, view_lon);
      view_center_y = shumate_map_source_get_y (map_source, zoom_level, view_lat);
      x_offset = scroll_map_x - priv->current_x;
      y_offset = scroll_map_y - priv->current_y;
      shumate_location_set_location (SHUMATE_LOCATION (priv->viewport),
                                     shumate_map_source_get_latitude (map_source, zoom_level, view_center_y + y_offset),
                                     shumate_map_source_get_longitude (map_source, zoom_level, view_center_x + x_offset));
    }
  g_object_thaw_notify (G_OBJECT (priv->viewport));

  return TRUE;
}

static void
on_motion_controller_motion (ShumateView              *self,
                             double                   x,
                             double                   y,
                             GtkEventControllerMotion *controller)
{
  ShumateViewPrivate *priv = shumate_view_get_instance_private (self);

  priv->current_x = x;
  priv->current_y = y;
}

static void
shumate_view_go_to_with_duration (ShumateView *view,
                                  double       latitude,
                                  double       longitude,
                                  guint        duration_ms)
{
  ShumateViewPrivate *priv = shumate_view_get_instance_private (view);
  GoToContext *ctx;

  if (duration_ms == 0)
    {
      shumate_view_center_on (view, latitude, longitude);
      return;
    }

  shumate_view_stop_go_to (view);

  ctx = g_new (GoToContext, 1);
  ctx->start_us = g_get_monotonic_time ();
  ctx->duration_us = ms_to_us (duration_ms);
  ctx->from_latitude = shumate_location_get_latitude (SHUMATE_LOCATION (priv->viewport));
  ctx->from_longitude = shumate_location_get_longitude (SHUMATE_LOCATION (priv->viewport));
  ctx->to_latitude = latitude;
  ctx->to_longitude = longitude;
  ctx->view = view;

  /* We keep a reference for stop */
  priv->goto_context = ctx;

  ctx->tick_id = gtk_widget_add_tick_callback (GTK_WIDGET (view), go_to_tick_cb, ctx, NULL);
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
    case PROP_ZOOM_ON_DOUBLE_CLICK:
      g_value_set_boolean (value, priv->zoom_on_double_click);
      break;

    case PROP_ANIMATE_ZOOM:
      g_value_set_boolean (value, priv->animate_zoom);
      break;

    case PROP_STATE:
      g_value_set_enum (value, priv->state);
      break;

    case PROP_GO_TO_DURATION:
      g_value_set_uint (value, priv->go_to_duration);
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

  switch (prop_id)
    {
    case PROP_ZOOM_ON_DOUBLE_CLICK:
      shumate_view_set_zoom_on_double_click (view, g_value_get_boolean (value));
      break;

    case PROP_ANIMATE_ZOOM:
      shumate_view_set_animate_zoom (view, g_value_get_boolean (value));
      break;

    case PROP_GO_TO_DURATION:
      shumate_view_set_go_to_duration (view, g_value_get_uint (value));
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
   * ShumateView:go-to-duration:
   *
   * The duration of an animation when going to a location, in milliseconds.
   * A value of 0 means that the duration is calculated automatically for you.
   *
   * Please note that animation of #shumate_view_ensure_visible also
   * involves a 'go-to' animation.
   *
   */
  obj_properties[PROP_GO_TO_DURATION] =
    g_param_spec_uint ("go-to-duration",
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

  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
  gtk_widget_class_set_css_name (widget_class, g_intern_static_string ("map-view"));

  go_to_quark = g_quark_from_static_string ("go-to");
}

static void
shumate_view_init (ShumateView *view)
{
  ShumateViewPrivate *priv = shumate_view_get_instance_private (view);
  GtkGesture *drag_gesture;
  GtkEventController *scroll_controller;
  GtkEventController *motion_controller;
  GtkGesture *swipe_gesture;

  shumate_debug_set_flags (g_getenv ("SHUMATE_DEBUG"));

  priv->viewport = shumate_viewport_new ();
  priv->zoom_on_double_click = TRUE;
  priv->animate_zoom = TRUE;
  priv->state = SHUMATE_STATE_NONE;
  priv->goto_context = NULL;
  priv->tiles_loading = 0;
  priv->animating_zoom = FALSE;
  priv->zoom_actor_timeout = 0;
  priv->go_to_duration = 0;
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

  swipe_gesture = gtk_gesture_swipe_new ();
  g_signal_connect (swipe_gesture, "swipe", G_CALLBACK (view_swipe_cb), view);
  gtk_widget_add_controller (GTK_WIDGET (view), GTK_EVENT_CONTROLLER (swipe_gesture));

  scroll_controller = gtk_event_controller_scroll_new (GTK_EVENT_CONTROLLER_SCROLL_VERTICAL|GTK_EVENT_CONTROLLER_SCROLL_DISCRETE);
  g_signal_connect_swapped (scroll_controller, "scroll", G_CALLBACK (on_scroll_controller_scroll), view);
  gtk_widget_add_controller (GTK_WIDGET (view), scroll_controller);

  motion_controller = gtk_event_controller_motion_new ();
  g_signal_connect_swapped (motion_controller, "motion", G_CALLBACK (on_motion_controller_motion), view);
  gtk_widget_add_controller (GTK_WIDGET (view), motion_controller);

  gtk_widget_set_overflow (GTK_WIDGET (view), GTK_OVERFLOW_HIDDEN);
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
    double latitude,
    double longitude)
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

  gtk_widget_remove_tick_callback (GTK_WIDGET (view), priv->goto_context->tick_id);
  g_clear_pointer (&priv->goto_context, g_free);

  g_signal_emit (view, signals[ANIMATION_COMPLETED], go_to_quark, NULL);
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
                    double      latitude,
                    double      longitude)
{
  ShumateViewPrivate *priv = shumate_view_get_instance_private (view);
  guint duration;

  g_return_if_fail (SHUMATE_IS_VIEW (view));
  g_return_if_fail (latitude >= SHUMATE_MIN_LATITUDE && latitude <= SHUMATE_MAX_LATITUDE);
  g_return_if_fail (longitude >= SHUMATE_MIN_LONGITUDE && longitude <= SHUMATE_MAX_LONGITUDE);

  duration = priv->go_to_duration;
  if (duration == 0) /* calculate duration from zoom level */
      duration = 500 * shumate_viewport_get_zoom_level (priv->viewport) / 2.0;

  shumate_view_go_to_with_duration (view, latitude, longitude, duration);
}

/**
 * shumate_view_get_go_to_duration:
 * @self: a #ShumateView
 *
 * Get the 'go-to-duration' property.
 *
 * Returns: the animation duration when calling shumate_view_go_to(),
 *   in milliseconds.
 */
guint
shumate_view_get_go_to_duration (ShumateView *self)
{
  ShumateViewPrivate *priv = shumate_view_get_instance_private (self);

  g_return_val_if_fail (SHUMATE_IS_VIEW (self), 0);

  return priv->go_to_duration;
}

/**
 * shumate_view_set_go_to_duration:
 * @self: a #ShumateView
 * @duration: the animation duration, in milliseconds
 *
 * Set the duration of the transition of shumate_view_go_to().
 */
void
shumate_view_set_go_to_duration (ShumateView *self,
                                 guint        duration)
{
  ShumateViewPrivate *priv = shumate_view_get_instance_private (self);

  g_return_if_fail (SHUMATE_IS_VIEW (self));

  if (priv->go_to_duration == duration)
    return;

  priv->go_to_duration = duration;
  g_object_notify_by_pspec (G_OBJECT (self), obj_properties[PROP_GO_TO_DURATION]);
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
 * shumate_view_insert_layer_behind:
 * @view: a #ShumateView
 * @layer: a #ShumateLayer
 * @next_sibling: (nullable): a #ShumateLayer that is a child of @view, or %NULL
 *
 * Adds @layer to @view behind @next_sibling or, if @next_sibling is %NULL, at
 * the top of the layer list.
 */
void
shumate_view_insert_layer_behind (ShumateView  *view,
                                  ShumateLayer *layer,
                                  ShumateLayer *next_sibling)
{
  g_return_if_fail (SHUMATE_IS_VIEW (view));
  g_return_if_fail (SHUMATE_IS_LAYER (layer));
  g_return_if_fail (next_sibling == NULL || SHUMATE_IS_LAYER (next_sibling));
  g_return_if_fail (next_sibling == NULL || gtk_widget_get_parent (GTK_WIDGET (next_sibling)) == GTK_WIDGET (view));

  gtk_widget_insert_before (GTK_WIDGET (layer), GTK_WIDGET (view), GTK_WIDGET (next_sibling));
}


/**
 * shumate_view_insert_layer_above:
 * @view: a #ShumateView
 * @layer: a #ShumateLayer
 * @next_sibling: (nullable): a #ShumateLayer that is a child of @view, or %NULL
 *
 * Adds @layer to @view above @next_sibling or, if @next_sibling is %NULL, at
 * the bottom of the layer list.
 */
void
shumate_view_insert_layer_above (ShumateView  *view,
                                 ShumateLayer *layer,
                                 ShumateLayer *next_sibling)
{
  g_return_if_fail (SHUMATE_IS_VIEW (view));
  g_return_if_fail (SHUMATE_IS_LAYER (layer));
  g_return_if_fail (next_sibling == NULL || SHUMATE_IS_LAYER (next_sibling));
  g_return_if_fail (next_sibling == NULL || gtk_widget_get_parent (GTK_WIDGET (next_sibling)) == GTK_WIDGET (view));

  gtk_widget_insert_after (GTK_WIDGET (layer), GTK_WIDGET (view), GTK_WIDGET (next_sibling));
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
