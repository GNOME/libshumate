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
 * ShumateMap:
 *
 * The Map widget is a [class@Gtk.Widget] that show and allows interaction with
 * the user.
 *
 * This is the base widget and doesn't have advanced features. You can check the
 * [class@Shumate.SimpleMap] for a ready-to-use widget.
 *
 * By default, a [class@Shumate.Viewport] is created and can be accessed with
 * [method@Shumate.Map.get_viewport].
 *
 * Unless created with [ctor@Shumate.Map.new_simple], the widget doesn't hold any
 * layer and won't show anything. A [class@Shumate.Layer] can be added or removed
 * using the [method@Shumate.Map.add_layer] or [method@Shumate.Map.remove_layer]
 * methods.
 *
 * ## CSS nodes
 *
 * `ShumateMap` has a single CSS node with the name “map-view”.
 */

#include "shumate-map.h"

#include "shumate.h"
#include "shumate-enum-types.h"
#include "shumate-inspector-page-private.h"
#include "shumate-inspector-settings-private.h"
#include "shumate-kinetic-scrolling-private.h"
#include "shumate-marshal.h"
#include "shumate-map-layer.h"
#include "shumate-map-source.h"
#include "shumate-map-source-registry.h"
#include "shumate-tile.h"
#include "shumate-license.h"
#include "shumate-location.h"
#include "shumate-viewport-private.h"

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <math.h>

#define DECELERATION_FRICTION 4.0
#define ZOOM_ANIMATION_MS 200
#define SCROLL_PIXELS_PER_LEVEL 50

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
  PROP_VIEWPORT,
  N_PROPERTIES
};

static guint signals[LAST_SIGNAL] = { 0, };

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };
static GQuark go_to_quark;

