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
 * The #ShumateMap is a #GtkWidget to display maps.  It supports two modes
 * of scrolling:
 * <itemizedlist>
 *   <listitem><para>Push: the normal behavior where the maps don't move
 *   after the user stopped scrolling;</para></listitem>
 *   <listitem><para>Kinetic: the behavior where the maps decelerate after
 *   the user stopped scrolling.</para></listitem>
 * </itemizedlist>
 *
 * You can use the same #ShumateMap to display many types of maps.  In
 * Shumate they are called map sources.  You can change the #map-source
 * property at anytime to replace the current displayed map.
 *
 * The maps are downloaded from Internet from open maps sources (like
 * <ulink role="online-location"
 * url="http://www.openstreetmap.org">OpenStreetMap</ulink>).  Maps are divided
 * in tiles for each zoom level.  When a tile is requested, #ShumateMap will
 * first check if it is in cache (in the user's cache dir under shumate). If
 * an error occurs during download, an error tile will be displayed.
 */

#include "shumate-map.h"

#include "shumate.h"
#include "shumate-enum-types.h"
#include "shumate-kinetic-scrolling-private.h"
#include "shumate-marshal.h"
#include "shumate-map-layer.h"
#include "shumate-map-source.h"
#include "shumate-map-source-registry.h"
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
  ShumateMap *map;
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
  ShumateKineticScrolling *kinetic_scrolling;
  ShumateMap *map;
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

  gboolean zoom_on_double_click;
  gboolean animate_zoom;

  ShumateState state; /* View's global state */

  // shumate_map_go_to's context, kept for stop_go_to
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

  double zoom_level_begin;
  double rotate_begin;

  double focus_lat;
  double focus_lon;
  double accumulated_scroll_dy;
  double gesture_begin_lat;
  double gesture_begin_lon;
  double drag_begin_x;
  double drag_begin_y;
} ShumateMapPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (ShumateMap, shumate_map, GTK_TYPE_WIDGET);

static double
positive_mod (double i, double n)
{
  return fmod (fmod (i, n) + n, n);
}

static void
move_location_to_coords (ShumateMap *self,
                         double lat,
                         double lon,
                         double x,
                         double y)
{
  ShumateMapPrivate *priv = shumate_map_get_instance_private (self);
  ShumateMapSource *map_source = shumate_viewport_get_reference_map_source (priv->viewport);
  double zoom_level = shumate_viewport_get_zoom_level (priv->viewport);
  double tile_size, map_width, map_height;
  double map_x, map_y;
  double target_lat, target_lon;
  double target_map_x, target_map_y;
  double current_lat, current_lon;
  double current_map_x, current_map_y;
  double new_map_x, new_map_y;

  if (map_source == NULL)
    return;

  tile_size = shumate_map_source_get_tile_size_at_zoom (map_source, zoom_level);
  map_width = tile_size * shumate_map_source_get_column_count (map_source, zoom_level);
  map_height = tile_size * shumate_map_source_get_row_count (map_source, zoom_level);

  map_x = shumate_map_source_get_x (map_source, zoom_level, lon);
  map_y = shumate_map_source_get_y (map_source, zoom_level, lat);

  current_lat = shumate_location_get_latitude (SHUMATE_LOCATION (priv->viewport));
  current_lon = shumate_location_get_longitude (SHUMATE_LOCATION (priv->viewport));
  current_map_x = shumate_map_source_get_x (map_source, zoom_level, current_lon);
  current_map_y = shumate_map_source_get_y (map_source, zoom_level, current_lat);

  shumate_viewport_widget_coords_to_location (priv->viewport, GTK_WIDGET (self), x, y, &target_lat, &target_lon);
  target_map_x = shumate_map_source_get_x (map_source, zoom_level, target_lon);
  target_map_y = shumate_map_source_get_y (map_source, zoom_level, target_lat);

  new_map_x = positive_mod (current_map_x - (target_map_x - map_x), map_width);
  new_map_y = positive_mod (current_map_y - (target_map_y - map_y), map_height);

  shumate_location_set_location (SHUMATE_LOCATION (priv->viewport),
                                 shumate_map_source_get_latitude (map_source, zoom_level, new_map_y),
                                 shumate_map_source_get_longitude (map_source, zoom_level, new_map_x));
}