/* Between state values for go_to */
typedef struct
{
  int64_t duration_us;
  int64_t start_us;
  double to_latitude;
  double to_longitude;
  double to_zoom;
  double from_latitude;
  double from_longitude;
  double from_zoom;
  guint tick_id;
  gboolean zoom_animation : 1;
  gboolean zoom_deceleration : 1;
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

struct _ShumateMap
{
  GtkWidget parent_instance;

  ShumateViewport *viewport;

  gboolean zoom_on_double_click;
  gboolean animate_zoom;

  ShumateState state; /* View's global state */

  GoToContext *goto_context;

  guint deceleration_tick_id;

  guint zoom_timeout;

  guint go_to_duration;

  double current_x;
  double current_y;

  double zoom_level_begin;
  double rotate_begin;

  double gesture_begin_lat;
  double gesture_begin_lon;
  double drag_begin_x;
  double drag_begin_y;
};

G_DEFINE_TYPE (ShumateMap, shumate_map, GTK_TYPE_WIDGET);

static double
positive_mod (double i, double n)
{
  return fmod (fmod (i, n) + n, n);
}

static void
move_location_to_coords_calc (ShumateMap      *self,
                              double          *lat,
                              double          *lon,
                              double           x,
                              double           y,
                              ShumateViewport *viewport)
{
  ShumateMapSource *map_source = shumate_viewport_get_reference_map_source (viewport);
  double zoom_level = shumate_viewport_get_zoom_level (viewport);
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

  map_x = shumate_map_source_get_x (map_source, zoom_level, *lon);
  map_y = shumate_map_source_get_y (map_source, zoom_level, *lat);

  current_lat = shumate_location_get_latitude (SHUMATE_LOCATION (viewport));
  current_lon = shumate_location_get_longitude (SHUMATE_LOCATION (viewport));
  current_map_x = shumate_map_source_get_x (map_source, zoom_level, current_lon);
  current_map_y = shumate_map_source_get_y (map_source, zoom_level, current_lat);

  shumate_viewport_widget_coords_to_location (viewport, GTK_WIDGET (self), x, y, &target_lat, &target_lon);
  target_map_x = shumate_map_source_get_x (map_source, zoom_level, target_lon);
  target_map_y = shumate_map_source_get_y (map_source, zoom_level, target_lat);

  new_map_x = positive_mod (current_map_x - (target_map_x - map_x), map_width);
  new_map_y = positive_mod (current_map_y - (target_map_y - map_y), map_height);

  *lat = shumate_map_source_get_latitude (map_source, zoom_level, new_map_y);
  *lon = shumate_map_source_get_longitude (map_source, zoom_level, new_map_x);
}

static void
move_location_to_coords (ShumateMap *self,
                         double lat,
                         double lon,
                         double x,
                         double y)
{
  move_location_to_coords_calc (self, &lat, &lon, x, y, self->viewport);
  shumate_location_set_location (SHUMATE_LOCATION (self->viewport), lat, lon);
}

static void
move_viewport_from_pixel_offset (ShumateMap *self,
                                 double      latitude,
                                 double      longitude,
                                 double      offset_x,
                                 double      offset_y)
{
  ShumateMapSource *map_source;
  double x, y;
  double lat, lon;

  g_assert (SHUMATE_IS_MAP (self));

  map_source = shumate_viewport_get_reference_map_source (self->viewport);
  if (!map_source)
    return;

  shumate_viewport_location_to_widget_coords (self->viewport, GTK_WIDGET (self), latitude, longitude, &x, &y);

  x -= offset_x;
  y -= offset_y;

  shumate_viewport_widget_coords_to_location (self->viewport, GTK_WIDGET (self), x, y, &lat, &lon);

  lat = fmod (lat + 90, 180) - 90;
  lon = fmod (lon + 180, 360) - 180;

  shumate_location_set_location (SHUMATE_LOCATION (self->viewport), lat, lon);
}

static void
cancel_deceleration (ShumateMap *self)
{
  if (self->deceleration_tick_id > 0)
    {
      gtk_widget_remove_tick_callback (GTK_WIDGET (self), self->deceleration_tick_id);
      self->deceleration_tick_id = 0;
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
  GdkFrameClock *frame_clock;
  KineticScrollData *data;
  graphene_vec2_t velocity;

  g_assert (self->deceleration_tick_id == 0);

  frame_clock = gtk_widget_get_frame_clock (GTK_WIDGET (self));

  graphene_vec2_init (&velocity, h_velocity, v_velocity);

  data = g_new0 (KineticScrollData, 1);
  data->map = self;
  data->last_deceleration_time_us = gdk_frame_clock_get_frame_time (frame_clock);
  data->start_lat = shumate_location_get_latitude (SHUMATE_LOCATION (self->viewport));
  data->start_lon = shumate_location_get_longitude (SHUMATE_LOCATION (self->viewport));
  graphene_vec2_normalize (&velocity, &data->direction);
  data->kinetic_scrolling =
    shumate_kinetic_scrolling_new (DECELERATION_FRICTION,
                                   graphene_vec2_length (&velocity));

  self->deceleration_tick_id =
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

static inline double
ease_out_quad (double p)
{
  return 1 - (1 - p) * (1 - p);
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
  ShumateMap *self = SHUMATE_MAP (widget);
  GoToContext *ctx;
  int64_t now_us;
  double latitude, longitude;
  double progress;
  double current_zoom;

  g_assert (SHUMATE_IS_MAP (widget));
  g_assert (user_data == NULL);

  ctx = self->goto_context;

  g_assert (ctx != NULL);
  g_assert (ctx->duration_us >= 0);

  now_us = g_get_monotonic_time ();

  if (now_us >= ctx->start_us + ctx->duration_us)
    {
      shumate_location_set_location (SHUMATE_LOCATION (self->viewport),
                                     ctx->to_latitude,
                                     ctx->to_longitude);
      shumate_viewport_set_zoom_level (self->viewport, ctx->to_zoom);
      shumate_map_stop_go_to (SHUMATE_MAP (widget));
      return G_SOURCE_REMOVE;
    }

  progress = (now_us - ctx->start_us) / (double) ctx->duration_us;
  g_assert (progress >= 0.0 && progress <= 1.0);

  /* Apply the ease function to the progress itself */
  if (ctx->zoom_deceleration)
    progress = ease_out_quad (progress);
  else if (!ctx->zoom_animation)
    progress = ease_in_out_quad (progress);

  /* Interpolate zoom level */
  current_zoom = ctx->from_zoom + (ctx->to_zoom - ctx->from_zoom) * progress;
  shumate_viewport_set_zoom_level (self->viewport, current_zoom);

  /* If we're zooming, we need to adjust for that in the progress, otherwise
   * the animation will speed up at higher zoom levels. */
  if (ctx->to_zoom != ctx->from_zoom)
    progress = (pow (2, -ctx->from_zoom) - pow (2, -current_zoom))
                / (pow (2, -ctx->from_zoom) - pow (2, -ctx->to_zoom));

  /* Since progress already follows the easing curve, a simple LERP is guaranteed
   * to follow it too.
   */
  latitude = ctx->from_latitude + (ctx->to_latitude - ctx->from_latitude) * progress;
  longitude = ctx->from_longitude + (ctx->to_longitude - ctx->from_longitude) * progress;

  shumate_location_set_location (SHUMATE_LOCATION (self->viewport), latitude, longitude);

  return G_SOURCE_CONTINUE;
}

static void
on_drag_gesture_drag_begin (ShumateMap     *self,
                            double          start_x,
                            double          start_y,
                            GtkGestureDrag *gesture)
{
  g_assert (SHUMATE_IS_MAP (self));

  cancel_deceleration (self);

  self->drag_begin_x = start_x;
  self->drag_begin_y = start_y;

  shumate_viewport_widget_coords_to_location (self->viewport, GTK_WIDGET (self),
                                              start_x, start_y,
                                              &self->gesture_begin_lat,
                                              &self->gesture_begin_lon);

  gtk_widget_set_cursor_from_name (GTK_WIDGET (self), "grabbing");
}

static void
on_drag_gesture_drag_update (ShumateMap     *self,
                             double          offset_x,
                             double          offset_y,
                             GtkGestureDrag *gesture)
{
  move_location_to_coords (self,
                           self->gesture_begin_lat,
                           self->gesture_begin_lon,
                           self->drag_begin_x + offset_x,
                           self->drag_begin_y + offset_y);
}

static void
on_drag_gesture_drag_end (ShumateMap     *self,
                          double          offset_x,
                          double          offset_y,
                          GtkGestureDrag *gesture)
{
  g_assert (SHUMATE_IS_MAP (self));

  gtk_widget_set_cursor_from_name (GTK_WIDGET (self), "grab");

  self->gesture_begin_lon = 0;
  self->gesture_begin_lat = 0;
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
                double      zoom_level,
                double      animate)
{
  ShumateMapSource *map_source;
  double lat, lon;

  g_object_freeze_notify (G_OBJECT (self->viewport));

  map_source = shumate_viewport_get_reference_map_source (self->viewport);
  if (map_source)
    shumate_viewport_widget_coords_to_location (self->viewport,
                                                GTK_WIDGET (self),
                                                self->current_x, self->current_y,
                                                &lat, &lon);

  if (map_source)
    {
      g_autoptr(ShumateViewport) new_viewport = shumate_viewport_copy (self->viewport);
      shumate_viewport_set_zoom_level (new_viewport, zoom_level);
      move_location_to_coords_calc (self, &lat, &lon, self->current_x, self->current_y, new_viewport);

      shumate_map_go_to_full_with_duration (self,
                                            lat,
                                            lon,
                                            zoom_level,
                                            self->animate_zoom && animate ? ZOOM_ANIMATION_MS : 0);

      if (self->goto_context != NULL)
        self->goto_context->zoom_animation = TRUE;
    }
  else
    shumate_viewport_set_zoom_level (self->viewport, zoom_level);

  g_object_thaw_notify (G_OBJECT (self->viewport));
}

static gboolean
on_scroll_controller_scroll (ShumateMap               *self,
                             double                    dx,
                             double                    dy,
                             GtkEventControllerScroll *controller)
{
  double zoom_level = shumate_viewport_get_zoom_level (self->viewport);

  if (self->goto_context != NULL && self->goto_context->zoom_animation)
    zoom_level = self->goto_context->to_zoom;

  if (gtk_event_controller_scroll_get_unit (controller) == GDK_SCROLL_UNIT_SURFACE)
    /* Smooth scrolling with a touchpad or similar device */
    set_zoom_level (self, zoom_level - dy / SCROLL_PIXELS_PER_LEVEL, FALSE);
  else
    {
      zoom_level -= dy / 5;
      if (fabs(dy) == 1)
        /* Traditional discrete mouse, snap to the nearest 1/5 of a zoom level */
        set_zoom_level (self, roundf (zoom_level * 5) / 5, TRUE);
      else
        /* Smooth scrolling using a mouse.
         *
         * Various smooth-scrolling mice behave like "discrete" mice,
         * while emitting fractions of a scroll at the same time.
         * Do not round their events, or most of the scrolling gets ignored.*/
        set_zoom_level (self, zoom_level, TRUE);
    }

  return TRUE;
}

static gboolean
on_scroll_controller_decelerate (ShumateMap               *self,
                                 double                    vel_x,
                                 double                    vel_y,
                                 GtkEventControllerScroll *controller)
{
  double zoom_level = shumate_viewport_get_zoom_level (self->viewport);

  if (self->goto_context != NULL && self->goto_context->zoom_animation)
    zoom_level = self->goto_context->to_zoom;

  set_zoom_level (self, zoom_level - vel_y / SCROLL_PIXELS_PER_LEVEL / ZOOM_ANIMATION_MS, TRUE);
  if (self->goto_context != NULL)
    self->goto_context->zoom_deceleration = TRUE;

  return TRUE;
}

static void
on_zoom_gesture_begin (ShumateMap       *self,
                       GdkEventSequence *seq,
                       GtkGestureZoom   *zoom)
{
  double zoom_level = shumate_viewport_get_zoom_level (self->viewport);
  double x, y;

  gtk_gesture_set_state (GTK_GESTURE (zoom), GTK_EVENT_SEQUENCE_CLAIMED);
  cancel_deceleration (self);

  self->zoom_level_begin = zoom_level;

  gtk_gesture_get_bounding_box_center (GTK_GESTURE (zoom), &x, &y);
  shumate_viewport_widget_coords_to_location (self->viewport, GTK_WIDGET (self),
                                              x, y,
                                              &self->gesture_begin_lat,
                                              &self->gesture_begin_lon);
}

static void
on_zoom_gesture_update (ShumateMap       *self,
                        GdkEventSequence *seq,
                        GtkGestureZoom   *zoom)
{
  double x, y;
  double scale = gtk_gesture_zoom_get_scale_delta (zoom);

  gtk_gesture_get_bounding_box_center (GTK_GESTURE (zoom), &x, &y);
  shumate_viewport_set_zoom_level (self->viewport, self->zoom_level_begin + log (scale) / G_LN2);
  move_location_to_coords (self, self->gesture_begin_lat, self->gesture_begin_lon, x, y);
}

static void
on_rotate_gesture_begin (ShumateMap *self,
                         GdkEventSequence *seq,
                         GtkGestureRotate *rotate)
{
  double rotation = shumate_viewport_get_rotation (self->viewport);

  gtk_gesture_set_state (GTK_GESTURE (rotate), GTK_EVENT_SEQUENCE_CLAIMED);
  cancel_deceleration (self);

  self->rotate_begin = rotation;
}

static void
on_rotate_gesture_update (ShumateMap *self,
                          GdkEventSequence *seq,
                          GtkGestureRotate *rotate)
{
  double rotation;
  double x, y;

  rotation = gtk_gesture_rotate_get_angle_delta (rotate) + self->rotate_begin;

  /* snap to due north */
  if (fabs (fmod (rotation - 0.25, G_PI * 2)) < 0.5)
    rotation = 0.0;

  shumate_viewport_set_rotation (self->viewport, rotation);
  gtk_gesture_get_bounding_box_center (GTK_GESTURE (rotate), &x, &y);
  move_location_to_coords (self, self->gesture_begin_lat, self->gesture_begin_lon, x, y);
}

static void
on_click_gesture_pressed (ShumateMap      *self,
                          int              n_press,
                          double           x,
                          double           y,
                          GtkGestureClick *click)
{
  if (n_press == 2)
    {
      double zoom_level = shumate_viewport_get_zoom_level (self->viewport);
      self->current_x = x;
      self->current_y = y;
      set_zoom_level (self, zoom_level + 1, TRUE);
    }
}

static void
on_motion_controller_motion (ShumateMap               *self,
                             double                    x,
                             double                    y,
                             GtkEventControllerMotion *controller)
{
  self->current_x = x;
  self->current_y = y;
}

static void
on_arrow_key (GtkWidget  *widget,
              const char *action,
              GVariant   *args)
{
  ShumateMap *self = SHUMATE_MAP (widget);
  int dx, dy;
  double lat, lon;

  g_variant_get (args, "(ii)", &dx, &dy);

  shumate_viewport_widget_coords_to_location (self->viewport, widget, dx * 25, dy * 25, &lat, &lon);
  move_location_to_coords (self, lat, lon, 0, 0);
}

static void
shumate_map_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  ShumateMap *self = SHUMATE_MAP (object);

  switch (prop_id)
    {
    case PROP_ZOOM_ON_DOUBLE_CLICK:
      g_value_set_boolean (value, self->zoom_on_double_click);
      break;

    case PROP_ANIMATE_ZOOM:
      g_value_set_boolean (value, self->animate_zoom);
      break;

    case PROP_STATE:
      g_value_set_enum (value, self->state);
      break;

    case PROP_GO_TO_DURATION:
      g_value_set_uint (value, self->go_to_duration);
      break;

    case PROP_VIEWPORT:
      g_value_set_object (value, self->viewport);
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
  ShumateMap *self = SHUMATE_MAP (object);

  switch (prop_id)
    {
    case PROP_ZOOM_ON_DOUBLE_CLICK:
      shumate_map_set_zoom_on_double_click (self, g_value_get_boolean (value));
      break;

    case PROP_ANIMATE_ZOOM:
      shumate_map_set_animate_zoom (self, g_value_get_boolean (value));
      break;

    case PROP_GO_TO_DURATION:
      shumate_map_set_go_to_duration (self, g_value_get_uint (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}


static void
shumate_map_dispose (GObject *object)
{
  ShumateMap *self = SHUMATE_MAP (object);
  GtkWidget *child;

  if (self->goto_context != NULL)
    shumate_map_stop_go_to (self);

  while ((child = gtk_widget_get_first_child (GTK_WIDGET (object))))
    gtk_widget_unparent (child);

  g_clear_object (&self->viewport);

  g_clear_handle_id (&self->zoom_timeout, g_source_remove);

  G_OBJECT_CLASS (shumate_map_parent_class)->dispose (object);
}

static void
shumate_map_snapshot (GtkWidget *widget,
                      GtkSnapshot *snapshot)
{
  ShumateMap *self = SHUMATE_MAP (widget);
  ShumateInspectorSettings *settings = shumate_inspector_settings_get_default ();
  ShumateMapSource *map_source = shumate_viewport_get_reference_map_source (self->viewport);
  int width, height;

  GTK_WIDGET_CLASS (shumate_map_parent_class)->snapshot (GTK_WIDGET (self), snapshot);

  if (shumate_inspector_settings_get_show_debug_overlay (settings))
    {
      g_autoptr(GString) all_debug_text = g_string_new ("");
      PangoContext *context = gtk_widget_create_pango_context (GTK_WIDGET (self));
      g_autoptr(PangoLayout) layout = NULL;
      double lat = shumate_location_get_latitude (SHUMATE_LOCATION (self->viewport));
      double lon = shumate_location_get_longitude (SHUMATE_LOCATION (self->viewport));
      double zoom = shumate_viewport_get_zoom_level (self->viewport);
      double rot = shumate_viewport_get_rotation (self->viewport);

      g_string_append (all_debug_text, "<tt>");
      g_string_append_printf (all_debug_text,
                              "lat = %9.5f, lon = %10.5f\nzoom = %5.2f, rot = %5.1f\n",
                              lat, lon, zoom, rot * 180 / G_PI);

      if (map_source != NULL)
        g_string_append_printf (all_debug_text, "tile size = %4dpx (%7.2f)\n",
                                shumate_map_source_get_tile_size (map_source),
                                shumate_map_source_get_tile_size_at_zoom (map_source, zoom));

      g_string_append (all_debug_text, "\n");

      for (GtkWidget *w = gtk_widget_get_first_child (GTK_WIDGET (self));
           w != NULL;
           w = gtk_widget_get_next_sibling (w))
        {
          ShumateLayer *layer = SHUMATE_LAYER (w);
          g_string_append_printf (all_debug_text, "<b>%s</b>\n", G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (layer)));
          if (SHUMATE_LAYER_GET_CLASS (layer)->get_debug_text != NULL)
            {
              g_autofree char *debug_text = SHUMATE_LAYER_GET_CLASS (layer)->get_debug_text (layer);
              if (debug_text != NULL)
                {
                  g_string_append (all_debug_text, debug_text);
                  g_string_append (all_debug_text, "\n");
                }
            }
        }

      g_string_append (all_debug_text, "</tt>");

      layout = pango_layout_new (context);
      pango_layout_set_markup (layout, all_debug_text->str, -1);
      pango_layout_set_width (layout, gtk_widget_get_width (GTK_WIDGET (self)) * PANGO_SCALE);

      pango_layout_get_pixel_size (layout, &width, &height);
      gtk_snapshot_append_color (snapshot, &(GdkRGBA){1, 1, 1, 0.7}, &GRAPHENE_RECT_INIT (0, 0, width, height));

      gtk_snapshot_append_layout (snapshot, layout, &(GdkRGBA){0, 0, 0, 1});

      g_object_unref (context);
    }
}

static void
shumate_map_class_init (ShumateMapClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = shumate_map_dispose;
  object_class->get_property = shumate_map_get_property;
  object_class->set_property = shumate_map_set_property;

  widget_class->snapshot = shumate_map_snapshot;

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

  /**
   * ShumateMap:viewport:
   *
   * The viewport, which contains information about the center, rotation, zoom,
   * etc. of the map.
   */
  obj_properties[PROP_VIEWPORT] =
    g_param_spec_object ("viewport",
                         "Viewport",
                         "Viewport",
                         SHUMATE_TYPE_VIEWPORT,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

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

  /* Arrow keys */
  gtk_widget_class_install_action (widget_class, "pan", "(ii)", on_arrow_key);
  gtk_widget_class_add_binding_action (widget_class,
                                       GDK_KEY_Left, 0,
                                       "pan",
                                       "(ii)", -1, 0);
  gtk_widget_class_add_binding_action (widget_class,
                                       GDK_KEY_Right, 0,
                                       "pan",
                                       "(ii)", 1, 0);
  gtk_widget_class_add_binding_action (widget_class,
                                       GDK_KEY_Up, 0,
                                       "pan",
                                       "(ii)", 0, -1);
  gtk_widget_class_add_binding_action (widget_class,
                                       GDK_KEY_Down, 0,
                                       "pan",
                                       "(ii)", 0, 1);

  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
  gtk_widget_class_set_css_name (widget_class, "map-view");

  go_to_quark = g_quark_from_static_string ("go-to");

  shumate_inspector_page_ensure_registered ();
}

static void
shumate_map_init (ShumateMap *self)
{
  ShumateInspectorSettings *settings = shumate_inspector_settings_get_default ();
  GtkGesture *drag_gesture;
  GtkEventController *scroll_controller;
  GtkEventController *motion_controller;
  GtkGesture *swipe_gesture;
  GtkGesture *zoom_gesture;
  GtkGesture *rotate_gesture;
  GtkGesture *click_gesture;

  self->viewport = shumate_viewport_new ();
  self->zoom_on_double_click = TRUE;
  self->animate_zoom = TRUE;
  self->state = SHUMATE_STATE_NONE;
  self->goto_context = NULL;
  self->go_to_duration = 0;

  g_signal_connect_object (settings, "notify::show-debug-overlay", G_CALLBACK (gtk_widget_queue_draw), self, G_CONNECT_SWAPPED);

  gtk_widget_set_cursor_from_name (GTK_WIDGET (self), "grab");

  drag_gesture = gtk_gesture_drag_new ();
  g_signal_connect_swapped (drag_gesture, "drag-begin", G_CALLBACK (on_drag_gesture_drag_begin), self);
  g_signal_connect_swapped (drag_gesture, "drag-update", G_CALLBACK (on_drag_gesture_drag_update), self);
  g_signal_connect_swapped (drag_gesture, "drag-end", G_CALLBACK (on_drag_gesture_drag_end), self);
  gtk_widget_add_controller (GTK_WIDGET (self), GTK_EVENT_CONTROLLER (drag_gesture));

  swipe_gesture = gtk_gesture_swipe_new ();
  g_signal_connect (swipe_gesture, "swipe", G_CALLBACK (view_swipe_cb), self);
  gtk_widget_add_controller (GTK_WIDGET (self), GTK_EVENT_CONTROLLER (swipe_gesture));

  scroll_controller = gtk_event_controller_scroll_new (GTK_EVENT_CONTROLLER_SCROLL_VERTICAL|GTK_EVENT_CONTROLLER_SCROLL_KINETIC);
  g_signal_connect_swapped (scroll_controller, "scroll", G_CALLBACK (on_scroll_controller_scroll), self);
  g_signal_connect_swapped (scroll_controller, "decelerate", G_CALLBACK (on_scroll_controller_decelerate), self);
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

  click_gesture = gtk_gesture_click_new ();
  gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (click_gesture), GDK_BUTTON_PRIMARY);
  g_signal_connect_swapped (click_gesture, "pressed", G_CALLBACK (on_click_gesture_pressed), self);
  gtk_widget_add_controller (GTK_WIDGET (self), GTK_EVENT_CONTROLLER (click_gesture));

  gtk_widget_set_overflow (GTK_WIDGET (self), GTK_OVERFLOW_HIDDEN);
  gtk_widget_set_focusable (GTK_WIDGET (self), TRUE);
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
  g_return_val_if_fail (SHUMATE_IS_MAP (self), NULL);

  return self->viewport;
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
  g_return_if_fail (SHUMATE_IS_MAP (self));

  shumate_location_set_location (SHUMATE_LOCATION (self->viewport), latitude, longitude);
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
  g_return_if_fail (SHUMATE_IS_MAP (self));

  if (self->goto_context == NULL)
    return;

  gtk_widget_remove_tick_callback (GTK_WIDGET (self), self->goto_context->tick_id);
  g_clear_pointer (&self->goto_context, g_free);

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
  double zoom_level;

  g_return_if_fail (SHUMATE_IS_MAP (self));
  g_return_if_fail (latitude >= SHUMATE_MIN_LATITUDE && latitude <= SHUMATE_MAX_LATITUDE);
  g_return_if_fail (longitude >= SHUMATE_MIN_LONGITUDE && longitude <= SHUMATE_MAX_LONGITUDE);

  zoom_level = shumate_viewport_get_zoom_level (self->viewport);

  shumate_map_go_to_full (self, latitude, longitude, zoom_level);
}


/**
 * shumate_map_go_to_full:
 * @self: a #ShumateMap
 * @latitude: the longitude to center the map at
 * @longitude: the longitude to center the map at
 * @zoom_level: the zoom level to end at
 *
 * Move from the current position to these coordinates and zoom to the given
 * zoom level. All tiles in the intermediate view WILL be loaded!
 */
void
shumate_map_go_to_full (ShumateMap *self,
                        double      latitude,
                        double      longitude,
                        double      zoom_level)
{
  guint duration;

  g_return_if_fail (SHUMATE_IS_MAP (self));
  g_return_if_fail (latitude >= SHUMATE_MIN_LATITUDE && latitude <= SHUMATE_MAX_LATITUDE);
  g_return_if_fail (longitude >= SHUMATE_MIN_LONGITUDE && longitude <= SHUMATE_MAX_LONGITUDE);

  duration = self->go_to_duration;
  if (duration == 0) /* calculate duration from zoom level */
    duration = 500 * zoom_level / 2.0;

  shumate_map_go_to_full_with_duration (self, latitude, longitude, zoom_level, duration);
}


/**
 * shumate_map_go_to_full_with_duration:
 * @self: a #ShumateMap
 * @latitude: the longitude to center the map at
 * @longitude: the longitude to center the map at
 * @zoom_level: the zoom level to end at
 * @duration_ms: animation duration in milliseconds
 *
 * Move from the current position to these coordinates and zoom to the given
 * zoom level. The given duration is used instead of the map's default [property@Map:go-to-duration].
 * All tiles in the intermediate view WILL be loaded!
 */
void
shumate_map_go_to_full_with_duration (ShumateMap *self,
                                      double      latitude,
                                      double      longitude,
                                      double      zoom_level,
                                      guint       duration_ms)
{
  double min_zoom, max_zoom;
  GoToContext *ctx;
  gboolean enable_animations;

  g_return_if_fail (SHUMATE_IS_MAP (self));
  g_return_if_fail (latitude >= SHUMATE_MIN_LATITUDE && latitude <= SHUMATE_MAX_LATITUDE);
  g_return_if_fail (longitude >= SHUMATE_MIN_LONGITUDE && longitude <= SHUMATE_MAX_LONGITUDE);

  g_object_get (gtk_widget_get_settings (GTK_WIDGET (self)),
                "gtk-enable-animations", &enable_animations,
                NULL);

  if (!enable_animations || duration_ms == 0)
    {
      shumate_map_center_on (self, latitude, longitude);
      shumate_viewport_set_zoom_level (self->viewport, zoom_level);
      return;
    }

  shumate_map_stop_go_to (self);

  min_zoom = shumate_viewport_get_min_zoom_level (self->viewport);
  max_zoom = shumate_viewport_get_max_zoom_level (self->viewport);

  ctx = g_new (GoToContext, 1);
  ctx->start_us = g_get_monotonic_time ();
  ctx->duration_us = ms_to_us (duration_ms);
  ctx->from_latitude = shumate_location_get_latitude (SHUMATE_LOCATION (self->viewport));
  ctx->from_longitude = shumate_location_get_longitude (SHUMATE_LOCATION (self->viewport));
  ctx->from_zoom = CLAMP (shumate_viewport_get_zoom_level (self->viewport), min_zoom, max_zoom);
  ctx->to_latitude = latitude;
  ctx->to_longitude = longitude;
  ctx->to_zoom = CLAMP (zoom_level, min_zoom, max_zoom);
  ctx->zoom_animation = FALSE;

  self->goto_context = ctx;

  ctx->tick_id = gtk_widget_add_tick_callback (GTK_WIDGET (self), go_to_tick_cb, NULL, NULL);
}


/**
 * shumate_map_get_go_to_duration:
 * @self: a #ShumateMap
 *
 * Get the 'go-to-duration' property.
 *
 * Returns: the animation duration when calling [method@Map.go_to],
 *   in milliseconds.
 */
guint
shumate_map_get_go_to_duration (ShumateMap *self)
{
  g_return_val_if_fail (SHUMATE_IS_MAP (self), 0);

  return self->go_to_duration;
}

/**
 * shumate_map_set_go_to_duration:
 * @self: a #ShumateMap
 * @duration: the animation duration, in milliseconds
 *
 * Set the duration of the transition of [method@Map.go_to].
 */
void
shumate_map_set_go_to_duration (ShumateMap *self,
                                guint       duration)
{
  g_return_if_fail (SHUMATE_IS_MAP (self));

  if (self->go_to_duration == duration)
    return;

  self->go_to_duration = duration;
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
  ShumateMapSource *ref_map_source;

  g_return_if_fail (SHUMATE_IS_MAP (self));
  g_return_if_fail (SHUMATE_IS_MAP_SOURCE (source));

  ref_map_source = shumate_viewport_get_reference_map_source (self->viewport);
  if (ref_map_source == source)
    return;

  shumate_viewport_set_reference_map_source (self->viewport, source);
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
  g_return_if_fail (SHUMATE_IS_MAP (self));

  self->zoom_on_double_click = value;
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
  g_return_if_fail (SHUMATE_IS_MAP (self));

  self->animate_zoom = value;
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
  g_return_val_if_fail (SHUMATE_IS_MAP (self), FALSE);

  return self->zoom_on_double_click;
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
  g_return_val_if_fail (SHUMATE_IS_MAP (self), FALSE);

  return self->animate_zoom;
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
  g_return_val_if_fail (SHUMATE_IS_MAP (self), SHUMATE_STATE_NONE);

  return self->state;
}

static void
zoom (ShumateMap *self,
      gboolean    zoom_out)
{
  double amount = (zoom_out ? -.2 : .2);

  /* If there is an ongoing animation, add to it rather than starting a new animation from the current position */
  if (self->goto_context != NULL && self->goto_context->zoom_animation)
    {
      shumate_map_go_to_full_with_duration (self,
                                            self->goto_context->to_latitude,
                                            self->goto_context->to_longitude,
                                            self->goto_context->to_zoom + amount,
                                            ZOOM_ANIMATION_MS);
    }
  else
    {
      double zoom_level = shumate_viewport_get_zoom_level (self->viewport);
      shumate_map_go_to_full_with_duration (self,
                                            shumate_location_get_latitude (SHUMATE_LOCATION (self->viewport)),
                                            shumate_location_get_longitude (SHUMATE_LOCATION (self->viewport)),
                                            roundf ((zoom_level + amount) * 5) / 5,
                                            self->animate_zoom ? ZOOM_ANIMATION_MS : 0);
    }

  if (self->goto_context != NULL)
    self->goto_context->zoom_animation = TRUE;
}

/**
 * shumate_map_zoom_in:
 * @self: a [class@Map]
 *
 * Zooms the map in. If [property@Map:animate-zoom] is `TRUE`, the change will be animated.
 */
void
shumate_map_zoom_in (ShumateMap *self)
{
  g_return_if_fail (SHUMATE_IS_MAP (self));
  zoom (self, FALSE);
}

/**
 * shumate_map_zoom_out:
 * @self: a [class@Map]
 *
 * Zooms the map out. If [property@Map:animate-zoom] is `TRUE`, the change will be animated.
 */
void
shumate_map_zoom_out (ShumateMap *self)
{
  g_return_if_fail (SHUMATE_IS_MAP (self));
  zoom (self, TRUE);
}