static void
move_viewport_from_pixel_offset (ShumateMap *self,
                                 double      latitude,
                                 double      longitude,
                                 double      offset_x,
                                 double      offset_y)
{
  ShumateMapPrivate *priv = shumate_map_get_instance_private (self);
  ShumateMapSource *map_source;
  double x, y;
  double lat, lon;

  g_assert (SHUMATE_IS_MAP (self));

  map_source = shumate_viewport_get_reference_map_source (priv->viewport);
  if (!map_source)
    return;

  shumate_viewport_location_to_widget_coords (priv->viewport, GTK_WIDGET (self), latitude, longitude, &x, &y);

  x -= offset_x;
  y -= offset_y;

  shumate_viewport_widget_coords_to_location (priv->viewport, GTK_WIDGET (self), x, y, &lat, &lon);

  lat = fmod (lat + 90, 180) - 90;
  lon = fmod (lon + 180, 360) - 180;

  shumate_location_set_location (SHUMATE_LOCATION (priv->viewport), lat, lon);
}

static void
cancel_deceleration (ShumateMap *self)
{
  ShumateMapPrivate *priv = shumate_map_get_instance_private (self);

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
  ShumateMap *map = data->map;
  int64_t current_time_us;
  double elapsed_us;
  double position;

  g_assert (SHUMATE_IS_MAP (map));

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

      move_viewport_from_pixel_offset (map,
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
      cancel_deceleration (map);
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
start_deceleration (ShumateMap *self,
                    double      h_velocity,
                    double      v_velocity)
{
  ShumateMapPrivate *priv = shumate_map_get_instance_private (self);
  GdkFrameClock *frame_clock;
  KineticScrollData *data;
  graphene_vec2_t velocity;

  g_assert (priv->deceleration_tick_id == 0);

  frame_clock = gtk_widget_get_frame_clock (GTK_WIDGET (self));

  graphene_vec2_init (&velocity, h_velocity, v_velocity);

  data = g_new0 (KineticScrollData, 1);
  data->map = self;
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
  ShumateMapPrivate *priv = shumate_map_get_instance_private (ctx->map);
  int64_t now_us;
  double latitude, longitude;
  double progress;

  g_assert (SHUMATE_IS_MAP (ctx->map));
  g_assert (ctx->duration_us >= 0);

  now_us = g_get_monotonic_time ();
  gtk_widget_queue_allocate (widget);

  if (now_us >= ctx->start_us + ctx->duration_us)
    {
      shumate_location_set_location (SHUMATE_LOCATION (priv->viewport),
                                     ctx->to_latitude,
                                     ctx->to_longitude);
      shumate_map_stop_go_to (ctx->map);
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
on_drag_gesture_drag_begin (ShumateMap     *self,
                            double          start_x,
                            double          start_y,
                            GtkGestureDrag *gesture)
{
  ShumateMapPrivate *priv = shumate_map_get_instance_private (self);

  g_assert (SHUMATE_IS_MAP (self));

  cancel_deceleration (self);

  priv->drag_begin_x = start_x;
  priv->drag_begin_y = start_y;

  shumate_viewport_widget_coords_to_location (priv->viewport, GTK_WIDGET (self),
                                              start_x, start_y,
                                              &priv->gesture_begin_lat,
                                              &priv->gesture_begin_lon);

  gtk_widget_set_cursor_from_name (GTK_WIDGET (self), "grabbing");
}

static void
on_drag_gesture_drag_update (ShumateMap     *self,
                             double          offset_x,
                             double          offset_y,
                             GtkGestureDrag *gesture)
{
  ShumateMapPrivate *priv = shumate_map_get_instance_private (self);

  move_location_to_coords (self,
                           priv->gesture_begin_lat,
                           priv->gesture_begin_lon,
                           priv->drag_begin_x + offset_x,
                           priv->drag_begin_y + offset_y);
}

static void
on_drag_gesture_drag_end (ShumateMap     *self,
                          double          offset_x,
                          double          offset_y,
                          GtkGestureDrag *gesture)
{
  ShumateMapPrivate *priv = shumate_map_get_instance_private (self);

  g_assert (SHUMATE_IS_MAP (self));

  gtk_widget_set_cursor_from_name (GTK_WIDGET (self), "grab");

  priv->gesture_begin_lon = 0;
  priv->gesture_begin_lat = 0;
}

static void
view_swipe_cb (GtkGestureSwipe *swipe_gesture,
               double           velocity_x,
               double           velocity_y,
               ShumateMap      *self)
{
  start_deceleration (self, velocity_x, velocity_y);
}

static void
set_zoom_level (ShumateMap *self,
                double      zoom_level)
{
  ShumateMapPrivate *priv = shumate_map_get_instance_private (self);
  ShumateMapSource *map_source;
  double lat, lon;

  g_object_freeze_notify (G_OBJECT (priv->viewport));

  map_source = shumate_viewport_get_reference_map_source (priv->viewport);
  if (map_source)
    shumate_viewport_widget_coords_to_location (priv->viewport,
                                                GTK_WIDGET (self),
                                                priv->current_x, priv->current_y,
                                                &lat, &lon);

  shumate_viewport_set_zoom_level (priv->viewport, zoom_level);

  if (map_source)
    move_location_to_coords (self, lat, lon, priv->current_x, priv->current_y);

  g_object_thaw_notify (G_OBJECT (priv->viewport));
}

static gboolean
on_scroll_controller_scroll (ShumateMap               *self,
                             double                    dx,
                             double                    dy,
                             GtkEventControllerScroll *controller)
{
  ShumateMapPrivate *priv = shumate_map_get_instance_private (self);
  double zoom_level = shumate_viewport_get_zoom_level (priv->viewport);

  if (dy < 0)
    zoom_level += 0.2;
  else if (dy > 0)
    zoom_level -= 0.2;

  /* snap to the nearest 1/5 of a zoom level */
  set_zoom_level (self, roundf (zoom_level * 5) / 5);

  return TRUE;
}

static void
on_zoom_gesture_begin (ShumateMap       *self,
                       GdkEventSequence *seq,
                       GtkGestureZoom   *zoom)
{
  ShumateMapPrivate *priv = shumate_map_get_instance_private (self);
  double zoom_level = shumate_viewport_get_zoom_level (priv->viewport);
  double x, y;

  gtk_gesture_set_state (GTK_GESTURE (zoom), GTK_EVENT_SEQUENCE_CLAIMED);
  cancel_deceleration (self);

  priv->zoom_level_begin = zoom_level;

  gtk_gesture_get_bounding_box_center (GTK_GESTURE (zoom), &x, &y);
  shumate_viewport_widget_coords_to_location (priv->viewport, GTK_WIDGET (self),
                                              x, y,
                                              &priv->gesture_begin_lat,
                                              &priv->gesture_begin_lon);
}

static void
on_zoom_gesture_update (ShumateMap       *self,
                        GdkEventSequence *seq,
                        GtkGestureZoom   *zoom)
{
  ShumateMapPrivate *priv = shumate_map_get_instance_private (self);
  double x, y;
  double scale = gtk_gesture_zoom_get_scale_delta (zoom);

  gtk_gesture_get_bounding_box_center (GTK_GESTURE (zoom), &x, &y);
  shumate_viewport_set_zoom_level (priv->viewport, priv->zoom_level_begin + log (scale) / G_LN2);
  move_location_to_coords (self, priv->gesture_begin_lat, priv->gesture_begin_lon, x, y);
}

static void
on_rotate_gesture_begin (ShumateMap *self,
                         GdkEventSequence *seq,
                         GtkGestureRotate *rotate)
{
  ShumateMapPrivate *priv = shumate_map_get_instance_private (self);
  double rotation = shumate_viewport_get_rotation (priv->viewport);

  gtk_gesture_set_state (GTK_GESTURE (rotate), GTK_EVENT_SEQUENCE_CLAIMED);
  cancel_deceleration (self);

  priv->rotate_begin = rotation;
}

static void
on_rotate_gesture_update (ShumateMap *self,
                          GdkEventSequence *seq,
                          GtkGestureRotate *rotate)
{
  ShumateMapPrivate *priv = shumate_map_get_instance_private (self);
  double rotation;
  double x, y;

  rotation = gtk_gesture_rotate_get_angle_delta (rotate) + priv->rotate_begin;

  /* snap to due north */
  if (fabs (fmod (rotation - 0.25, G_PI * 2)) < 0.5)
    rotation = 0.0;

  shumate_viewport_set_rotation (priv->viewport, rotation);
  gtk_gesture_get_bounding_box_center (GTK_GESTURE (rotate), &x, &y);
  move_location_to_coords (self, priv->gesture_begin_lat, priv->gesture_begin_lon, x, y);
}

static void
on_motion_controller_motion (ShumateMap               *self,
                             double                    x,
                             double                    y,
                             GtkEventControllerMotion *controller)
{
  ShumateMapPrivate *priv = shumate_map_get_instance_private (self);

  priv->current_x = x;
  priv->current_y = y;
}

static void
shumate_map_go_to_with_duration (ShumateMap *self,
                                 double      latitude,
                                 double      longitude,
                                 guint       duration_ms)
{
  ShumateMapPrivate *priv = shumate_map_get_instance_private (self);
  GoToContext *ctx;

  if (duration_ms == 0)
    {
      shumate_map_center_on (self, latitude, longitude);
      return;
    }

  shumate_map_stop_go_to (self);

  ctx = g_new (GoToContext, 1);
  ctx->start_us = g_get_monotonic_time ();
  ctx->duration_us = ms_to_us (duration_ms);
  ctx->from_latitude = shumate_location_get_latitude (SHUMATE_LOCATION (priv->viewport));
  ctx->from_longitude = shumate_location_get_longitude (SHUMATE_LOCATION (priv->viewport));
  ctx->to_latitude = latitude;
  ctx->to_longitude = longitude;
  ctx->map = self;

  /* We keep a reference for stop */
  priv->goto_context = ctx;

  ctx->tick_id = gtk_widget_add_tick_callback (GTK_WIDGET (self), go_to_tick_cb, ctx, NULL);
}

static void
shumate_map_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  ShumateMap *view = SHUMATE_MAP (object);
  ShumateMapPrivate *priv = shumate_map_get_instance_private (view);

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
shumate_map_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  ShumateMap *view = SHUMATE_MAP (object);

  switch (prop_id)
    {
    case PROP_ZOOM_ON_DOUBLE_CLICK:
      shumate_map_set_zoom_on_double_click (view, g_value_get_boolean (value));
      break;

    case PROP_ANIMATE_ZOOM:
      shumate_map_set_animate_zoom (view, g_value_get_boolean (value));
      break;

    case PROP_GO_TO_DURATION:
      shumate_map_set_go_to_duration (view, g_value_get_uint (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}


static void
shumate_map_dispose (GObject *object)
{
  ShumateMap *view = SHUMATE_MAP (object);
  ShumateMapPrivate *priv = shumate_map_get_instance_private (view);
  GtkWidget *child;

  if (priv->goto_context != NULL)
    shumate_map_stop_go_to (view);

  while ((child = gtk_widget_get_first_child (GTK_WIDGET (object))))
    gtk_widget_unparent (child);

  g_clear_object (&priv->viewport);

  g_clear_handle_id (&priv->zoom_timeout, g_source_remove);

  G_OBJECT_CLASS (shumate_map_parent_class)->dispose (object);
}

static void
shumate_map_class_init (ShumateMapClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  object_class->dispose = shumate_map_dispose;
  object_class->get_property = shumate_map_get_property;
  object_class->set_property = shumate_map_set_property;

  /**
   * ShumateMap:zoom-on-double-click:
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
   * ShumateMap:animate-zoom:
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
   * ShumateMap:state:
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
   * ShumateMap:go-to-duration:
   *
   * The duration of an animation when going to a location, in milliseconds.
   * A value of 0 means that the duration is calculated automatically for you.
   *
   * Please note that animation of #shumate_map_ensure_visible also
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
   * ShumateMap::animation-completed:
   *
   * The #ShumateMap::animation-completed signal is emitted when any animation in the view
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
shumate_map_init (ShumateMap *self)
{
  ShumateMapPrivate *priv = shumate_map_get_instance_private (self);
  GtkGesture *drag_gesture;
  GtkEventController *scroll_controller;
  GtkEventController *motion_controller;
  GtkGesture *swipe_gesture;
  GtkGesture *zoom_gesture;
  GtkGesture *rotate_gesture;

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

  gtk_widget_set_cursor_from_name (GTK_WIDGET (self), "grab");

  /* Setup viewport */
  priv->viewport = shumate_viewport_new ();

  /* Setup license */
  drag_gesture = gtk_gesture_drag_new ();
  g_signal_connect_swapped (drag_gesture, "drag-begin", G_CALLBACK (on_drag_gesture_drag_begin), self);
  g_signal_connect_swapped (drag_gesture, "drag-update", G_CALLBACK (on_drag_gesture_drag_update), self);
  g_signal_connect_swapped (drag_gesture, "drag-end", G_CALLBACK (on_drag_gesture_drag_end), self);
  gtk_widget_add_controller (GTK_WIDGET (self), GTK_EVENT_CONTROLLER (drag_gesture));

  swipe_gesture = gtk_gesture_swipe_new ();
  g_signal_connect (swipe_gesture, "swipe", G_CALLBACK (view_swipe_cb), self);
  gtk_widget_add_controller (GTK_WIDGET (self), GTK_EVENT_CONTROLLER (swipe_gesture));

  scroll_controller = gtk_event_controller_scroll_new (GTK_EVENT_CONTROLLER_SCROLL_VERTICAL|GTK_EVENT_CONTROLLER_SCROLL_DISCRETE);
  g_signal_connect_swapped (scroll_controller, "scroll", G_CALLBACK (on_scroll_controller_scroll), self);
  gtk_widget_add_controller (GTK_WIDGET (self), scroll_controller);

  zoom_gesture = gtk_gesture_zoom_new ();
  g_signal_connect_swapped (zoom_gesture, "begin", G_CALLBACK (on_zoom_gesture_begin), self);
  g_signal_connect_swapped (zoom_gesture, "update", G_CALLBACK (on_zoom_gesture_update), self);
  gtk_widget_add_controller (GTK_WIDGET (self), GTK_EVENT_CONTROLLER (zoom_gesture));

  motion_controller = gtk_event_controller_motion_new ();
  g_signal_connect_swapped (motion_controller, "motion", G_CALLBACK (on_motion_controller_motion), self);
  gtk_widget_add_controller (GTK_WIDGET (self), motion_controller);

  rotate_gesture = gtk_gesture_rotate_new ();
  g_signal_connect_swapped (rotate_gesture, "begin", G_CALLBACK (on_rotate_gesture_begin), self);
  g_signal_connect_swapped (rotate_gesture, "update", G_CALLBACK (on_rotate_gesture_update), self);
  gtk_widget_add_controller (GTK_WIDGET (self), GTK_EVENT_CONTROLLER (rotate_gesture));

  gtk_gesture_group (zoom_gesture, rotate_gesture);

  gtk_widget_set_overflow (GTK_WIDGET (self), GTK_OVERFLOW_HIDDEN);
}

/**
 * shumate_map_new:
 *
 * Creates an instance of #ShumateMap.
 *
 * Returns: a new #ShumateMap ready to be used as a #GtkWidget.
 */
ShumateMap *
shumate_map_new (void)
{
  return g_object_new (SHUMATE_TYPE_MAP, NULL);
}


ShumateMap *
shumate_map_new_simple (void)
{
  ShumateMap *view = g_object_new (SHUMATE_TYPE_MAP, NULL);
  g_autoptr(ShumateMapSourceRegistry) registry = NULL;
  ShumateMapSource *source;
  ShumateMapLayer *map_layer;
  ShumateViewport *viewport;

  viewport = shumate_map_get_viewport (view);
  registry = shumate_map_source_registry_new_with_defaults ();
  source = shumate_map_source_registry_get_by_id (registry, SHUMATE_MAP_SOURCE_OSM_MAPNIK);
  shumate_viewport_set_reference_map_source (viewport, source);
  map_layer = shumate_map_layer_new (source, viewport);
  shumate_map_add_layer (view, SHUMATE_LAYER (map_layer));

  return view;
}

/**
 * shumate_map_get_viewport:
 * @self: a #ShumateMap
 *
 * Get the #ShumateViewport used by this view.
 *
 * Returns: (transfer none): the #ShumateViewport
 */
ShumateViewport *
shumate_map_get_viewport (ShumateMap *self)
{
  ShumateMapPrivate *priv = shumate_map_get_instance_private (self);

  g_return_val_if_fail (SHUMATE_IS_MAP (self), NULL);

  return priv->viewport;
}

/**
 * shumate_map_center_on:
 * @self: a #ShumateMap
 * @latitude: the longitude to center the map at
 * @longitude: the longitude to center the map at
 *
 * Centers the map on these coordinates.
 */
void
shumate_map_center_on (ShumateMap *self,
                       double      latitude,
                       double      longitude)
{
  ShumateMapPrivate *priv = shumate_map_get_instance_private (self);

  g_return_if_fail (SHUMATE_IS_MAP (self));

  shumate_location_set_location (SHUMATE_LOCATION (priv->viewport), latitude, longitude);
}

/**
 * shumate_map_stop_go_to:
 * @self: a #ShumateMap
 *
 * Stop the go to animation.  The view will stay where it was when the
 * animation was stopped.
 */
void
shumate_map_stop_go_to (ShumateMap *self)
{
  ShumateMapPrivate *priv = shumate_map_get_instance_private (self);

  g_return_if_fail (SHUMATE_IS_MAP (self));

  if (priv->goto_context == NULL)
    return;

  gtk_widget_remove_tick_callback (GTK_WIDGET (self), priv->goto_context->tick_id);
  g_clear_pointer (&priv->goto_context, g_free);

  g_signal_emit (self, signals[ANIMATION_COMPLETED], go_to_quark, NULL);
}


/**
 * shumate_map_go_to:
 * @self: a #ShumateMap
 * @latitude: the longitude to center the map at
 * @longitude: the longitude to center the map at
 *
 * Move from the current position to these coordinates. All tiles in the
 * intermediate view WILL be loaded!
 */
void
shumate_map_go_to (ShumateMap *self,
                   double      latitude,
                   double      longitude)
{
  ShumateMapPrivate *priv = shumate_map_get_instance_private (self);
  guint duration;

  g_return_if_fail (SHUMATE_IS_MAP (self));
  g_return_if_fail (latitude >= SHUMATE_MIN_LATITUDE && latitude <= SHUMATE_MAX_LATITUDE);
  g_return_if_fail (longitude >= SHUMATE_MIN_LONGITUDE && longitude <= SHUMATE_MAX_LONGITUDE);

  duration = priv->go_to_duration;
  if (duration == 0) /* calculate duration from zoom level */
      duration = 500 * shumate_viewport_get_zoom_level (priv->viewport) / 2.0;

  shumate_map_go_to_with_duration (self, latitude, longitude, duration);
}

/**
 * shumate_map_get_go_to_duration:
 * @self: a #ShumateMap
 *
 * Get the 'go-to-duration' property.
 *
 * Returns: the animation duration when calling shumate_map_go_to(),
 *   in milliseconds.
 */
guint
shumate_map_get_go_to_duration (ShumateMap *self)
{
  ShumateMapPrivate *priv = shumate_map_get_instance_private (self);

  g_return_val_if_fail (SHUMATE_IS_MAP (self), 0);

  return priv->go_to_duration;
}

/**
 * shumate_map_set_go_to_duration:
 * @self: a #ShumateMap
 * @duration: the animation duration, in milliseconds
 *
 * Set the duration of the transition of shumate_map_go_to().
 */
void
shumate_map_set_go_to_duration (ShumateMap *self,
                                guint       duration)
{
  ShumateMapPrivate *priv = shumate_map_get_instance_private (self);

  g_return_if_fail (SHUMATE_IS_MAP (self));

  if (priv->go_to_duration == duration)
    return;

  priv->go_to_duration = duration;
  g_object_notify_by_pspec (G_OBJECT (self), obj_properties[PROP_GO_TO_DURATION]);
}

/**
 * shumate_map_add_layer:
 * @self: a #ShumateMap
 * @layer: a #ShumateLayer
 *
 * Adds a new layer to the view
 */
void
shumate_map_add_layer (ShumateMap   *self,
                       ShumateLayer *layer)
{
  g_return_if_fail (SHUMATE_IS_MAP (self));
  g_return_if_fail (SHUMATE_IS_LAYER (layer));

  gtk_widget_insert_before (GTK_WIDGET (layer), GTK_WIDGET (self), NULL);
}


/**
 * shumate_map_insert_layer_behind:
 * @self: a #ShumateMap
 * @layer: a #ShumateLayer
 * @next_sibling: (nullable): a #ShumateLayer that is a child of @self, or %NULL
 *
 * Adds @layer to @self behind @next_sibling or, if @next_sibling is %NULL, at
 * the top of the layer list.
 */
void
shumate_map_insert_layer_behind (ShumateMap   *self,
                                 ShumateLayer *layer,
                                 ShumateLayer *next_sibling)
{
  g_return_if_fail (SHUMATE_IS_MAP (self));
  g_return_if_fail (SHUMATE_IS_LAYER (layer));
  g_return_if_fail (next_sibling == NULL || SHUMATE_IS_LAYER (next_sibling));
  g_return_if_fail (next_sibling == NULL || gtk_widget_get_parent (GTK_WIDGET (next_sibling)) == GTK_WIDGET (self));

  gtk_widget_insert_before (GTK_WIDGET (layer), GTK_WIDGET (self), GTK_WIDGET (next_sibling));
}


/**
 * shumate_map_insert_layer_above:
 * @self: a #ShumateMap
 * @layer: a #ShumateLayer
 * @next_sibling: (nullable): a #ShumateLayer that is a child of @self, or %NULL
 *
 * Adds @layer to @self above @next_sibling or, if @next_sibling is %NULL, at
 * the bottom of the layer list.
 */
void
shumate_map_insert_layer_above (ShumateMap   *self,
                                ShumateLayer *layer,
                                ShumateLayer *next_sibling)
{
  g_return_if_fail (SHUMATE_IS_MAP (self));
  g_return_if_fail (SHUMATE_IS_LAYER (layer));
  g_return_if_fail (next_sibling == NULL || SHUMATE_IS_LAYER (next_sibling));
  g_return_if_fail (next_sibling == NULL || gtk_widget_get_parent (GTK_WIDGET (next_sibling)) == GTK_WIDGET (self));

  gtk_widget_insert_after (GTK_WIDGET (layer), GTK_WIDGET (self), GTK_WIDGET (next_sibling));
}


/**
 * shumate_map_remove_layer:
 * @self: a #ShumateMap
 * @layer: a #ShumateLayer
 *
 * Removes the given layer from the view
 */
void
shumate_map_remove_layer (ShumateMap  *self,
                          ShumateLayer *layer)
{
  g_return_if_fail (SHUMATE_IS_MAP (self));
  g_return_if_fail (SHUMATE_IS_LAYER (layer));

  if (gtk_widget_get_parent (GTK_WIDGET (layer)) != GTK_WIDGET (self))
    {
      g_critical ("The given ShumateLayer isn't a child of the view");
      return;
    }

  gtk_widget_unparent (GTK_WIDGET (layer));
}

/**
 * shumate_map_set_map_source:
 * @self: a #ShumateMap
 * @map_source: a #ShumateMapSource
 *
 * Changes the currently used map source. #g_object_unref() will be called on
 * the previous one.
 *
 * As a side effect, changing the primary map source will also clear all
 * secondary map sources.
 */
void
shumate_map_set_map_source (ShumateMap       *self,
                            ShumateMapSource *source)
{
  ShumateMapPrivate *priv = shumate_map_get_instance_private (self);
  ShumateMapSource *ref_map_source;

  g_return_if_fail (SHUMATE_IS_MAP (self));
  g_return_if_fail (SHUMATE_IS_MAP_SOURCE (source));

  ref_map_source = shumate_viewport_get_reference_map_source (priv->viewport);
  if (ref_map_source == source)
    return;

  shumate_viewport_set_reference_map_source (priv->viewport, source);
}

/**
 * shumate_map_set_zoom_on_double_click:
 * @self: a #ShumateMap
 * @value: a #gboolean
 *
 * Should the view zoom in and recenter when the user double click on the map.
 */
void
shumate_map_set_zoom_on_double_click (ShumateMap *self,
                                      gboolean    value)
{
  ShumateMapPrivate *priv = shumate_map_get_instance_private (self);

  g_return_if_fail (SHUMATE_IS_MAP (self));

  priv->zoom_on_double_click = value;
  g_object_notify_by_pspec (G_OBJECT (self), obj_properties[PROP_ZOOM_ON_DOUBLE_CLICK]);
}


/**
 * shumate_map_set_animate_zoom:
 * @self: a #ShumateMap
 * @value: a #gboolean
 *
 * Should the view animate zoom level changes.
 */
void
shumate_map_set_animate_zoom (ShumateMap *self,
                              gboolean    value)
{
  ShumateMapPrivate *priv = shumate_map_get_instance_private (self);

  g_return_if_fail (SHUMATE_IS_MAP (self));

  priv->animate_zoom = value;
  g_object_notify_by_pspec (G_OBJECT (self), obj_properties[PROP_ANIMATE_ZOOM]);
}

/**
 * shumate_map_get_zoom_on_double_click:
 * @self: a #ShumateMap
 *
 * Checks whether the view zooms on double click.
 *
 * Returns: TRUE if the view zooms on double click, FALSE otherwise.
 */
gboolean
shumate_map_get_zoom_on_double_click (ShumateMap *self)
{
  ShumateMapPrivate *priv = shumate_map_get_instance_private (self);

  g_return_val_if_fail (SHUMATE_IS_MAP (self), FALSE);

  return priv->zoom_on_double_click;
}


/**
 * shumate_map_get_animate_zoom:
 * @self: a #ShumateMap
 *
 * Checks whether the view animates zoom level changes.
 *
 * Returns: TRUE if the view animates zooms, FALSE otherwise.
 */
gboolean
shumate_map_get_animate_zoom (ShumateMap *self)
{
  ShumateMapPrivate *priv = shumate_map_get_instance_private (self);

  g_return_val_if_fail (SHUMATE_IS_MAP (self), FALSE);

  return priv->animate_zoom;
}

/**
 * shumate_map_get_state:
 * @self: a #ShumateMap
 *
 * Gets the view's state.
 *
 * Returns: the state.
 */
ShumateState
shumate_map_get_state (ShumateMap *self)
{
  ShumateMapPrivate *priv = shumate_map_get_instance_private (self);

  g_return_val_if_fail (SHUMATE_IS_MAP (self), SHUMATE_STATE_NONE);

  return priv->state;
}
