/*
 * Copyright (C) 2008-2009 Pierre-Luc Beaudoin <pierre-luc@pierlux.com>
 * Copyright (C) 2010-2013 Jiri Techet <techet@gmail.com>
 * Copyright (C) 2012 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2019 Marcus Lundblad <ml@update.uu.se>
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
#include "shumate-cairo-exportable.h"
#include "shumate-defines.h"
#include "shumate-enum-types.h"
#include "shumate-marshal.h"
#include "shumate-map-source.h"
#include "shumate-map-source-factory.h"
#include "shumate-private.h"
#include "shumate-tile.h"
#include "shumate-license.h"

#include <clutter/clutter.h>
#include <glib.h>
#include <glib-object.h>
#include <math.h>
#include <shumate-kinetic-scroll-view.h>
#include <shumate-viewport.h>
#include <shumate-adjustment.h>

/* #define VIEW_LOG */
#ifdef VIEW_LOG
#define DEBUG_LOG() g_print ("%s\n", __FUNCTION__);
#else
#define DEBUG_LOG()
#endif

enum
{
  /* normal signals */
  ANIMATION_COMPLETED,
  LAYER_RELOCATED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_LONGITUDE,
  PROP_LATITUDE,
  PROP_ZOOM_LEVEL,
  PROP_MIN_ZOOM_LEVEL,
  PROP_MAX_ZOOM_LEVEL,
  PROP_MAP_SOURCE,
  PROP_DECELERATION,
  PROP_KINETIC_MODE,
  PROP_KEEP_CENTER_ON_RESIZE,
  PROP_ZOOM_ON_DOUBLE_CLICK,
  PROP_ANIMATE_ZOOM,
  PROP_STATE,
  PROP_BACKGROUND_PATTERN,
  PROP_GOTO_ANIMATION_MODE,
  PROP_GOTO_ANIMATION_DURATION,
  PROP_WORLD,
  PROP_HORIZONTAL_WRAP
};

#define PADDING 10
static guint signals[LAST_SIGNAL] = { 0, };

#define GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), SHUMATE_TYPE_VIEW, ShumateViewPrivate))

#define ZOOM_LEVEL_OUT_OF_RANGE(priv, level) \
  (level < priv->min_zoom_level || \
           level > priv->max_zoom_level || \
   level < shumate_map_source_get_min_zoom_level (priv->map_source) || \
           level > shumate_map_source_get_max_zoom_level (priv->map_source))

/* Between state values for go_to */
typedef struct
{
  ShumateView *view;
  ClutterTimeline *timeline;
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


struct _ShumateViewPrivate
{
                                /* ShumateView */
  ClutterActor *kinetic_scroll;     /* kinetic_scroll */
  ClutterActor *viewport;               /* viewport */
  ClutterActor *viewport_container;         /* viewport_container */
  ClutterActor *background_layer;               /* background_layer */
  ClutterActor *zoom_layer;                     /* zoom_layer */
  ClutterActor *map_layer;                      /* map_layer */
                                                /* map_layer clones */
  ClutterActor *user_layers;                    /* user_layers and clones */
  ClutterActor *zoom_overlay_actor; /* zoom_overlay_actor */
  ClutterActor *license_actor;      /* license_actor */

  ClutterContent *background_content;

  gboolean hwrap;
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

  ShumateMapSource *map_source; /* Current map tile source */
  GList *overlay_sources;

  guint zoom_level; /* Holds the current zoom level number */
  guint min_zoom_level; /* Lowest allowed zoom level */
  guint max_zoom_level; /* Highest allowed zoom level */

  /* Represents the (lat, lon) at the center of the viewport */
  gdouble longitude;
  gdouble latitude;
  gboolean location_updated;

  gint bg_offset_x;
  gint bg_offset_y;

  gboolean keep_center_on_resize;
  gboolean zoom_on_double_click;
  gboolean animate_zoom;

  gboolean kinetic_mode;

  ShumateState state; /* View's global state */

  /* shumate_view_go_to's context, kept for stop_go_to */
  GoToContext *goto_context;

  gint tiles_loading;

  guint redraw_timeout;
  guint zoom_timeout;

  ClutterAnimationMode goto_mode;
  guint goto_duration;

  gboolean animating_zoom;
  guint anim_start_zoom_level;
  gdouble zoom_actor_viewport_x;
  gdouble zoom_actor_viewport_y;
  guint zoom_actor_timeout;

  GHashTable *tile_map;

  gint tile_x_first;
  gint tile_y_first;
  gint tile_x_last;
  gint tile_y_last;

  /* Zoom gesture */
  ClutterGestureAction *zoom_gesture;
  guint initial_gesture_zoom;
  gdouble focus_lat;
  gdouble focus_lon;
  gboolean zoom_started;
  gdouble accumulated_scroll_dy;

  ShumateBoundingBox *world_bbox;

  GHashTable *visible_tiles;
};

G_DEFINE_TYPE (ShumateView, shumate_view, CLUTTER_TYPE_ACTOR);

static void exclusive_destroy_clone (ClutterActor *clone);
static void update_clones (ShumateView *view);
static gboolean scroll_event (ClutterActor *actor,
    ClutterScrollEvent *event,
    ShumateView *view);
static void resize_viewport (ShumateView *view);
static void shumate_view_get_property (GObject *object,
    guint prop_id,
    GValue *value,
    GParamSpec *pspec);
static void shumate_view_set_property (GObject *object,
    guint prop_id,
    const GValue *value,
    GParamSpec *pspec);
static void shumate_view_dispose (GObject *object);
static void shumate_view_class_init (ShumateViewClass *shumateViewClass);
static void shumate_view_init (ShumateView *view);
static void viewport_pos_changed_cb (GObject *gobject,
    GParamSpec *arg1,
    ShumateView *view);
static gboolean kinetic_scroll_button_press_cb (ClutterActor *actor,
    ClutterButtonEvent *event,
    ShumateView *view);
static ClutterActor *sample_user_layer_at_pos (ShumateView *view,
    gfloat x,
    gfloat y);
static void swap_user_layer_slots (ShumateView *view,
    gint original_index,
    gint clone_index);
static gboolean viewport_motion_cb (ClutterActor *actor,
    ClutterMotionEvent *event,
    ShumateView *view);
static gboolean viewport_press_cb (ClutterActor *actor,
    ClutterButtonEvent *event,
    ShumateView *view);
static void load_visible_tiles (ShumateView *view,
    gboolean relocate);
static gboolean view_set_zoom_level_at (ShumateView *view,
    guint zoom_level,
    gboolean use_event_coord,
    gint x,
    gint y);
static void tile_state_notify (ShumateTile *tile,
    G_GNUC_UNUSED GParamSpec *pspec,
    ShumateView *view);
static gboolean kinetic_scroll_key_press_cb (ShumateView *view,
    ClutterKeyEvent *event);
static void shumate_view_go_to_with_duration (ShumateView *view,
    gdouble latitude,
    gdouble longitude,
    guint duration);
static gboolean redraw_timeout_cb(gpointer view);
static void remove_all_tiles (ShumateView *view);
static void get_x_y_for_zoom_level (ShumateView *view,
    guint zoom_level,
    gint offset_x,
    gint offset_y,
    gdouble *new_x,
    gdouble *new_y);
static ShumateBoundingBox *get_bounding_box (ShumateView *view,
    guint zoom_level,
    gdouble x,
    gdouble y);
static void get_tile_bounds (ShumateView *view,
    guint *min_x,
    guint *min_y,
    guint *max_x,
    guint *max_y);
static gboolean tile_in_tile_table (ShumateView *view,
    GHashTable *table,
    gint tile_x,
    gint tile_y);

static gdouble
x_to_wrap_x (gdouble x, gdouble width)
{
  if (x < 0)
    x += ((gint)-x / (gint)width + 1) * width;

  return fmod (x, width);
}


static gint
get_map_width (ShumateView *view)
{
  gint size, cols;
  ShumateViewPrivate *priv = view->priv;

  size = shumate_map_source_get_tile_size (priv->map_source);
  cols = shumate_map_source_get_column_count (priv->map_source,
                                                priv->zoom_level);
  return size * cols;
}


static gdouble
get_longitude (ShumateView *view,
    guint zoom_level,
    gdouble x)
{
  ShumateViewPrivate *priv = view->priv;

  DEBUG_LOG ()

  g_return_val_if_fail (SHUMATE_IS_VIEW (view), 0.0);

  if (priv->hwrap)
    x = x_to_wrap_x (x, get_map_width (view));

  return shumate_map_source_get_longitude (priv->map_source,
        zoom_level,
        x);
}


static void
update_coords (ShumateView *view,
    gdouble x,
    gdouble y,
    gboolean notify)
{
  DEBUG_LOG ()

  ShumateViewPrivate *priv = view->priv;

  priv->viewport_x = x;
  priv->viewport_y = y;
  priv->longitude = get_longitude (view,
        priv->zoom_level,
        x + priv->viewport_width / 2.0);
  priv->latitude = shumate_map_source_get_latitude (priv->map_source,
        priv->zoom_level,
        y + priv->viewport_height / 2.0);

  if (notify)
    {
      g_object_notify (G_OBJECT (view), "longitude");
      g_object_notify (G_OBJECT (view), "latitude");
    }
}


static void
position_viewport (ShumateView *view,
    gdouble x,
    gdouble y)
{
  DEBUG_LOG ()

  ShumateViewPrivate *priv = view->priv;
  gint old_bg_offset_x = 0, old_bg_offset_y = 0;
  gfloat bg_width, bg_height;

  /* remember the relative offset of the background tile */
  if (priv->background_content)
    {
      clutter_content_get_preferred_size (priv->background_content, &bg_width, &bg_height);
      old_bg_offset_x = ((gint)priv->viewport_x + priv->bg_offset_x) % (gint)bg_width;
      old_bg_offset_y = ((gint)priv->viewport_y + priv->bg_offset_y) % (gint)bg_height;
    }

  /* notify about latitude and longitude change only after the viewport position is set */
  g_object_freeze_notify (G_OBJECT (view));

  update_coords (view, x, y, TRUE);

  /* compute the new relative offset of the background tile */
  if (priv->background_content)
    {
      gint new_bg_offset_x = (gint)priv->viewport_x % (gint)bg_width;
      gint new_bg_offset_y = (gint)priv->viewport_y % (gint)bg_height;
      priv->bg_offset_x = (old_bg_offset_x - new_bg_offset_x) % (gint)bg_width;
      priv->bg_offset_y = (old_bg_offset_y - new_bg_offset_y) % (gint)bg_height;
      if (priv->bg_offset_x < 0)
        priv->bg_offset_x += bg_width;
      if (priv->bg_offset_y < 0)
        priv->bg_offset_y += bg_height;
    }

  /* we know about the change already - don't send the notifications again */
  g_signal_handlers_block_by_func (priv->viewport, G_CALLBACK (viewport_pos_changed_cb), view);
  shumate_viewport_set_origin (SHUMATE_VIEWPORT (priv->viewport),
      (gint)priv->viewport_x,
      (gint)priv->viewport_y);
  g_signal_handlers_unblock_by_func (priv->viewport, G_CALLBACK (viewport_pos_changed_cb), view);

  g_object_thaw_notify (G_OBJECT (view));
}


static void
view_relocated_cb (G_GNUC_UNUSED ShumateViewport *viewport,
    ShumateView *view)
{
  ShumateViewPrivate *priv = view->priv;
  gint anchor_x, anchor_y, new_width, new_height;
  gint tile_size, column_count, row_count;

  clutter_actor_destroy_all_children (priv->zoom_layer);
  load_visible_tiles (view, TRUE);
  g_signal_emit_by_name (view, "layer-relocated", NULL);

  /* Clutter clones need their source actor to have an explicitly set size to display properly */
  tile_size = shumate_map_source_get_tile_size (priv->map_source);
  column_count = shumate_map_source_get_column_count (priv->map_source, priv->zoom_level);
  row_count = shumate_map_source_get_row_count (priv->map_source, priv->zoom_level);
  shumate_viewport_get_anchor (SHUMATE_VIEWPORT (priv->viewport), &anchor_x, &anchor_y);

  /* The area containing tiles in the map layer is actually column_count * tile_size wide (same
   * for height), but the viewport anchor acts as an offset for the tile actors, causing the map
   * layer to contain some empty space as well.
   */
  new_width = column_count * tile_size + anchor_x;
  new_height = row_count * tile_size + anchor_y;

  clutter_actor_set_size (priv->map_layer, new_width, new_height);
}


static void
panning_completed (G_GNUC_UNUSED ShumateKineticScrollView *scroll,
    ShumateView *view)
{
  DEBUG_LOG ()

  ShumateViewPrivate *priv = view->priv;
  gdouble x, y;

  if (priv->redraw_timeout != 0)
    {
      g_source_remove (priv->redraw_timeout);
      priv->redraw_timeout = 0;
    }

  shumate_viewport_get_origin (SHUMATE_VIEWPORT (priv->viewport), &x, &y);

  update_coords (view, x, y, TRUE);
  load_visible_tiles (view, FALSE);
}


static gboolean
zoom_timeout_cb (gpointer data)
{
  DEBUG_LOG ()

  ShumateView *view = data;
  ShumateViewPrivate *priv = view->priv;

  priv->accumulated_scroll_dy = 0;
  priv->zoom_timeout = 0;

  return FALSE;
}


static gboolean
scroll_event (G_GNUC_UNUSED ClutterActor *actor,
    ClutterScrollEvent *event,
    ShumateView *view)
{
  DEBUG_LOG ()

  ShumateViewPrivate *priv = view->priv;

  guint zoom_level = priv->zoom_level;

  if (event->direction == CLUTTER_SCROLL_UP)
    zoom_level = priv->zoom_level + 1;
  else if (event->direction == CLUTTER_SCROLL_DOWN)
    zoom_level = priv->zoom_level - 1;
  else if (event->direction == CLUTTER_SCROLL_SMOOTH)
    {
      gdouble dx, dy;
      gint steps;

      clutter_event_get_scroll_delta ((ClutterEvent *)event, &dx, &dy);

      priv->accumulated_scroll_dy += dy;
      /* add some small value to avoid missing step for values like 0.999999 */
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

  return view_set_zoom_level_at (view, zoom_level, TRUE, event->x, event->y);
}


static void
resize_viewport (ShumateView *view)
{
  DEBUG_LOG ()

  gdouble lower_x = 0;
  gdouble lower_y = 0;
  gdouble upper_x = G_MAXINT16;
  gdouble upper_y = G_MAXINT16;
  ShumateAdjustment *hadjust, *vadjust;
  guint min_x, min_y, max_x, max_y;

  ShumateViewPrivate *priv = view->priv;

  shumate_viewport_get_adjustments (SHUMATE_VIEWPORT (priv->viewport), &hadjust,
      &vadjust);

  get_tile_bounds (view, &min_x, &min_y, &max_x, &max_y);
  gint x_last = max_x * shumate_map_source_get_tile_size (priv->map_source);
  gint y_last = max_y * shumate_map_source_get_tile_size (priv->map_source);
  gint x_first = min_x * shumate_map_source_get_tile_size (priv->map_source);
  gint y_first = min_y * shumate_map_source_get_tile_size (priv->map_source);

  /* Location of viewport with respect to the first tile:
   *
   * - for large maps (higher zoom levels) we allow the map to end in the middle
   *   of the viewport; that is, one half of the viewport is positioned before
   *   the first tile
   * - for small maps (e.g. zoom level 0) we allow half of the map to go outside
   *   the viewport; that is, whole viewport except one half of the map is
   *   positioned before the first tile
   *
   * The first and the second element of the MIN() below corresponds to the
   * first and the second case above. */
  lower_x = MIN (x_first - priv->viewport_width / 2,
                 (x_first - priv->viewport_width) + (x_last - x_first) / 2);

  lower_y = MIN (y_first - priv->viewport_height / 2,
                 (y_first - priv->viewport_height) + (y_last - y_first) / 2);

  if (priv->hwrap)
    upper_x = MAX (x_last - x_first + priv->viewport_width / 2, priv->viewport_width + (x_last - x_first) / 2);
  else
    upper_x = MAX (x_last - priv->viewport_width / 2, (x_last - x_first) / 2);
  upper_y = MAX (y_last - priv->viewport_height / 2, (y_last - y_first)/ 2);

  /* we don't want to get notified about the position change now */
  g_signal_handlers_block_by_func (priv->viewport, G_CALLBACK (viewport_pos_changed_cb), view);
  shumate_adjustment_set_values (hadjust, shumate_adjustment_get_value (hadjust), lower_x, upper_x, 1.0);
  shumate_adjustment_set_values (vadjust, shumate_adjustment_get_value (vadjust), lower_y, upper_y, 1.0);
  g_signal_handlers_unblock_by_func (priv->viewport, G_CALLBACK (viewport_pos_changed_cb), view);
}

static void
shumate_view_get_property (GObject *object,
    guint prop_id,
    GValue *value,
    GParamSpec *pspec)
{
  DEBUG_LOG ()

  ShumateView *view = SHUMATE_VIEW (object);
  ShumateViewPrivate *priv = view->priv;

  switch (prop_id)
    {
    case PROP_LONGITUDE:
      g_value_set_double (value,
          CLAMP (priv->longitude, priv->world_bbox->left, priv->world_bbox->right));
      break;

    case PROP_LATITUDE:
      g_value_set_double (value,
          CLAMP (priv->latitude, priv->world_bbox->bottom, priv->world_bbox->top));
      break;

    case PROP_ZOOM_LEVEL:
      g_value_set_uint (value, priv->zoom_level);
      break;

    case PROP_MIN_ZOOM_LEVEL:
      g_value_set_uint (value, priv->min_zoom_level);
      break;

    case PROP_MAX_ZOOM_LEVEL:
      g_value_set_uint (value, priv->max_zoom_level);
      break;

    case PROP_MAP_SOURCE:
      g_value_set_object (value, priv->map_source);
      break;

    case PROP_KINETIC_MODE:
      g_value_set_boolean (value, priv->kinetic_mode);
      break;

    case PROP_DECELERATION:
      {
        gdouble decel = 0.0;
        g_object_get (priv->kinetic_scroll, "deceleration", &decel, NULL);
        g_value_set_double (value, decel);
        break;
      }

    case PROP_KEEP_CENTER_ON_RESIZE:
      g_value_set_boolean (value, priv->keep_center_on_resize);
      break;

    case PROP_ZOOM_ON_DOUBLE_CLICK:
      g_value_set_boolean (value, priv->zoom_on_double_click);
      break;

    case PROP_ANIMATE_ZOOM:
      g_value_set_boolean (value, priv->animate_zoom);
      break;

    case PROP_STATE:
      g_value_set_enum (value, priv->state);
      break;

    case PROP_BACKGROUND_PATTERN:
      g_value_set_object (value, priv->background_content);
      break;

    case PROP_GOTO_ANIMATION_MODE:
      g_value_set_enum (value, priv->goto_mode);
      break;

    case PROP_GOTO_ANIMATION_DURATION:
      g_value_set_uint (value, priv->goto_duration);
      break;

    case PROP_WORLD:
      g_value_set_boxed (value, priv->world_bbox);
      break;

    case PROP_HORIZONTAL_WRAP:
      g_value_set_boolean (value, shumate_view_get_horizontal_wrap (view));
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
  DEBUG_LOG ()

  ShumateView *view = SHUMATE_VIEW (object);
  ShumateViewPrivate *priv = view->priv;

  switch (prop_id)
    {
    case PROP_LONGITUDE:
      shumate_view_center_on (view, priv->latitude,
          g_value_get_double (value));
      break;

    case PROP_LATITUDE:
      shumate_view_center_on (view, g_value_get_double (value),
          priv->longitude);
      break;

    case PROP_ZOOM_LEVEL:
      shumate_view_set_zoom_level (view, g_value_get_uint (value));
      break;

    case PROP_MIN_ZOOM_LEVEL:
      shumate_view_set_min_zoom_level (view, g_value_get_uint (value));
      break;

    case PROP_MAX_ZOOM_LEVEL:
      shumate_view_set_max_zoom_level (view, g_value_get_uint (value));
      break;

    case PROP_MAP_SOURCE:
      shumate_view_set_map_source (view, g_value_get_object (value));
      break;

    case PROP_KINETIC_MODE:
      shumate_view_set_kinetic_mode (view, g_value_get_boolean (value));
      break;

    case PROP_DECELERATION:
      shumate_view_set_deceleration (view, g_value_get_double (value));
      break;

    case PROP_KEEP_CENTER_ON_RESIZE:
      shumate_view_set_keep_center_on_resize (view, g_value_get_boolean (value));
      break;

    case PROP_ZOOM_ON_DOUBLE_CLICK:
      shumate_view_set_zoom_on_double_click (view, g_value_get_boolean (value));
      break;

    case PROP_ANIMATE_ZOOM:
      shumate_view_set_animate_zoom (view, g_value_get_boolean (value));
      break;

    case PROP_BACKGROUND_PATTERN:
      shumate_view_set_background_pattern (view, g_value_get_object (value));
      break;

    case PROP_GOTO_ANIMATION_MODE:
      priv->goto_mode = g_value_get_enum (value);
      break;

    case PROP_GOTO_ANIMATION_DURATION:
      priv->goto_duration = g_value_get_uint (value);
      break;

    case PROP_WORLD:
      shumate_view_set_world (view, g_value_get_boxed (value));
      break;

    case PROP_HORIZONTAL_WRAP:
      shumate_view_set_horizontal_wrap (view, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}


static void
shumate_view_dispose (GObject *object)
{
  DEBUG_LOG ()

  ShumateView *view = SHUMATE_VIEW (object);
  ShumateViewPrivate *priv = view->priv;

  if (priv->goto_context != NULL)
    shumate_view_stop_go_to (view);

  if (priv->kinetic_scroll != NULL)
    {
      shumate_kinetic_scroll_view_stop (SHUMATE_KINETIC_SCROLL_VIEW (priv->kinetic_scroll));
      priv->kinetic_scroll = NULL;
    }

  if (priv->viewport != NULL)
    {
      shumate_viewport_stop (SHUMATE_VIEWPORT (priv->viewport));
      priv->viewport = NULL;
    }

  if (priv->map_source != NULL)
    {
      g_object_unref (priv->map_source);
      priv->map_source = NULL;
    }

  g_list_free_full (priv->overlay_sources, g_object_unref);
  priv->overlay_sources = NULL;

  if (priv->background_content)
    {
      g_object_unref (priv->background_content);
      priv->background_content = NULL;
    }

  if (priv->redraw_timeout != 0)
    {
      g_source_remove (priv->redraw_timeout);
      priv->redraw_timeout = 0;
    }

  if (priv->zoom_actor_timeout != 0)
    {
      g_source_remove (priv->zoom_actor_timeout);
      priv->zoom_actor_timeout = 0;
    }

  if (priv->zoom_timeout != 0)
    {
      g_source_remove (priv->zoom_timeout);
      priv->zoom_timeout = 0;
    }

  if (priv->tile_map != NULL)
    {
      g_hash_table_destroy (priv->tile_map);
      priv->tile_map = NULL;
    }

  if (priv->zoom_gesture)
    {
      clutter_actor_remove_action (CLUTTER_ACTOR (view),
                                   CLUTTER_ACTION (priv->zoom_gesture));
      priv->zoom_gesture = NULL;
    }

  if (priv->visible_tiles != NULL)
    {
      g_hash_table_destroy (priv->visible_tiles);
      priv->visible_tiles = NULL;
    }

  priv->map_layer = NULL;
  priv->license_actor = NULL;

  /* This is needed to prevent race condition see bug #760012 */
  if (priv->user_layers)
      clutter_actor_remove_all_children (priv->user_layers);
  priv->user_layers = NULL;
  priv->zoom_layer = NULL;

  if (priv->world_bbox)
    {
      shumate_bounding_box_free (priv->world_bbox);
      priv->world_bbox = NULL;
    }

  G_OBJECT_CLASS (shumate_view_parent_class)->dispose (object);
}


static void
shumate_view_finalize (GObject *object)
{
  DEBUG_LOG ()

/*  ShumateViewPrivate *priv = SHUMATE_VIEW (object)->priv; */

  G_OBJECT_CLASS (shumate_view_parent_class)->finalize (object);
}


/* These return fixed sizes because either a.) We expect the user to size
 * explicitly with clutter_actor_get_size or b.) place it in a container that
 * allocates it whatever it wants.
 */
static void
shumate_view_get_preferred_width (ClutterActor *actor,
    G_GNUC_UNUSED gfloat for_height,
    gfloat *min_width,
    gfloat *nat_width)
{
  DEBUG_LOG ()

  ShumateView *view = SHUMATE_VIEW (actor);
  gint width = shumate_map_source_get_tile_size (view->priv->map_source);

  if (min_width)
    *min_width = 1;

  if (nat_width)
    *nat_width = width;
}


static void
shumate_view_get_preferred_height (ClutterActor *actor,
    G_GNUC_UNUSED gfloat for_width,
    gfloat *min_height,
    gfloat *nat_height)
{
  DEBUG_LOG ()

  ShumateView *view = SHUMATE_VIEW (actor);
  gint height = shumate_map_source_get_tile_size (view->priv->map_source);

  if (min_height)
    *min_height = 1;

  if (nat_height)
    *nat_height = height;
}


static void
shumate_view_class_init (ShumateViewClass *shumateViewClass)
{
  DEBUG_LOG ()

  g_type_class_add_private (shumateViewClass, sizeof (ShumateViewPrivate));

  GObjectClass *object_class = G_OBJECT_CLASS (shumateViewClass);
  object_class->dispose = shumate_view_dispose;
  object_class->finalize = shumate_view_finalize;
  object_class->get_property = shumate_view_get_property;
  object_class->set_property = shumate_view_set_property;

  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (shumateViewClass);
  actor_class->get_preferred_width = shumate_view_get_preferred_width;
  actor_class->get_preferred_height = shumate_view_get_preferred_height;

  /**
   * ShumateView:longitude:
   *
   * The longitude coordonate of the map
   */
  g_object_class_install_property (object_class,
      PROP_LONGITUDE,
      g_param_spec_double ("longitude",
          "Longitude",
          "The longitude coordonate of the map",
          -180.0f,
          180.0f,
          0.0f,
          SHUMATE_PARAM_READWRITE));

  /**
   * ShumateView:latitude:
   *
   * The latitude coordonate of the map
   */
  g_object_class_install_property (object_class,
      PROP_LATITUDE,
      g_param_spec_double ("latitude",
          "Latitude",
          "The latitude coordonate of the map",
          -90.0f,
          90.0f,
          0.0f,
          SHUMATE_PARAM_READWRITE));

  /**
   * ShumateView:zoom-level:
   *
   * The level of zoom of the content.
   */
  g_object_class_install_property (object_class,
      PROP_ZOOM_LEVEL,
      g_param_spec_uint ("zoom-level",
          "Zoom level",
          "The level of zoom of the map",
          0,
          20,
          3,
          SHUMATE_PARAM_READWRITE));

  /**
   * ShumateView:min-zoom-level:
   *
   * The lowest allowed level of zoom of the content.
   */
  g_object_class_install_property (object_class,
      PROP_MIN_ZOOM_LEVEL,
      g_param_spec_uint ("min-zoom-level",
          "Min zoom level",
          "The lowest allowed level of zoom",
          0,
          20,
          0,
          SHUMATE_PARAM_READWRITE));

  /**
   * ShumateView:max-zoom-level:
   *
   * The highest allowed level of zoom of the content.
   */
  g_object_class_install_property (object_class,
      PROP_MAX_ZOOM_LEVEL,
      g_param_spec_uint ("max-zoom-level",
          "Max zoom level",
          "The highest allowed level of zoom",
          0,
          20,
          20,
          SHUMATE_PARAM_READWRITE));

  /**
   * ShumateView:map-source:
   *
   * The #ShumateMapSource being displayed
   */
  g_object_class_install_property (object_class,
      PROP_MAP_SOURCE,
      g_param_spec_object ("map-source",
          "Map source",
          "The map source being displayed",
          SHUMATE_TYPE_MAP_SOURCE,
          SHUMATE_PARAM_READWRITE));

  /**
   * ShumateView:kinetic-mode:
   *
   * Determines whether the view should use kinetic mode.
   */
  g_object_class_install_property (object_class,
      PROP_KINETIC_MODE,
      g_param_spec_boolean ("kinetic-mode",
          "Kinetic Mode",
          "Determines whether the view should use kinetic mode.",
          FALSE,
          SHUMATE_PARAM_READWRITE));

  /**
   * ShumateView:deceleration:
   *
   * The deceleration rate for the kinetic mode. The default value is 1.1.
   */
  g_object_class_install_property (object_class,
      PROP_DECELERATION,
      g_param_spec_double ("deceleration",
          "Deceleration rate",
          "Rate at which the view will decelerate in kinetic mode.",
          1.0001,
          2.0,
          1.1,
          SHUMATE_PARAM_READWRITE));

  /**
   * ShumateView:keep-center-on-resize:
   *
   * Keep the current centered position when resizing the view.
   */
  g_object_class_install_property (object_class,
      PROP_KEEP_CENTER_ON_RESIZE,
      g_param_spec_boolean ("keep-center-on-resize",
          "Keep center on resize",
          "Keep the current centered position upon resizing",
          TRUE,
          SHUMATE_PARAM_READWRITE));

  /**
   * ShumateView:zoom-on-double-click:
   *
   * Should the view zoom in and recenter when the user double click on the map.
   */
  g_object_class_install_property (object_class,
      PROP_ZOOM_ON_DOUBLE_CLICK,
      g_param_spec_boolean ("zoom-on-double-click",
          "Zoom in on double click",
          "Zoom in and recenter on double click on the map",
          TRUE,
          SHUMATE_PARAM_READWRITE));

  /**
   * ShumateView:animate-zoom:
   *
   * Animate zoom change when zooming in/out.
   */
  g_object_class_install_property (object_class,
      PROP_ANIMATE_ZOOM,
      g_param_spec_boolean ("animate-zoom",
          "Animate zoom level change",
          "Animate zoom change when zooming in/out",
          TRUE,
          SHUMATE_PARAM_READWRITE));

  /**
   * ShumateView:state:
   *
   * The view's global state. Useful to inform using if the view is busy loading
   * tiles or not.
   */
  g_object_class_install_property (object_class,
      PROP_STATE,
      g_param_spec_enum ("state",
          "View's state",
          "View's global state",
          SHUMATE_TYPE_STATE,
          SHUMATE_STATE_NONE,
          G_PARAM_READABLE));

  /**
   * ShumateView:background-pattern:
   *
   * The pattern displayed in the background of the map.
   */
  g_object_class_install_property (object_class,
      PROP_BACKGROUND_PATTERN,
      g_param_spec_object ("background-pattern",
          "Background pattern",
          "The tile's background pattern",
          CLUTTER_TYPE_ACTOR,
          G_PARAM_READWRITE));

  /**
   * ShumateView:goto-animation-mode:
   *
   * The mode of animation when going to a location.
   *
   * Please note that animation of #shumate_view_ensure_visible also
   * involves a 'goto' animation.
   *
   */
  g_object_class_install_property (object_class,
      PROP_GOTO_ANIMATION_MODE,
      g_param_spec_enum ("goto-animation-mode",
          "Go to animation mode",
          "The mode of animation when going to a location",
          CLUTTER_TYPE_ANIMATION_MODE,
          CLUTTER_EASE_IN_OUT_CIRC,
          G_PARAM_READWRITE));

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
  g_object_class_install_property (object_class,
      PROP_GOTO_ANIMATION_DURATION,
      g_param_spec_uint ("goto-animation-duration",
          "Go to animation duration",
          "The duration of an animation when going to a location",
          0,
          G_MAXINT,
          0,
          G_PARAM_READWRITE));

  /**
   * ShumateView:world:
   *
   * Set a bounding box to limit the world to. No tiles will be loaded
   * outside of this bounding box. It will not be possible to scroll outside
   * of this bounding box.
   *
   * Default world is the actual world.
   */
  g_object_class_install_property (object_class,
      PROP_WORLD,
      g_param_spec_boxed ("world",
          "The world",
          "The bounding box to limit the #ShumateView to",
          SHUMATE_TYPE_BOUNDING_BOX,
          G_PARAM_READWRITE));

  /**
   * ShumateView:horizontal-wrap:
   *
   * Determines whether the view should wrap horizontally.
   *
   */
  g_object_class_install_property (object_class,
      PROP_HORIZONTAL_WRAP,
      g_param_spec_boolean ("horizontal-wrap",
          "Horizontal wrap",
          "Determines whether the view should wrap horizontally.",
          FALSE,
          SHUMATE_PARAM_READWRITE));

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
}


static void
shumate_view_realized_cb (ShumateView *view,
    G_GNUC_UNUSED GParamSpec *pspec)
{
  DEBUG_LOG ()

  ShumateViewPrivate *priv = view->priv;

  if (!CLUTTER_ACTOR_IS_REALIZED (view))
    return;

  clutter_actor_grab_key_focus (priv->kinetic_scroll);

  resize_viewport (view);
  shumate_view_center_on (view, priv->latitude, priv->longitude);

  g_object_notify (G_OBJECT (view), "zoom-level");
  g_object_notify (G_OBJECT (view), "map-source");
  g_signal_emit_by_name (view, "layer-relocated", NULL);
}


static gboolean
_update_idle_cb (ShumateView *view)
{
  DEBUG_LOG ()

  ShumateViewPrivate *priv = view->priv;

  if (!priv->kinetic_scroll)
    return FALSE;

  clutter_actor_set_size (priv->kinetic_scroll,
      priv->viewport_width,
      priv->viewport_height);

  resize_viewport (view);

  if (priv->keep_center_on_resize)
    shumate_view_center_on (view, priv->latitude, priv->longitude);
  else
    load_visible_tiles (view, FALSE);

  if (priv->hwrap)
    {
      update_clones (view);
      position_viewport (view, x_to_wrap_x (priv->viewport_x, get_map_width (view)), priv->viewport_y);
    }

  return FALSE;
}


static void
view_size_changed_cb (ShumateView *view,
    G_GNUC_UNUSED GParamSpec *pspec)
{
  ShumateViewPrivate *priv = GET_PRIVATE (view);
  gint width, height;

  width = clutter_actor_get_width (CLUTTER_ACTOR (view));
  height = clutter_actor_get_height (CLUTTER_ACTOR (view));

  if (priv->viewport_width != width || priv->viewport_height != height)
    {
      g_idle_add_full (CLUTTER_PRIORITY_REDRAW,
          (GSourceFunc) _update_idle_cb,
          g_object_ref (view),
          (GDestroyNotify) g_object_unref);
    }

  priv->viewport_width = width;
  priv->viewport_height = height;
}

static void
exclusive_destroy_clone (ClutterActor *clone)
{
  if (!CLUTTER_IS_CLONE (clone))
    return;

  clutter_actor_destroy (clone);
}

static void
add_clone (ShumateView *view,
    gint x)
{
  ShumateViewPrivate *priv = view->priv;
  ClutterActor *map_clone, *user_clone;

  /* Map layer clones */
  map_clone = clutter_clone_new (priv->map_layer);
  clutter_actor_set_x (map_clone, x);
  clutter_actor_insert_child_below (priv->viewport_container, map_clone,
                                    NULL);

  priv->map_clones = g_list_prepend (priv->map_clones, map_clone);

  /* User layer clones */
  user_clone = clutter_clone_new (priv->user_layers);
  clutter_actor_set_x (user_clone, x);
  clutter_actor_insert_child_below (priv->viewport_container, user_clone,
                                    priv->user_layers);

  /* Inserting clones in the slots following the real user layer (order must be kept)*/
  priv->user_layer_slots = g_list_append (priv->user_layer_slots, user_clone);
}


static void
update_clones (ShumateView *view)
{
  DEBUG_LOG ()

  ShumateViewPrivate *priv = view->priv;
  gint map_size;
  gfloat view_width;
  gint i;

  map_size = get_map_width (view);
  clutter_actor_get_size (CLUTTER_ACTOR (view), &view_width, NULL);

  priv->num_right_clones = ceil (view_width / map_size) + 1;

  if (priv->map_clones != NULL)
    {
      /* Only destroy clones, skip the real user_layers actor */
      g_list_free_full (priv->user_layer_slots, (GDestroyNotify) exclusive_destroy_clone);
      g_list_free_full (priv->map_clones, (GDestroyNotify) clutter_actor_destroy);

      priv->map_clones = NULL;
      priv->user_layer_slots = NULL;
    }

  /* An extra clone is added to the left for smoother panning */
  add_clone (view, -map_size);

  /* Inserting the real user layer in the second slot (after the left clone) */
  priv->user_layer_slots = g_list_append (priv->user_layer_slots, priv->user_layers);
  clutter_actor_set_x (priv->user_layers, 0);

  for (i = 0; i < priv->num_right_clones; i++)
    add_clone (view, (i + 1) * map_size);
}


static void
slice_free_gint64 (gpointer data)
{
  g_slice_free (gint64, data);
}


static guint
view_find_suitable_zoom (ShumateView *view,
    gdouble factor)
{
  ShumateViewPrivate *priv = GET_PRIVATE (view);
  guint zoom_level = priv->initial_gesture_zoom;

  while (factor > 2 && zoom_level <= priv->max_zoom_level)
    {
      factor /= 2;
      zoom_level++;
    }

  while (factor < 0.5 && zoom_level >= priv->min_zoom_level)
    {
      factor *= 2;
      zoom_level--;
    }

  return zoom_level;
}


static gboolean
zoom_gesture_zoom_cb (ClutterZoomAction *gesture,
    G_GNUC_UNUSED ClutterActor *actor,
    ClutterPoint *focal_point,
    gdouble factor,
    gpointer user_data)
{
  ShumateView *view = user_data;
  ShumateViewPrivate *priv = GET_PRIVATE (view);
  gdouble dx, dy, lat, lon;
  ClutterPoint focus;

  if (!priv->zoom_started)
    {
      priv->zoom_started = TRUE;
      priv->focus_lat = shumate_view_y_to_latitude (user_data, focal_point->y);
      priv->focus_lon = shumate_view_x_to_longitude (user_data, focal_point->x);
      priv->initial_gesture_zoom = priv->zoom_level;
    }
  else
    {
      guint zoom_level;

      zoom_level = view_find_suitable_zoom (view, factor);

      focus.x = shumate_map_source_get_x (priv->map_source,
                                            zoom_level, priv->focus_lon);
      focus.y = shumate_map_source_get_y (priv->map_source,
                                            zoom_level, priv->focus_lat);

      dx = (priv->viewport_width / 2.0) - focal_point->x;
      dy = (priv->viewport_height / 2.0) - focal_point->y;

      lon = get_longitude (view, zoom_level, focus.x + dx);
      lat = shumate_map_source_get_latitude (priv->map_source, zoom_level, focus.y + dy);

      shumate_view_center_on (view, lat, lon);
      shumate_view_set_zoom_level (view, zoom_level);
    }

  return FALSE;
}

static gboolean
zoom_gesture_begin_cb (ClutterGestureAction *gesture,
    G_GNUC_UNUSED ClutterActor *actor,
    G_GNUC_UNUSED gpointer user_data)
{
  ClutterEvent *event = clutter_gesture_action_get_last_event (gesture, 0);
  ClutterInputDevice *device = clutter_event_get_source_device (event);

  /* Give up on >2 finger input and when using mouse */
  return clutter_gesture_action_get_n_current_points (gesture) == 2 &&
    clutter_input_device_get_device_type (device) != CLUTTER_POINTER_DEVICE;
}


static void
zoom_gesture_finish_cb (ClutterGestureAction *gesture,
    G_GNUC_UNUSED ClutterActor *actor,
    gpointer user_data)
{
  ShumateViewPrivate *priv = GET_PRIVATE (user_data);

  priv->zoom_started = FALSE;
}


static void
zoom_gesture_cancel_cb (ClutterGestureAction *gesture,
    G_GNUC_UNUSED ClutterActor *actor,
    gpointer user_data)
{
  ShumateViewPrivate *priv = GET_PRIVATE (user_data);

  priv->zoom_started = FALSE;
  g_signal_stop_emission_by_name (gesture, "gesture-cancel");
}


static void
shumate_view_init (ShumateView *view)
{
  DEBUG_LOG ()

  ShumateViewPrivate *priv = GET_PRIVATE (view);
  ShumateMapSourceFactory *factory;
  ShumateMapSource *source;
  ClutterLayoutManager *layout;
  ClutterColor color = { 0xf1, 0xee, 0xe8, 0xff };

  shumate_debug_set_flags (g_getenv ("SHUMATE_DEBUG"));

  view->priv = priv;

  factory = shumate_map_source_factory_dup_default ();
  source = shumate_map_source_factory_create_cached_source (factory, SHUMATE_MAP_SOURCE_OSM_MAPNIK);

  priv->map_source = SHUMATE_MAP_SOURCE (g_object_ref_sink (source));

  priv->zoom_level = 0;
  priv->min_zoom_level = shumate_map_source_get_min_zoom_level (priv->map_source);
  priv->max_zoom_level = shumate_map_source_get_max_zoom_level (priv->map_source);
  priv->keep_center_on_resize = TRUE;
  priv->zoom_on_double_click = TRUE;
  priv->animate_zoom = TRUE;
  priv->license_actor = NULL;
  priv->kinetic_mode = FALSE;
  priv->viewport_x = 0;
  priv->viewport_y = 0;
  priv->viewport_width = 0;
  priv->viewport_height = 0;
  priv->state = SHUMATE_STATE_NONE;
  priv->latitude = 0.0;
  priv->longitude = 0.0;
  priv->goto_context = NULL;
  priv->tiles_loading = 0;
  priv->animating_zoom = FALSE;
  priv->background_content = NULL;
  priv->zoom_overlay_actor = NULL;
  priv->bg_offset_x = 0;
  priv->bg_offset_y = 0;
  priv->location_updated = FALSE;
  priv->redraw_timeout = 0;
  priv->zoom_actor_timeout = 0;
  priv->tile_map = g_hash_table_new_full (g_int64_hash, g_int64_equal, slice_free_gint64, NULL);
  priv->visible_tiles = g_hash_table_new_full (g_int64_hash, g_int64_equal, slice_free_gint64, NULL);
  priv->goto_duration = 0;
  priv->goto_mode = CLUTTER_EASE_IN_OUT_CIRC;
  priv->world_bbox = shumate_bounding_box_new ();
  priv->world_bbox->left = SHUMATE_MIN_LONGITUDE;
  priv->world_bbox->bottom = SHUMATE_MIN_LATITUDE;
  priv->world_bbox->right = SHUMATE_MAX_LONGITUDE;
  priv->world_bbox->top = SHUMATE_MAX_LATITUDE;
  priv->num_right_clones = 0;
  priv->map_clones = NULL;
  priv->user_layer_slots = NULL;
  priv->hwrap = FALSE;

  clutter_actor_set_background_color (CLUTTER_ACTOR (view), &color);

  g_signal_connect (view, "notify::width", G_CALLBACK (view_size_changed_cb), NULL);
  g_signal_connect (view, "notify::height", G_CALLBACK (view_size_changed_cb), NULL);

  g_signal_connect (view, "notify::realized", G_CALLBACK (shumate_view_realized_cb), NULL);

  layout = clutter_bin_layout_new (CLUTTER_BIN_ALIGNMENT_FIXED,
                                   CLUTTER_BIN_ALIGNMENT_FIXED);
  clutter_actor_set_layout_manager (CLUTTER_ACTOR (view), layout);

  /* Setup viewport layers*/
  priv->background_layer = clutter_actor_new ();
  priv->zoom_layer = clutter_actor_new ();
  priv->map_layer = clutter_actor_new ();
  priv->user_layers = clutter_actor_new ();

  priv->viewport_container = clutter_actor_new ();
  clutter_actor_add_child (priv->viewport_container, priv->background_layer);
  clutter_actor_add_child (priv->viewport_container, priv->zoom_layer);
  clutter_actor_add_child (priv->viewport_container, priv->map_layer);
  clutter_actor_add_child (priv->viewport_container, priv->user_layers);

  /* Setup viewport */
  priv->viewport = shumate_viewport_new ();
  shumate_viewport_set_child (SHUMATE_VIEWPORT (priv->viewport), priv->viewport_container);
  g_signal_connect (priv->viewport, "relocated", G_CALLBACK (view_relocated_cb), view);

  g_signal_connect (priv->viewport, "notify::x-origin",
      G_CALLBACK (viewport_pos_changed_cb), view);
  g_signal_connect (priv->viewport, "notify::y-origin",
      G_CALLBACK (viewport_pos_changed_cb), view);
  clutter_actor_set_reactive (priv->viewport, TRUE);

  /* Setup kinetic scroll */
  priv->kinetic_scroll = shumate_kinetic_scroll_view_new (FALSE, SHUMATE_VIEWPORT (priv->viewport));

  g_signal_connect (priv->kinetic_scroll, "scroll-event",
      G_CALLBACK (scroll_event), view);
  g_signal_connect (priv->kinetic_scroll, "panning-completed",
      G_CALLBACK (panning_completed), view);
  g_signal_connect (priv->kinetic_scroll, "button-press-event",
      G_CALLBACK (kinetic_scroll_button_press_cb), view);

  /* Setup zoom gesture */
  priv->zoom_gesture = CLUTTER_GESTURE_ACTION (clutter_zoom_action_new ());
  g_signal_connect (priv->zoom_gesture, "zoom",
                    G_CALLBACK (zoom_gesture_zoom_cb), view);
  g_signal_connect (priv->zoom_gesture, "gesture-begin",
                    G_CALLBACK (zoom_gesture_begin_cb), view);
  g_signal_connect (priv->zoom_gesture, "gesture-end",
                    G_CALLBACK (zoom_gesture_finish_cb), view);
  g_signal_connect (priv->zoom_gesture, "gesture-cancel",
                    G_CALLBACK (zoom_gesture_cancel_cb), view);
  clutter_actor_add_action (CLUTTER_ACTOR (view),
                            CLUTTER_ACTION (priv->zoom_gesture));

  /* Setup stage */
  clutter_actor_add_child (CLUTTER_ACTOR (view), priv->kinetic_scroll);
  priv->zoom_overlay_actor = clutter_actor_new ();
  clutter_actor_add_child (CLUTTER_ACTOR (view), priv->zoom_overlay_actor);
  g_signal_connect (view, "key-press-event",
                    G_CALLBACK (kinetic_scroll_key_press_cb), NULL);

  /* Setup license */
  priv->license_actor = shumate_license_new ();
  shumate_license_connect_view (SHUMATE_LICENSE (priv->license_actor), view);
  clutter_actor_set_x_expand (priv->license_actor, TRUE);
  clutter_actor_set_y_expand (priv->license_actor, TRUE);
  clutter_actor_set_x_align (priv->license_actor, CLUTTER_ACTOR_ALIGN_END);
  clutter_actor_set_y_align (priv->license_actor, CLUTTER_ACTOR_ALIGN_END);
  clutter_actor_add_child (CLUTTER_ACTOR (view), priv->license_actor);
}


static gboolean
redraw_timeout_cb (gpointer data)
{
  DEBUG_LOG ()

  ShumateView *view = data;
  ShumateViewPrivate *priv = view->priv;
  gdouble x, y;

  shumate_viewport_get_origin (SHUMATE_VIEWPORT (priv->viewport), &x, &y);

  if (priv->location_updated || (gint)ABS (x - priv->viewport_x) > 0 || (gint)ABS (y - priv->viewport_y) > 0)
    {
      update_coords (view, x, y, TRUE);
      load_visible_tiles (view, FALSE);
      priv->location_updated = FALSE;
    }

  return TRUE;
}


static void
viewport_pos_changed_cb (G_GNUC_UNUSED GObject *gobject,
    G_GNUC_UNUSED GParamSpec *arg1,
    ShumateView *view)
{
  DEBUG_LOG ()

  ShumateViewPrivate *priv = view->priv;
  gdouble x, y;

  if (priv->redraw_timeout == 0)
    priv->redraw_timeout = g_timeout_add (350, redraw_timeout_cb, view);

  shumate_viewport_get_origin (SHUMATE_VIEWPORT (priv->viewport), &x, &y);

  if (priv->hwrap)
    {
      gint map_width;
      map_width = get_map_width (view);

      /* Faux wrapping, by positioning viewport to correct wrap point
       * so the master map view is on the left edge of ShumateView
       * (possibly partially invisible) */
      if (x < 0 || x >= map_width)
        position_viewport (view, x_to_wrap_x (x, map_width), y);
    }

  if (ABS (x - priv->viewport_x) > 100 || ABS (y - priv->viewport_y) > 100)
    {
      update_coords (view, x, y, FALSE);
      load_visible_tiles (view, FALSE);
      priv->location_updated = TRUE;
    }
}


static void
swap_user_layer_slots (ShumateView *view,
    gint original_index,
    gint clone_index)
{
  ShumateViewPrivate *priv = view->priv;
  gint map_width = get_map_width (view);

  GList *original_slot = g_list_nth (priv->user_layer_slots, original_index);
  GList *clone_slot = g_list_nth (priv->user_layer_slots, clone_index);

  ClutterActor *clone = clone_slot->data;

  original_slot->data = clone;
  clone_slot->data = priv->user_layers;

  clutter_actor_set_x (clone, (original_index - 1) * map_width);
  clutter_actor_set_x (priv->user_layers, (clone_index - 1) * map_width);
}


static gboolean
viewport_motion_cb (G_GNUC_UNUSED ClutterActor *actor,
    ClutterMotionEvent *event,
    ShumateView *view)
{
   ShumateViewPrivate *priv = view->priv;

   gint map_width = get_map_width (view);

   gint original_index = g_list_index (priv->user_layer_slots, priv->user_layers);
   gint clone_index = (event->x + priv->viewport_x) / map_width + 1;

   if (clone_index != original_index && clone_index < priv->num_right_clones + 2)
     swap_user_layer_slots (view, original_index, clone_index);

   return TRUE;
 }


static ClutterActor *
sample_user_layer_at_pos (ShumateView *view,
    gfloat x,
    gfloat y)
{
    ShumateViewPrivate *priv = view->priv;

    ClutterStage *stage = CLUTTER_STAGE (clutter_actor_get_stage (CLUTTER_ACTOR (view)));
    ClutterActor *retval = clutter_stage_get_actor_at_pos (stage,
        CLUTTER_PICK_REACTIVE, x, y);

    /* If no reactive actor is found on top of the clone, return NULL */
    if (!clutter_actor_contains (priv->user_layers, retval))
      return NULL;

    return retval;
}


static gboolean
viewport_press_cb (G_GNUC_UNUSED ClutterActor *actor,
    ClutterButtonEvent *event,
    ShumateView *view)
{
  DEBUG_LOG ()

  ShumateViewPrivate *priv = view->priv;

  if (!priv->hwrap)
    return FALSE;

  gint original_index = g_list_index (priv->user_layer_slots, priv->user_layers);
  gint initial_original_index = original_index;
  ClutterActor *sampled_actor = NULL;

  /* Sampling neighbouring slots for children that are split by the slot border.
   * (e.g. a marker that has one half in a slot #n and the other half in #n-1)
   * Whenever a user clicks on the real user layer, it is swapped succesively with
   * the right and left neighbors (if they exist) and the area at the event
   * coordinates is inspected for a reactive child actor. If a child is found,
   * a button press is synthesized over it.
   */
  gint right_neighbor_index = original_index + 1;
  gint left_neighbor_index = original_index - 1;

  /* Swapping and testing right neighbor */
  if (right_neighbor_index < priv->num_right_clones + 2)
    {
      swap_user_layer_slots (view, original_index, right_neighbor_index);
      original_index = right_neighbor_index;
      sampled_actor = sample_user_layer_at_pos (view, event->x, event->y);
    }

  /* Swapping and testing left neighbor */
  if (left_neighbor_index >= 0 && sampled_actor == NULL)
    {
      swap_user_layer_slots (view, original_index, left_neighbor_index);
      original_index = left_neighbor_index;
      sampled_actor = sample_user_layer_at_pos (view, event->x, event->y);
    }

  /* If found, redirecting event to the sampled actor */
  if (sampled_actor != NULL)
    {
      ClutterEvent *cloned_event = (ClutterEvent *)event;
      clutter_event_set_source (cloned_event, sampled_actor);
      clutter_event_put (cloned_event);
    }
  else
    {
      /* Swapping the real layer back to its initial slot */
      if (original_index != initial_original_index)
        swap_user_layer_slots (view, original_index, initial_original_index);

      return FALSE;
    }

  return TRUE;
}

static gboolean
kinetic_scroll_button_press_cb (G_GNUC_UNUSED ClutterActor *actor,
    ClutterButtonEvent *event,
    ShumateView *view)
{
  DEBUG_LOG ()

  ShumateViewPrivate *priv = view->priv;

  if (priv->zoom_on_double_click && event->button == 1 && event->click_count == 2)
    return view_set_zoom_level_at (view, priv->zoom_level + 1, TRUE, event->x, event->y);

  return FALSE; /* Propagate the event */
}


static void
shumate_view_scroll (ShumateView *view,
    gint deltax,
    gint deltay)
{
  DEBUG_LOG ()

  ShumateViewPrivate *priv = view->priv;
  gdouble lat, lon;
  gint x, y;

  x = priv->viewport_x + priv->viewport_width / 2.0 + deltax;
  y = priv->viewport_y + priv->viewport_height / 2.0 + deltay;

  lat = shumate_map_source_get_latitude (priv->map_source, priv->zoom_level, y);
  lon = get_longitude (view, priv->zoom_level, x);

  if (priv->kinetic_mode)
    shumate_view_go_to_with_duration (view, lat, lon, 300);
  else
    shumate_view_center_on (view, lat, lon);
}


static gboolean
kinetic_scroll_key_press_cb (ShumateView *view,
    ClutterKeyEvent *event)
{
  DEBUG_LOG ()

  ShumateViewPrivate *priv = view->priv;

  switch (event->keyval)
    {
    case 65361: /* Left */
      shumate_view_scroll (view, -priv->viewport_width / 4.0, 0);
      return TRUE;
      break;

    case 65362: /* Up */
      if (event->modifier_state & CLUTTER_CONTROL_MASK)
        shumate_view_zoom_in (view);
      else
        shumate_view_scroll (view, 0, -priv->viewport_width / 4.0);
      return TRUE;
      break;

    case 65363: /* Right */
      shumate_view_scroll (view, priv->viewport_width / 4.0, 0);
      return TRUE;
      break;

    case 65364: /* Down */
      if (event->modifier_state & CLUTTER_CONTROL_MASK)
        shumate_view_zoom_out (view);
      else
        shumate_view_scroll (view, 0, priv->viewport_width / 4.0);
      return TRUE;
      break;

    default:
      return FALSE; /* Propagate the event */
    }
  return FALSE; /* Propagate the event */
}


/**
 * shumate_view_new:
 *
 * Creates an instance of #ShumateView.
 *
 * Returns: a new #ShumateView ready to be used as a #ClutterActor.
 */
ClutterActor *
shumate_view_new (void)
{
  DEBUG_LOG ()

  return g_object_new (SHUMATE_TYPE_VIEW, NULL);
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
  DEBUG_LOG ()

  g_return_if_fail (SHUMATE_IS_VIEW (view));

  gdouble x, y;
  ShumateViewPrivate *priv = view->priv;

  longitude = CLAMP (longitude, priv->world_bbox->left, priv->world_bbox->right);
  latitude = CLAMP (latitude, priv->world_bbox->bottom, priv->world_bbox->top);

  x = shumate_map_source_get_x (priv->map_source, priv->zoom_level, longitude) - priv->viewport_width / 2.0;
  y = shumate_map_source_get_y (priv->map_source, priv->zoom_level, latitude) - priv->viewport_height / 2.0;

  DEBUG ("Centering on %f, %f (%g, %g)", latitude, longitude, x, y);

  if (priv->hwrap)
    position_viewport (view, x_to_wrap_x (x, get_map_width (view)), y);
  else
    position_viewport (view, x, y);
  load_visible_tiles (view, FALSE);
}


static void
timeline_new_frame (G_GNUC_UNUSED ClutterTimeline *timeline,
    G_GNUC_UNUSED gint frame_num,
    GoToContext *ctx)
{
  DEBUG_LOG ()

  gdouble alpha;
  gdouble lat;
  gdouble lon;

  alpha = clutter_timeline_get_progress (timeline);
  lat = ctx->to_latitude - ctx->from_latitude;
  lon = ctx->to_longitude - ctx->from_longitude;

  shumate_view_center_on (ctx->view,
      ctx->from_latitude + alpha * lat,
      ctx->from_longitude + alpha * lon);
}


static void
timeline_completed (G_GNUC_UNUSED ClutterTimeline *timeline,
    ShumateView *view)
{
  DEBUG_LOG ()

  shumate_view_stop_go_to (view);
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
  DEBUG_LOG ()

  g_return_if_fail (SHUMATE_IS_VIEW (view));

  ShumateViewPrivate *priv = view->priv;

  if (priv->goto_context == NULL)
    return;

  clutter_timeline_stop (priv->goto_context->timeline);

  g_object_unref (priv->goto_context->timeline);

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
    gdouble latitude,
    gdouble longitude)
{
  DEBUG_LOG ()

  guint duration = view->priv->goto_duration;

  if (duration == 0) /* calculate duration from zoom level */
      duration = 500 * view->priv->zoom_level / 2.0;

  shumate_view_go_to_with_duration (view, latitude, longitude, duration);
}


static void
shumate_view_go_to_with_duration (ShumateView *view,
    gdouble latitude,
    gdouble longitude,
    guint duration) /* In ms */
{
  DEBUG_LOG ()

  g_return_if_fail (SHUMATE_IS_VIEW (view));

  if (duration == 0)
    {
      shumate_view_center_on (view, latitude, longitude);
      return;
    }

  GoToContext *ctx;

  ShumateViewPrivate *priv = view->priv;

  shumate_view_stop_go_to (view);

  ctx = g_slice_new (GoToContext);
  ctx->from_latitude = priv->latitude;
  ctx->from_longitude = priv->longitude;
  ctx->to_latitude = CLAMP (latitude, priv->world_bbox->bottom, priv->world_bbox->top);
  ctx->to_longitude = CLAMP (longitude, priv->world_bbox->left, priv->world_bbox->right);

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
  ctx->timeline = clutter_timeline_new (duration);
  clutter_timeline_set_progress_mode (ctx->timeline, priv->goto_mode);

  g_signal_connect (ctx->timeline, "new-frame", G_CALLBACK (timeline_new_frame),
      ctx);
  g_signal_connect (ctx->timeline, "completed", G_CALLBACK (timeline_completed),
      view);

  clutter_timeline_start (ctx->timeline);
}


/**
 * shumate_view_zoom_in:
 * @view: a #ShumateView
 *
 * Zoom in the map by one level.
 */
void
shumate_view_zoom_in (ShumateView *view)
{
  DEBUG_LOG ()

  g_return_if_fail (SHUMATE_IS_VIEW (view));

  shumate_view_set_zoom_level (view, view->priv->zoom_level + 1);
}


/**
 * shumate_view_zoom_out:
 * @view: a #ShumateView
 *
 * Zoom out the map by one level.
 */
void
shumate_view_zoom_out (ShumateView *view)
{
  DEBUG_LOG ()

  g_return_if_fail (SHUMATE_IS_VIEW (view));

  shumate_view_set_zoom_level (view, view->priv->zoom_level - 1);
}

static void
paint_surface (ShumateView *view,
    cairo_t *cr,
    cairo_surface_t *surface,
    double x,
    double y,
    double opacity)
{
  ShumateViewPrivate *priv = view->priv;

  gint map_width = get_map_width (view);

  cairo_set_source_surface (cr,
                            surface,
                            x, y);
  cairo_paint_with_alpha (cr, opacity);

  /* Paint each surface num_right_clones - 1 extra times on the right
   * (last clone is not actually visible) and once in the left
   * in order to horizontally wrap.
   */
  if (priv->hwrap)
    {
      gint i;

      for (i = 0; i < priv->num_right_clones + 1; i++)
        {
          /* Don't repaint original layer */
          if (i == 1)
            continue;

          cairo_set_source_surface (cr,
                            surface,
                            x + (i - 1) * map_width, y);
          cairo_paint_with_alpha (cr, opacity);
        }
    }
}

static void
layers_to_surface (ShumateView *view,
    cairo_t *cr)
{
  ClutterActorIter iter;
  ClutterActor *child;

  clutter_actor_iter_init (&iter, view->priv->user_layers);
  while (clutter_actor_iter_next (&iter, &child))
    {
      ShumateLayer *layer = SHUMATE_LAYER (child);
      cairo_surface_t *surface;

      if (!SHUMATE_IS_CAIRO_EXPORTABLE (layer))
        continue;

      surface = shumate_cairo_exportable_get_surface (SHUMATE_CAIRO_EXPORTABLE (layer));
      if (!surface)
        continue;

      paint_surface (view, cr, surface, 0, 0, 255);
    }
}


/**
 * shumate_view_to_surface:
 * @view: a #ShumateView
 * @include_layers: Set to %TRUE if you want to include layers
 *
 * Will generate a #cairo_surface_t that represents the current view
 * of the map. Without any markers or layers. If the current #ShumateRenderer
 * used does not support this, this function will return %NULL.
 *
 * If @include_layers is set to %TRUE all layers that implement
 * #ShumateExportable will be included in the surface.
 *
 * The #ShumateView also need to be in #SHUMATE_STATE_DONE state.
 *
 * Returns: (transfer full): a #cairo_surface_t or %NULL on failure. Free with
 *          cairo_surface_destroy() when done.
 */
cairo_surface_t *
shumate_view_to_surface (ShumateView *view,
    gboolean include_layers)
{
  DEBUG_LOG ()

  g_return_val_if_fail (SHUMATE_IS_VIEW (view), NULL);

  ShumateViewPrivate *priv = view->priv;
  cairo_surface_t *surface;
  cairo_t *cr;
  ClutterActorIter iter;
  ClutterActor *child;
  gdouble width, height;

  if (priv->state != SHUMATE_STATE_DONE)
    return NULL;

  width = clutter_actor_get_width (CLUTTER_ACTOR (view));
  height = clutter_actor_get_height (CLUTTER_ACTOR (view));
  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
  cr = cairo_create (surface);

  clutter_actor_iter_init (&iter, priv->map_layer);
  while (clutter_actor_iter_next (&iter, &child))
    {
      ShumateTile *tile = SHUMATE_TILE (child);
      guint tile_x = shumate_tile_get_x (tile);
      guint tile_y = shumate_tile_get_y (tile);
      guint tile_size = shumate_tile_get_size (tile);

      if (tile_in_tile_table (view, priv->tile_map, tile_x, tile_y))
        {
          cairo_surface_t *tile_surface;
          double x, y, opacity;

          tile_surface = shumate_cairo_exportable_get_surface (SHUMATE_CAIRO_EXPORTABLE (tile));
          if (!tile_surface)
            {
              cairo_destroy (cr);
              cairo_surface_destroy (surface);
              return NULL;
            }
          opacity = ((double) clutter_actor_get_opacity (CLUTTER_ACTOR (tile))) / 255.0;
          x = ((double) tile_x * tile_size) - priv->viewport_x;
          y = ((double) tile_y * tile_size) - priv->viewport_y;

          paint_surface (view, cr, tile_surface, x, y, opacity);
        }
    }

    if (include_layers)
      layers_to_surface (view, cr);

    cairo_destroy (cr);
    return surface;
}


/**
 * shumate_view_set_zoom_level:
 * @view: a #ShumateView
 * @zoom_level: the level of zoom, a guint between 1 and 20
 *
 * Changes the current level of zoom
 */
void
shumate_view_set_zoom_level (ShumateView *view,
    guint zoom_level)
{
  DEBUG_LOG ()

  g_return_if_fail (SHUMATE_IS_VIEW (view));

  view_set_zoom_level_at (view, zoom_level, FALSE, 0, 0);
}


/**
 * shumate_view_set_min_zoom_level:
 * @view: a #ShumateView
 * @zoom_level: the level of zoom
 *
 * Changes the lowest allowed level of zoom
 */
void
shumate_view_set_min_zoom_level (ShumateView *view,
    guint min_zoom_level)
{
  DEBUG_LOG ()

  g_return_if_fail (SHUMATE_IS_VIEW (view));

  ShumateViewPrivate *priv = view->priv;

  if (priv->min_zoom_level == min_zoom_level ||
      min_zoom_level > priv->max_zoom_level ||
      min_zoom_level < shumate_map_source_get_min_zoom_level (priv->map_source))
    return;

  priv->min_zoom_level = min_zoom_level;
  g_object_notify (G_OBJECT (view), "min-zoom-level");

  if (priv->zoom_level < min_zoom_level)
    shumate_view_set_zoom_level (view, min_zoom_level);
}


/**
 * shumate_view_set_max_zoom_level:
 * @view: a #ShumateView
 * @zoom_level: the level of zoom
 *
 * Changes the highest allowed level of zoom
 */
void
shumate_view_set_max_zoom_level (ShumateView *view,
    guint max_zoom_level)
{
  DEBUG_LOG ()

  g_return_if_fail (SHUMATE_IS_VIEW (view));

  ShumateViewPrivate *priv = view->priv;

  if (priv->max_zoom_level == max_zoom_level ||
      max_zoom_level < priv->min_zoom_level ||
      max_zoom_level > shumate_map_source_get_max_zoom_level (priv->map_source))
    return;

  priv->max_zoom_level = max_zoom_level;
  g_object_notify (G_OBJECT (view), "max-zoom-level");

  if (priv->zoom_level > max_zoom_level)
    shumate_view_set_zoom_level (view, max_zoom_level);
}

/**
 * shumate_view_get_world:
 * @view: a #ShumateView
 *
 * Get the bounding box that represents the extent of the world.
 *
 * Returns: (transfer none): a #ShumateBoundingBox that represents the current world
 */
ShumateBoundingBox *
shumate_view_get_world (ShumateView *view)
{
  g_return_val_if_fail (SHUMATE_IS_VIEW (view), NULL);

  ShumateViewPrivate *priv = view->priv;

  return priv->world_bbox;
}


/**
 * shumate_view_set_world:
 * @view: a #ShumateView
 * @bbox: (transfer none): the #ShumateBoundingBox of the world
 *
 * Set a bounding box to limit the world to. No tiles will be loaded
 * outside of this bounding box. It will not be possible to scroll outside
 * of this bounding box.
 */
void
shumate_view_set_world (ShumateView *view,
    ShumateBoundingBox *bbox)
{
  g_return_if_fail (SHUMATE_IS_VIEW (view));
  g_return_if_fail (bbox != NULL);

  if (!shumate_bounding_box_is_valid (bbox))
    return;

  ShumateViewPrivate *priv = view->priv;
  gdouble latitude, longitude;

  bbox->left = CLAMP (bbox->left, SHUMATE_MIN_LONGITUDE, SHUMATE_MAX_LONGITUDE);
  bbox->bottom = CLAMP (bbox->bottom, SHUMATE_MIN_LATITUDE, SHUMATE_MAX_LATITUDE);
  bbox->right = CLAMP (bbox->right, SHUMATE_MIN_LONGITUDE, SHUMATE_MAX_LONGITUDE);
  bbox->top = CLAMP (bbox->top, SHUMATE_MIN_LATITUDE, SHUMATE_MAX_LATITUDE);

  if (priv->world_bbox)
    shumate_bounding_box_free (priv->world_bbox);

  priv->world_bbox = shumate_bounding_box_copy (bbox);

  if (!shumate_bounding_box_covers (priv->world_bbox, priv->latitude, priv->longitude))
    {
      shumate_bounding_box_get_center (priv->world_bbox, &latitude, &longitude);
      shumate_view_center_on (view, latitude, longitude);
    }
}

/**
 * shumate_view_add_layer:
 * @view: a #ShumateView
 * @layer: a #ShumateLayer
 *
 * Adds a new layer to the view
 */
void
shumate_view_add_layer (ShumateView *view,
    ShumateLayer *layer)
{
  DEBUG_LOG ()

  g_return_if_fail (SHUMATE_IS_VIEW (view));
  g_return_if_fail (SHUMATE_IS_LAYER (layer));

  clutter_actor_add_child (view->priv->user_layers, CLUTTER_ACTOR (layer));
  shumate_layer_set_view (layer, view);
  clutter_actor_set_child_above_sibling (view->priv->user_layers, CLUTTER_ACTOR (layer), NULL);
}


/**
 * shumate_view_remove_layer:
 * @view: a #ShumateView
 * @layer: a #ShumateLayer
 *
 * Removes the given layer from the view
 */
void
shumate_view_remove_layer (ShumateView *view,
    ShumateLayer *layer)
{
  DEBUG_LOG ()

  g_return_if_fail (SHUMATE_IS_VIEW (view));
  g_return_if_fail (SHUMATE_IS_LAYER (layer));

  shumate_layer_set_view (layer, NULL);

  clutter_actor_remove_child (view->priv->user_layers, CLUTTER_ACTOR (layer));
}


/**
 * shumate_view_x_to_longitude:
 * @view: a #ShumateView
 * @x: x coordinate of the view
 *
 * Converts the view's x coordinate to longitude.
 *
 * Returns: the longitude
 */
gdouble
shumate_view_x_to_longitude (ShumateView *view,
    gdouble x)
{
  ShumateViewPrivate *priv = view->priv;

  DEBUG_LOG ()

  g_return_val_if_fail (SHUMATE_IS_VIEW (view), 0.0);

  return get_longitude (view, priv->zoom_level, x + priv->viewport_x);
}


/**
 * shumate_view_y_to_latitude:
 * @view: a #ShumateView
 * @y: y coordinate of the view
 *
 * Converts the view's y coordinate to latitude.
 *
 * Returns: the latitude
 */
gdouble
shumate_view_y_to_latitude (ShumateView *view,
    gdouble y)
{
  ShumateViewPrivate *priv = view->priv;
  gdouble latitude;

  DEBUG_LOG ()

  g_return_val_if_fail (SHUMATE_IS_VIEW (view), 0.0);

  latitude = shumate_map_source_get_latitude (priv->map_source,
        priv->zoom_level,
        y + priv->viewport_y);

  return latitude;
}


/**
 * shumate_view_longitude_to_x:
 * @view: a #ShumateView
 * @longitude: the longitude
 *
 * Converts the longitude to view's x coordinate.
 *
 * Returns: the x coordinate
 */
gdouble
shumate_view_longitude_to_x (ShumateView *view,
    gdouble longitude)
{
  ShumateViewPrivate *priv = view->priv;
  gdouble x;

  DEBUG_LOG ()

  g_return_val_if_fail (SHUMATE_IS_VIEW (view), 0);

  x = shumate_map_source_get_x (priv->map_source, priv->zoom_level, longitude);

  return x - priv->viewport_x;
}


/**
 * shumate_view_latitude_to_y:
 * @view: a #ShumateView
 * @latitude: the latitude
 *
 * Converts the latitude to view's y coordinate.
 *
 * Returns: the y coordinate
 */
gdouble
shumate_view_latitude_to_y (ShumateView *view,
    gdouble latitude)
{
  ShumateViewPrivate *priv = view->priv;
  gdouble y;

  DEBUG_LOG ()

  g_return_val_if_fail (SHUMATE_IS_VIEW (view), 0);

  y = shumate_map_source_get_y (priv->map_source, priv->zoom_level, latitude);

  return y - priv->viewport_y;
}

/**
 * shumate_view_get_viewport_anchor:
 * @view: a #ShumateView
 * @anchor_x: (out): the x coordinate of the viewport anchor
 * @anchor_y: (out): the y coordinate of the viewport anchor
 *
 * Gets the x and y coordinate of the viewport anchor in respect to the layer origin.
 */
void
shumate_view_get_viewport_anchor (ShumateView *view,
    gint *anchor_x,
    gint *anchor_y)
{
  DEBUG_LOG ()

  g_return_if_fail (SHUMATE_IS_VIEW (view));
  ShumateViewPrivate *priv = view->priv;

  shumate_viewport_get_anchor (SHUMATE_VIEWPORT (priv->viewport), anchor_x, anchor_y);
}

/**
 * shumate_view_get_viewport_origin:
 * @view: a #ShumateView
 * @x: (out): the x coordinate of the viewport
 * @y: (out): the y coordinate of the viewport
 *
 * Gets the x and y coordinate of the viewport in respect to the layer origin.
 */
void
shumate_view_get_viewport_origin (ShumateView *view,
    gint *x,
    gint *y)
{
  DEBUG_LOG ()

  g_return_if_fail (SHUMATE_IS_VIEW (view));
  ShumateViewPrivate *priv = view->priv;
  gint anchor_x, anchor_y;

  shumate_viewport_get_anchor (SHUMATE_VIEWPORT (priv->viewport), &anchor_x, &anchor_y);

  if (x)
    *x = priv->viewport_x - anchor_x;

  if (y)
    *y = priv->viewport_y - anchor_y;
}


static void
fill_background_tiles (ShumateView *view)
{
  DEBUG_LOG ()

  ShumateViewPrivate *priv = view->priv;
  ClutterActorIter iter;
  ClutterActor *child;
  gint x_count, y_count, x_first, y_first;
  gint x, y;
  gfloat width, height;
  gboolean have_children = TRUE;

  clutter_content_get_preferred_size (priv->background_content, &width, &height);

  x_count = ceil ((gfloat) priv->viewport_width / width) + 3;
  y_count = ceil ((gfloat) priv->viewport_height / height) + 3;

  x_first = (gint)priv->viewport_x / width - 1;
  y_first = (gint)priv->viewport_y / height - 1;

  clutter_actor_iter_init (&iter, priv->background_layer);

  for (x = x_first; x < x_first + x_count; ++x)
    {
      for (y = y_first; y < y_first + y_count; ++y)
        {
          if (!have_children || !clutter_actor_iter_next (&iter, &child))
            {
              have_children = FALSE;
              child = clutter_actor_new ();
              clutter_actor_set_size (child, width, height);
              clutter_actor_set_content (child, priv->background_content);
              clutter_actor_add_child (priv->background_layer, child);
            }
          shumate_viewport_set_actor_position (SHUMATE_VIEWPORT (priv->viewport),
              child,
              (x * width) - priv->bg_offset_x,
              (y * height) - priv->bg_offset_y);
          child = clutter_actor_get_next_sibling (child);
        }
    }

  if (have_children)
    {
      while (clutter_actor_iter_next (&iter, &child))
          clutter_actor_iter_destroy (&iter);
    }
}

static void
tile_table_set (ShumateView *view,
    GHashTable *table,
    gint tile_x,
    gint tile_y,
    gboolean value)
{
  ShumateViewPrivate *priv = view->priv;
  gint64 count = shumate_map_source_get_column_count (priv->map_source, priv->zoom_level);
  gint64 *key = g_slice_alloc (sizeof(gint64));
  *key = (gint64)tile_y * count + tile_x;
  if (value)
    g_hash_table_insert (table, key, GINT_TO_POINTER (TRUE));
  else
    {
      g_hash_table_remove (table, key);
      g_slice_free (gint64, key);
    }
}


static gboolean
tile_in_tile_table (ShumateView *view,
    GHashTable *table,
    gint tile_x,
    gint tile_y)
{
  ShumateViewPrivate *priv = view->priv;
  gint64 count = shumate_map_source_get_column_count (priv->map_source, priv->zoom_level);
  gint64 key = (gint64)tile_y * count + tile_x;
  return GPOINTER_TO_INT (g_hash_table_lookup (table, &key));
}


static void
load_tile_for_source (ShumateView *view,
    ShumateMapSource *source,
    gint opacity,
    gint size,
    gint x,
    gint y)
{
  ShumateViewPrivate *priv = view->priv;
  ShumateTile *tile = shumate_tile_new ();

  DEBUG ("Loading tile %d, %d, %d", priv->zoom_level, x, y);

  shumate_tile_set_x (tile, x);
  shumate_tile_set_y (tile, y);
  shumate_tile_set_zoom_level (tile, priv->zoom_level);
  shumate_tile_set_size (tile, size);
  clutter_actor_set_opacity (CLUTTER_ACTOR (tile), opacity);

  g_signal_connect (tile, "notify::state", G_CALLBACK (tile_state_notify), view);
  clutter_actor_add_child (priv->map_layer, CLUTTER_ACTOR (tile));
  shumate_viewport_set_actor_position (SHUMATE_VIEWPORT (priv->viewport), CLUTTER_ACTOR (tile), x * size, y * size);

  /* updates shumate_view state automatically as
     notify::state signal is connected  */
  shumate_tile_set_state (tile, SHUMATE_STATE_LOADING);

  shumate_map_source_fill_tile (source, tile);

  if (source != priv->map_source)
    g_object_set_data (G_OBJECT (tile), "overlay", GINT_TO_POINTER (TRUE));
}


static gboolean
fill_tile_cb (FillTileCallbackData *data)
{
  DEBUG_LOG ()

  ShumateView *view = data->view;
  ShumateViewPrivate *priv = view->priv;
  gint x = data->x;
  gint y = data->y;
  gint size = data->size;
  gint zoom_level = data->zoom_level;

  if (!tile_in_tile_table (view, priv->tile_map, x, y) &&
      zoom_level == priv->zoom_level &&
      data->map_source == priv->map_source &&
      tile_in_tile_table (view, priv->visible_tiles, x, y))
    {
      GList *iter;

      load_tile_for_source (view, priv->map_source, 255, size, x, y);
      for (iter = priv->overlay_sources; iter; iter = iter->next)
        {
          gint opacity = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (iter->data), "opacity"));
          load_tile_for_source (view, iter->data, opacity, size, x, y);
        }

      tile_table_set (view, priv->tile_map, x, y, TRUE);
    }

  g_object_unref (view);
  g_slice_free (FillTileCallbackData, data);

  return FALSE;
}

static void
load_visible_tiles (ShumateView *view,
    gboolean relocate)
{
  DEBUG_LOG ()

  ShumateViewPrivate *priv = view->priv;
  ClutterActorIter iter;
  gint size;
  ClutterActor *child;
  gint x_count, y_count, column_count;
  guint min_x, min_y, max_x, max_y;
  gint arm_size, arm_max, turn;
  gint dirs[5] = { 0, 1, 0, -1, 0 };
  gint i, x, y;

  size = shumate_map_source_get_tile_size (priv->map_source);
  get_tile_bounds (view, &min_x, &min_y, &max_x, &max_y);

  x_count = ceil ((gfloat) priv->viewport_width / size) + 1;
  column_count = shumate_map_source_get_column_count (priv->map_source, priv->zoom_level);

  if (priv->hwrap)
    {
      priv->tile_x_first = priv->viewport_x / size;
      priv->tile_x_last = priv->tile_x_first + x_count;
    }
  else
    {
      priv->tile_x_first = CLAMP (priv->viewport_x / size, min_x, max_x);
      priv->tile_x_last = priv->tile_x_first + x_count;
      priv->tile_x_last = CLAMP (priv->tile_x_last, priv->tile_x_first, max_x);
      x_count = priv->tile_x_last - priv->tile_x_first;
    }

  y_count = ceil ((gfloat) priv->viewport_height / size) + 1;
  priv->tile_y_first = CLAMP (priv->viewport_y / size, min_y, max_y);
  priv->tile_y_last = priv->tile_y_first + y_count;
  priv->tile_y_last = CLAMP (priv->tile_y_last, priv->tile_y_first, max_y);
  y_count = priv->tile_y_last - priv->tile_y_first;

  DEBUG ("Range %d, %d to %d, %d", priv->tile_x_first, priv->tile_y_first, priv->tile_x_last, priv->tile_y_last);

  g_hash_table_remove_all (priv->visible_tiles);
  for (x = priv->tile_x_first; x < priv->tile_x_last; x++)
    for (y = priv->tile_y_first; y < priv->tile_y_last; y++)
      {
        gint tile_x = x;

        if (priv->hwrap)
          tile_x = x_to_wrap_x (tile_x, column_count);

        tile_table_set (view, priv->visible_tiles, tile_x, y, TRUE);
      }

  /* fill background tiles */
  if (priv->background_content != NULL)
      fill_background_tiles (view);

  /* Get rid of old tiles first */
  clutter_actor_iter_init (&iter, priv->map_layer);
  while (clutter_actor_iter_next (&iter, &child))
    {
      ShumateTile *tile = SHUMATE_TILE (child);

      gint tile_x = shumate_tile_get_x (tile);
      gint tile_y = shumate_tile_get_y (tile);

      if (!tile_in_tile_table (view, priv->visible_tiles, tile_x, tile_y))
        {
          shumate_tile_set_state (tile, SHUMATE_STATE_DONE);
          clutter_actor_iter_destroy (&iter);
          tile_table_set (view, priv->tile_map, tile_x, tile_y, FALSE);
        }
      else if (relocate)
        shumate_viewport_set_actor_position (SHUMATE_VIEWPORT (priv->viewport), CLUTTER_ACTOR (tile), tile_x * size, tile_y * size);
    }

  /* Load new tiles if needed */
  x = priv->tile_x_first + x_count / 2 - 1;
  y = priv->tile_y_first + y_count / 2 - 1;
  arm_max = MAX (x_count, y_count) + 2;
  arm_size = 1;

  for (turn = 0; arm_size < arm_max; turn++)
    {
      for (i = 0; i < arm_size; i++)
        {
          gint tile_x = x;

          if (priv->hwrap)
            tile_x = x_to_wrap_x (tile_x, column_count);

          if (!tile_in_tile_table (view, priv->tile_map, tile_x, y) &&
              tile_in_tile_table (view, priv->visible_tiles, tile_x, y))
            {
              FillTileCallbackData *data;

              DEBUG ("Loading tile %d, %d, %d", priv->zoom_level, x, y);

              data = g_slice_new (FillTileCallbackData);
              data->x = tile_x;
              data->y = y;
              data->size = size;
              data->zoom_level = priv->zoom_level;
              /* used only to check that the map source didn't change before the
               * idle function is called */
              data->map_source = priv->map_source;
              data->view = g_object_ref (view);

              g_idle_add_full (CLUTTER_PRIORITY_REDRAW, (GSourceFunc) fill_tile_cb, data, NULL);
            }

          x += dirs[turn % 4 + 1];
          y += dirs[turn % 4];
        }

      if (turn % 2 == 1)
        arm_size++;
    }
}


static void
remove_all_tiles (ShumateView *view)
{
  DEBUG_LOG ()

  ShumateViewPrivate *priv = view->priv;
  ClutterActorIter iter;
  ClutterActor *child;

  clutter_actor_destroy_all_children (priv->zoom_layer);

  clutter_actor_iter_init (&iter, priv->map_layer);
  while (clutter_actor_iter_next (&iter, &child))
    shumate_tile_set_state (SHUMATE_TILE (child), SHUMATE_STATE_DONE);

  g_hash_table_remove_all (priv->tile_map);

  clutter_actor_destroy_all_children (priv->map_layer);
}


/**
 * shumate_view_reload_tiles:
 * @view: a #ShumateView
 *
 * Reloads all visible tiles.
 */
void
shumate_view_reload_tiles (ShumateView *view)
{
  DEBUG_LOG ()

  remove_all_tiles (view);

  load_visible_tiles (view, FALSE);
}


static gboolean
remove_zoom_actor_cb (ShumateView *view)
{
  ShumateViewPrivate *priv = view->priv;

  clutter_actor_destroy_all_children (priv->zoom_layer);
  priv->zoom_actor_timeout = 0;
  return FALSE;
}


static void
tile_state_notify (ShumateTile *tile,
    G_GNUC_UNUSED GParamSpec *pspec,
    ShumateView *view)
{
  DEBUG_LOG ()

  ShumateState tile_state = shumate_tile_get_state (tile);
  ShumateViewPrivate *priv = view->priv;

  if (tile_state == SHUMATE_STATE_LOADING)
    {
      if (priv->tiles_loading == 0)
        {
          priv->state = SHUMATE_STATE_LOADING;
          g_object_notify (G_OBJECT (view), "state");
        }
      priv->tiles_loading++;
    }
  else if (tile_state == SHUMATE_STATE_DONE)
    {
      if (priv->tiles_loading > 0)
        priv->tiles_loading--;
      if (priv->tiles_loading == 0)
        {
          priv->state = SHUMATE_STATE_DONE;
          g_object_notify (G_OBJECT (view), "state");
          if (clutter_actor_get_n_children (priv->zoom_layer) > 0)
            priv->zoom_actor_timeout = g_timeout_add_seconds_full (CLUTTER_PRIORITY_REDRAW, 1, (GSourceFunc) remove_zoom_actor_cb, view, NULL);
        }
    }
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
shumate_view_set_map_source (ShumateView *view,
    ShumateMapSource *source)
{
  DEBUG_LOG ()

  g_return_if_fail (SHUMATE_IS_VIEW (view) &&
      SHUMATE_IS_MAP_SOURCE (source));

  ShumateViewPrivate *priv = view->priv;
  guint source_min_zoom;
  guint source_max_zoom;

  if (priv->map_source == source)
    return;

  g_object_unref (priv->map_source);
  priv->map_source = g_object_ref_sink (source);

  g_list_free_full (priv->overlay_sources, g_object_unref);
  priv->overlay_sources = NULL;

  source_min_zoom = shumate_map_source_get_min_zoom_level (priv->map_source);
  shumate_view_set_min_zoom_level (view, source_min_zoom);
  source_max_zoom = shumate_map_source_get_max_zoom_level (priv->map_source);
  shumate_view_set_max_zoom_level (view, source_max_zoom);

  /* Keep same zoom level if the new map supports it */
  if (priv->zoom_level > priv->max_zoom_level)
    {
      priv->zoom_level = priv->max_zoom_level;
      g_object_notify (G_OBJECT (view), "zoom-level");
    }
  else if (priv->zoom_level < priv->min_zoom_level)
    {
      priv->zoom_level = priv->min_zoom_level;
      g_object_notify (G_OBJECT (view), "zoom-level");
    }

  shumate_view_reload_tiles (view);

  g_object_notify (G_OBJECT (view), "map-source");
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
    gdouble rate)
{
  DEBUG_LOG ()

  g_return_if_fail (SHUMATE_IS_VIEW (view) &&
      rate < 2.0 && rate > 1.0001);

  g_object_set (view->priv->kinetic_scroll, "decel-rate", rate, NULL);
  g_object_notify (G_OBJECT (view), "deceleration");
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
    gboolean kinetic)
{
  DEBUG_LOG ()

  g_return_if_fail (SHUMATE_IS_VIEW (view));

  ShumateViewPrivate *priv = view->priv;

  priv->kinetic_mode = kinetic;
  g_object_set (view->priv->kinetic_scroll, "mode", kinetic, NULL);
  g_object_notify (G_OBJECT (view), "kinetic-mode");
}


/**
 * shumate_view_set_keep_center_on_resize:
 * @view: a #ShumateView
 * @value: a #gboolean
 *
 * Keep the current centered position when resizing the view.
 */
void
shumate_view_set_keep_center_on_resize (ShumateView *view,
    gboolean value)
{
  DEBUG_LOG ()

  g_return_if_fail (SHUMATE_IS_VIEW (view));

  view->priv->keep_center_on_resize = value;
  g_object_notify (G_OBJECT (view), "keep-center-on-resize");
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
    gboolean value)
{
  DEBUG_LOG ()

  g_return_if_fail (SHUMATE_IS_VIEW (view));

  view->priv->zoom_on_double_click = value;
  g_object_notify (G_OBJECT (view), "zoom-on-double-click");
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
    gboolean value)
{
  DEBUG_LOG ()

  g_return_if_fail (SHUMATE_IS_VIEW (view));

  view->priv->animate_zoom = value;
  g_object_notify (G_OBJECT (view), "animate-zoom");
}


/**
 * shumate_view_ensure_visible:
 * @view: a #ShumateView
 * @bbox: bounding box of the area that should be visible
 * @animate: TRUE to perform animation, FALSE otherwise
 *
 * Changes the map's zoom level and center to make sure the given area
 * is visible
 */
void
shumate_view_ensure_visible (ShumateView *view,
    ShumateBoundingBox *bbox,
    gboolean animate)
{
  DEBUG_LOG ()

  ShumateViewPrivate *priv = view->priv;
  guint zoom_level = priv->zoom_level;
  gboolean good_size = FALSE;
  gdouble lat, lon;

  if (!shumate_bounding_box_is_valid (bbox))
    return;

  shumate_bounding_box_get_center (bbox, &lat, &lon);

  DEBUG ("Zone to expose (%f, %f) to (%f, %f)", bbox->bottom, bbox->left, bbox->top, bbox->right);
  do
    {
      gint min_x, min_y, max_x, max_y;

      min_x = shumate_map_source_get_x (priv->map_source, zoom_level, bbox->left);
      min_y = shumate_map_source_get_y (priv->map_source, zoom_level, bbox->bottom);
      max_x = shumate_map_source_get_x (priv->map_source, zoom_level, bbox->right);
      max_y = shumate_map_source_get_y (priv->map_source, zoom_level, bbox->top);

      if (min_y - max_y <= priv->viewport_height &&
          max_x - min_x <= priv->viewport_width)
        good_size = TRUE;
      else
        zoom_level--;

      if (zoom_level <= priv->min_zoom_level)
        {
          zoom_level = priv->min_zoom_level;
          good_size = TRUE;
        }
    } while (!good_size);

  DEBUG ("Ideal zoom level is %d", zoom_level);
  shumate_view_set_zoom_level (view, zoom_level);
  if (animate)
    shumate_view_go_to (view, lat, lon);
  else
    shumate_view_center_on (view, lat, lon);
}


/**
 * shumate_view_ensure_layers_visible:
 * @view: a #ShumateView
 * @animate: TRUE to perform animation, FALSE otherwise
 *
 * Changes the map's zoom level and center to make sure that the bounding
 * boxes of all inserted layers are visible.
 */
void
shumate_view_ensure_layers_visible (ShumateView *view,
    gboolean animate)
{
  DEBUG_LOG ()

  ClutterActorIter iter;
  ClutterActor *child;
  ShumateBoundingBox *bbox;

  bbox = shumate_bounding_box_new ();

  clutter_actor_iter_init (&iter, view->priv->user_layers);
  while (clutter_actor_iter_next (&iter, &child))
    {
      ShumateLayer *layer = SHUMATE_LAYER (child);
      ShumateBoundingBox *other;

      other = shumate_layer_get_bounding_box (layer);
      shumate_bounding_box_compose (bbox, other);
      shumate_bounding_box_free (other);
    }

  shumate_view_ensure_visible (view, bbox, animate);

  shumate_bounding_box_free (bbox);
}


/**
 * shumate_view_set_background_pattern:
 * @view: a #ShumateView
 * @background: The background texture
 *
 * Sets the background texture displayed behind the map. Setting the background
 * pattern affects performence slightly - use reasonably large patterns for
 * better performance.
 */
void
shumate_view_set_background_pattern (ShumateView *view,
    ClutterContent *background)
{
  DEBUG_LOG ()

  g_return_if_fail (SHUMATE_IS_VIEW (view));

  ShumateViewPrivate *priv = view->priv;

  if (priv->background_content)
    g_object_unref (priv->background_content);

  priv->background_content = g_object_ref_sink (background);
  clutter_actor_destroy_all_children (priv->background_layer);
}


/**
 * shumate_view_get_background_pattern:
 * @view: a #ShumateView
 *
 * Gets the current background texture displayed behind the map.
 *
 * Returns: (transfer none): The texture.
 */
ClutterContent *
shumate_view_get_background_pattern (ShumateView *view)
{
  DEBUG_LOG ()

  g_return_val_if_fail (SHUMATE_IS_VIEW (view), NULL);

  ShumateViewPrivate *priv = view->priv;

  return priv->background_content;
}


/**
 * shumate_view_set_horizontal_wrap:
 * @view: a #ShumateView
 * @wrap: %TRUE to enable horizontal wrapping
 *
 * Sets the value of the #ShumateView:horizontal-wrap property.
 */
void
shumate_view_set_horizontal_wrap (ShumateView *view,
    gboolean wrap)
{
  DEBUG_LOG ()

  g_return_if_fail (SHUMATE_IS_VIEW (view));

  ShumateViewPrivate *priv = view->priv;

  if (priv->hwrap == wrap)
    return;

  priv->hwrap = wrap;

  if (priv->hwrap)
    {
      g_signal_connect (priv->viewport, "motion-event",
        G_CALLBACK (viewport_motion_cb), view);
      g_signal_connect (priv->viewport, "button-press-event",
        G_CALLBACK (viewport_press_cb), view);
      update_clones (view);
    }
  else
    {
      g_list_free_full (priv->map_clones, (GDestroyNotify) clutter_actor_destroy);
      g_list_free_full (priv->user_layer_slots, (GDestroyNotify) exclusive_destroy_clone);
      priv->map_clones = NULL;
      priv->user_layer_slots = NULL;
      g_signal_handlers_disconnect_by_func (priv->viewport, viewport_motion_cb, view);
      g_signal_handlers_disconnect_by_func (priv->viewport, viewport_press_cb, view);
      clutter_actor_set_x (priv->user_layers, 0);
    }
  resize_viewport (view);

  gint map_width = get_map_width (view);
  if (priv->hwrap)
    position_viewport (view, x_to_wrap_x (priv->viewport_x, map_width), priv->viewport_y);
  else
    position_viewport (view, priv->viewport_x - ((gint)priv->viewport_width / map_width / 2) * map_width, priv->viewport_y);

  load_visible_tiles (view, FALSE);
}


/**
 * shumate_view_get_horizontal_wrap:
 * @view: a #ShumateView
 *
 * Returns the value of the #ShumateView:horizontal-wrap property.
 *
 * Returns: %TRUE if #ShumateView:horizontal-wrap is set.
 *
 */
gboolean
shumate_view_get_horizontal_wrap (ShumateView *view)
{
  DEBUG_LOG ()

  g_return_val_if_fail (SHUMATE_IS_VIEW (view), FALSE);

  ShumateViewPrivate *priv = view->priv;

  return priv->hwrap;
}


static void
position_zoom_actor (ShumateView *view)
{
  ShumateViewPrivate *priv = view->priv;
  gdouble x, y;
  gdouble deltazoom;

  clutter_actor_destroy_all_children (priv->zoom_layer);
  if (priv->zoom_actor_timeout != 0)
    {
      g_source_remove (priv->zoom_actor_timeout);
      priv->zoom_actor_timeout = 0;
    }

  ClutterActor *zoom_actor = clutter_actor_get_first_child (priv->zoom_overlay_actor);
  clutter_actor_set_pivot_point (zoom_actor, 0.0, 0.0);

  g_object_ref (zoom_actor);
  clutter_actor_remove_child(priv->zoom_overlay_actor, zoom_actor);
  clutter_actor_add_child (priv->zoom_layer, zoom_actor);
  g_object_unref (zoom_actor);

  deltazoom = pow (2, (gdouble)priv->zoom_level - (gdouble)priv->anim_start_zoom_level);
  x = priv->zoom_actor_viewport_x * deltazoom;
  y = priv->zoom_actor_viewport_y * deltazoom;

  shumate_viewport_set_actor_position (SHUMATE_VIEWPORT (priv->viewport), zoom_actor, x, y);
}


static void
zoom_animation_completed (ClutterActor *actor,
    const gchar *transition_name,
    gboolean is_finished,
    ShumateView *view)
{
  ShumateViewPrivate *priv = view->priv;

  priv->animating_zoom = FALSE;
  position_zoom_actor (view);
  clutter_actor_show (priv->user_layers);
  if (priv->hwrap)
    update_clones (view);

  if (priv->tiles_loading == 0)
    clutter_actor_destroy_all_children (priv->zoom_layer);

  g_signal_handlers_disconnect_by_func (actor, zoom_animation_completed, view);
  g_signal_emit_by_name (view, "animation-completed::zoom", NULL);
}


static void
show_zoom_actor (ShumateView *view,
    guint zoom_level,
    gdouble x,
    gdouble y)
{
  DEBUG_LOG ()

  ShumateViewPrivate *priv = view->priv;
  ClutterActor *zoom_actor = NULL;
  gdouble deltazoom;

  if (!priv->animating_zoom)
    {
      ClutterActorIter iter;
      ClutterActor *child;
      ClutterActor *tile_container;
      gint size;
      gint x_first, y_first;
      gdouble zoom_actor_width, zoom_actor_height;
      gint column_count;
      gdouble deltax, deltay;
      guint min_x, min_y, max_x, max_y;

      get_tile_bounds (view, &min_x, &min_y, &max_x, &max_y);
      size = shumate_map_source_get_tile_size (priv->map_source);

      column_count = shumate_map_source_get_column_count (priv->map_source, priv->zoom_level);

      x_first = CLAMP (priv->viewport_x / size, min_x, max_x);
      y_first = CLAMP (priv->viewport_y / size, min_y, max_y);

      clutter_actor_destroy_all_children (priv->zoom_overlay_actor);
      zoom_actor = clutter_actor_new ();
      clutter_actor_add_child (priv->zoom_overlay_actor, zoom_actor);

      deltax = priv->viewport_x - x_first * size;
      deltay = priv->viewport_y - y_first * size;

      priv->anim_start_zoom_level = priv->zoom_level;
      priv->zoom_actor_viewport_x = priv->viewport_x - deltax;
      priv->zoom_actor_viewport_y = priv->viewport_y - deltay;

      tile_container = clutter_actor_new ();
      clutter_actor_iter_init (&iter, priv->map_layer);
      while (clutter_actor_iter_next (&iter, &child))
        {
          ShumateTile *tile = SHUMATE_TILE (child);
          gint tile_x = shumate_tile_get_x (tile);
          gint tile_y = shumate_tile_get_y (tile);
          gboolean overlay = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (tile), "overlay"));

          shumate_tile_set_state (tile, SHUMATE_STATE_DONE);

          g_object_ref (CLUTTER_ACTOR (tile));
          clutter_actor_iter_remove (&iter);
          clutter_actor_add_child (tile_container, CLUTTER_ACTOR (tile));
          g_object_unref (CLUTTER_ACTOR (tile));

          /* We move overlay tiles to the zoom actor so they get properly reparented
             and destroyed as needed, but we hide them for performance reasons */
          if (overlay)
            clutter_actor_hide (CLUTTER_ACTOR (tile));

          clutter_actor_set_position (CLUTTER_ACTOR (tile), (tile_x - x_first) * size, (tile_y - y_first) * size);
        }
      clutter_actor_add_child (zoom_actor, tile_container);

      /* The tile_container is cloned and its clones are also added to the zoom_actor
       * in order to horizontally wrap. Moreover, the old clones are hidden while the zooming
       * animation is runnning.
       */
      if (priv->hwrap)
        {
          GList *old_clone = priv->map_clones;
          gint i;

          for (i = 0; i < priv->num_right_clones + 2; i++)
            {
              gfloat tiles_x;
              ClutterActor *clone;

              if (i == 1)
                continue;

              clone = clutter_clone_new (tile_container);

              clutter_actor_hide (CLUTTER_ACTOR (old_clone->data));

              clutter_actor_get_position (tile_container, &tiles_x, NULL);
              clutter_actor_set_x (clone, tiles_x + ((i - 1) * column_count * size));

              clutter_actor_add_child (zoom_actor, clone);

              old_clone = old_clone->next;
            }
        }

      zoom_actor_width = clutter_actor_get_width (zoom_actor);
      zoom_actor_height = clutter_actor_get_height (zoom_actor);

      clutter_actor_set_pivot_point (zoom_actor, (x + deltax) / zoom_actor_width, (y + deltay) / zoom_actor_height);
      clutter_actor_set_position (zoom_actor, -deltax, -deltay);
    }
  else
    zoom_actor = clutter_actor_get_first_child (priv->zoom_overlay_actor);

  deltazoom = pow (2.0, (gdouble)zoom_level - priv->anim_start_zoom_level);

  if (priv->animate_zoom)
    {
      clutter_actor_set_opacity (priv->map_layer, 0);

      clutter_actor_destroy_all_children (priv->zoom_layer);

      clutter_actor_save_easing_state (zoom_actor);
      clutter_actor_set_easing_mode (zoom_actor, CLUTTER_EASE_IN_OUT_QUAD);
      clutter_actor_set_easing_duration (zoom_actor, 350);
      clutter_actor_set_scale (zoom_actor, deltazoom, deltazoom);
      clutter_actor_restore_easing_state (zoom_actor);

      clutter_actor_save_easing_state (priv->map_layer);
      clutter_actor_set_easing_mode (priv->map_layer, CLUTTER_EASE_IN_EXPO);
      clutter_actor_set_easing_duration (priv->map_layer, 350);
      clutter_actor_set_opacity (priv->map_layer, 255);
      clutter_actor_restore_easing_state (priv->map_layer);

      if (!priv->animating_zoom)
        {
          if (priv->hwrap)
            {
              GList *slot;
              for (slot = priv->user_layer_slots; slot != NULL; slot = slot->next)
                clutter_actor_hide (CLUTTER_ACTOR (slot->data));
            }
          else
            clutter_actor_hide (priv->user_layers);

          g_signal_connect (zoom_actor, "transition-stopped::scale-x", G_CALLBACK (zoom_animation_completed), view);
        }

      priv->animating_zoom = TRUE;
    }
  else
    {
      clutter_actor_set_scale (zoom_actor, deltazoom, deltazoom);
      if (priv->hwrap)
        update_clones (view);
    }
}

static void
get_x_y_for_zoom_level (ShumateView *view,
                        guint zoom_level,
                        gint offset_x,
                        gint offset_y,
                        gdouble *new_x,
                        gdouble *new_y)
{
  ShumateViewPrivate *priv = view->priv;
  gdouble deltazoom;

  deltazoom = pow (2, (gdouble) zoom_level - (gdouble) priv->zoom_level);

  *new_x = (priv->viewport_x + offset_x) * deltazoom - offset_x;
  *new_y = (priv->viewport_y + offset_y) * deltazoom - offset_y;
}

/* Sets the zoom level, leaving the (x, y) at the exact same point in the view */
static gboolean
view_set_zoom_level_at (ShumateView *view,
    guint zoom_level,
    gboolean use_event_coord,
    gint x,
    gint y)
{
  DEBUG_LOG ()

  ShumateViewPrivate *priv = view->priv;
  gdouble new_x, new_y;
  gdouble offset_x = x;
  gdouble offset_y = y;

  if (zoom_level == priv->zoom_level || ZOOM_LEVEL_OUT_OF_RANGE (priv, zoom_level))
    return FALSE;

  shumate_view_stop_go_to (view);

  if (!use_event_coord)
    {
      offset_x = priv->viewport_width / 2.0;
      offset_y = priv->viewport_height / 2.0;
    }

  /* don't do anything when view not yet realized */
  if (CLUTTER_ACTOR_IS_REALIZED (view))
    show_zoom_actor (view, zoom_level, offset_x, offset_y);

  get_x_y_for_zoom_level (view, zoom_level, offset_x, offset_y, &new_x, &new_y);

  priv->zoom_level = zoom_level;

  if (CLUTTER_ACTOR_IS_REALIZED (view))
    {
      resize_viewport (view);
      remove_all_tiles (view);
      if (priv->hwrap)
        position_viewport (view, x_to_wrap_x (new_x, get_map_width (view)), new_y);
      else
        position_viewport (view, new_x, new_y);
      load_visible_tiles (view, FALSE);

      if (!priv->animate_zoom)
        position_zoom_actor (view);
    }

  g_object_notify (G_OBJECT (view), "zoom-level");
  return TRUE;
}


/**
 * shumate_view_get_zoom_level:
 * @view: a #ShumateView
 *
 * Gets the view's current zoom level.
 *
 * Returns: the view's current zoom level.
 */
guint
shumate_view_get_zoom_level (ShumateView *view)
{
  DEBUG_LOG ()

  g_return_val_if_fail (SHUMATE_IS_VIEW (view), 0);

  return view->priv->zoom_level;
}


/**
 * shumate_view_get_min_zoom_level:
 * @view: a #ShumateView
 *
 * Gets the view's minimal allowed zoom level.
 *
 * Returns: the view's minimal allowed zoom level.
 */
guint
shumate_view_get_min_zoom_level (ShumateView *view)
{
  DEBUG_LOG ()

  g_return_val_if_fail (SHUMATE_IS_VIEW (view), 0);

  return view->priv->min_zoom_level;
}


/**
 * shumate_view_get_max_zoom_level:
 * @view: a #ShumateView
 *
 * Gets the view's maximum allowed zoom level.
 *
 * Returns: the view's maximum allowed zoom level.
 */
guint
shumate_view_get_max_zoom_level (ShumateView *view)
{
  DEBUG_LOG ()

  g_return_val_if_fail (SHUMATE_IS_VIEW (view), 0);

  return view->priv->max_zoom_level;
}


/**
 * shumate_view_get_map_source:
 * @view: a #ShumateView
 *
 * Gets the view's current map source.
 *
 * Returns: (transfer none): the view's current map source. If you need to keep a reference to the
 * map source then you have to call #g_object_ref().
 */
ShumateMapSource *
shumate_view_get_map_source (ShumateView *view)
{
  DEBUG_LOG ()

  g_return_val_if_fail (SHUMATE_IS_VIEW (view), NULL);

  return view->priv->map_source;
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
  DEBUG_LOG ()

  g_return_val_if_fail (SHUMATE_IS_VIEW (view), 0.0);

  gdouble decel = 0.0;
  g_object_get (view->priv->kinetic_scroll, "decel-rate", &decel, NULL);
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
  DEBUG_LOG ()

  g_return_val_if_fail (SHUMATE_IS_VIEW (view), FALSE);

  return view->priv->kinetic_mode;
}


/**
 * shumate_view_get_keep_center_on_resize:
 * @view: a #ShumateView
 *
 * Checks whether to keep the center on resize
 *
 * Returns: TRUE if the view keeps the center on resize, FALSE otherwise.
 */
gboolean
shumate_view_get_keep_center_on_resize (ShumateView *view)
{
  DEBUG_LOG ()

  g_return_val_if_fail (SHUMATE_IS_VIEW (view), FALSE);

  return view->priv->keep_center_on_resize;
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
  DEBUG_LOG ()

  g_return_val_if_fail (SHUMATE_IS_VIEW (view), FALSE);

  return view->priv->zoom_on_double_click;
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
  DEBUG_LOG ()

  g_return_val_if_fail (SHUMATE_IS_VIEW (view), FALSE);

  return view->priv->animate_zoom;
}


static ClutterActorAlign
bin_alignment_to_actor_align (ClutterBinAlignment alignment)
{
    switch (alignment)
      {
        case CLUTTER_BIN_ALIGNMENT_FILL:
            return CLUTTER_ACTOR_ALIGN_FILL;
        case CLUTTER_BIN_ALIGNMENT_START:
            return CLUTTER_ACTOR_ALIGN_START;
        case CLUTTER_BIN_ALIGNMENT_END:
            return CLUTTER_ACTOR_ALIGN_END;
        case CLUTTER_BIN_ALIGNMENT_CENTER:
            return CLUTTER_ACTOR_ALIGN_CENTER;
        default:
            return CLUTTER_ACTOR_ALIGN_START;
      }
}

/**
 * shumate_view_get_license_actor:
 * @view: a #ShumateView
 *
 * Returns the #ShumateLicense actor which is inserted by default into the
 * layout manager. It can be manipulated using standard #ClutterActor methods
 * (hidden and so on).
 *
 * Returns: (transfer none): the license actor
 */
ShumateLicense *
shumate_view_get_license_actor (ShumateView *view)
{
  DEBUG_LOG ()

  g_return_val_if_fail (SHUMATE_IS_VIEW (view), NULL);

  return SHUMATE_LICENSE (view->priv->license_actor);
}


/**
 * shumate_view_get_center_latitude:
 * @view: a #ShumateView
 *
 * Gets the latitude of the view's center.
 *
 * Returns: the latitude.
 */
gdouble
shumate_view_get_center_latitude (ShumateView *view)
{
  DEBUG_LOG ()

  g_return_val_if_fail (SHUMATE_IS_VIEW (view), 0.0);

  return view->priv->latitude;
}


/**
 * shumate_view_get_center_longitude:
 * @view: a #ShumateView
 *
 * Gets the longitude of the view's center.
 *
 * Returns: the longitude.
 */
gdouble
shumate_view_get_center_longitude (ShumateView *view)
{
  DEBUG_LOG ()

  g_return_val_if_fail (SHUMATE_IS_VIEW (view), 0.0);

  return view->priv->longitude;
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
  DEBUG_LOG ()

  g_return_val_if_fail (SHUMATE_IS_VIEW (view), SHUMATE_STATE_NONE);

  return view->priv->state;
}

static void
get_tile_bounds (ShumateView *view,
                 guint *min_x,
                 guint *min_y,
                 guint *max_x,
                 guint *max_y)
{
  ShumateViewPrivate *priv = view->priv;
  guint size = shumate_map_source_get_tile_size (priv->map_source);
  gint coord;

  coord = shumate_map_source_get_x (priv->map_source,
                                      priv->zoom_level,
                                      priv->world_bbox->left);
  *min_x = coord / size;

  coord = shumate_map_source_get_y (priv->map_source,
                                      priv->zoom_level,
                                      priv->world_bbox->top);
  *min_y = coord/size;

  coord = shumate_map_source_get_x (priv->map_source,
                                      priv->zoom_level,
                                      priv->world_bbox->right);
  *max_x = ceil ((double) coord / (double) size);

  coord  = shumate_map_source_get_y (priv->map_source,
                                       priv->zoom_level,
                                       priv->world_bbox->bottom);
  *max_y = ceil ((double) coord / (double) size);
}

static ShumateBoundingBox *
get_bounding_box (ShumateView *view,
                  guint zoom_level,
                  gdouble x,
                  gdouble y)
{
  ShumateViewPrivate *priv = view->priv;
  ShumateBoundingBox *bbox;

  bbox = shumate_bounding_box_new ();

  bbox->top = shumate_map_source_get_latitude (priv->map_source,
                                                 zoom_level,
                                                 y);
  bbox->bottom = shumate_map_source_get_latitude (priv->map_source,
                                                    zoom_level,
                                                    y + priv->viewport_height);
  bbox->left = get_longitude (view,
                              zoom_level,
                              x);
  bbox->right = get_longitude (view,
                               zoom_level,
                               x + priv->viewport_width);
  return bbox;
}

/**
 * shumate_view_get_bounding_box_for_zoom_level:
 * @view: a #ShumateView
 * @zoom_level: the level of zoom, a guint between 1 and 20
 *
 * Gets the bounding box for view @view at @zoom_level.
 *
 * Returns: (transfer full): the bounding box for the view at @zoom_level.
 */
ShumateBoundingBox *
shumate_view_get_bounding_box_for_zoom_level (ShumateView *view,
                                                guint zoom_level)
{
  ShumateViewPrivate *priv = view->priv;
  gdouble x, y;
  gdouble offset_x, offset_y;

  g_return_val_if_fail (SHUMATE_IS_VIEW (view), NULL);

  offset_x = priv->viewport_width / 2.0;
  offset_y = priv->viewport_height / 2.0;

  get_x_y_for_zoom_level (view, zoom_level, offset_x, offset_y, &x, &y);

  return get_bounding_box (view, zoom_level, x, y);
}

/**
 * shumate_view_get_bounding_box:
 * @view: a #ShumateView
 *
 * Gets the bounding box for view @view at current zoom-level.
 *
 * Returns: (transfer full): the bounding box
 */
ShumateBoundingBox *
shumate_view_get_bounding_box (ShumateView *view)
{
  DEBUG_LOG ()

  ShumateViewPrivate *priv = view->priv;

  g_return_val_if_fail (SHUMATE_IS_VIEW (view), NULL);

  return get_bounding_box (view, priv->zoom_level, priv->viewport_x, priv->viewport_y);
}


/**
 * shumate_view_add_overlay_source:
 * @view: a #ShumateView
 * @map_source: a #ShumateMapSource
 * @opacity: opacity to use
 *
 * Adds a new overlay map source to render tiles with the supplied opacity on top
 * of the ordinary map source. Multiple overlay sources can be added.
 */
void
shumate_view_add_overlay_source (ShumateView *view,
    ShumateMapSource *map_source,
    guint8 opacity)
{
  DEBUG_LOG ()

  ShumateViewPrivate *priv;

  g_return_if_fail (SHUMATE_IS_VIEW (view));
  g_return_if_fail (SHUMATE_IS_MAP_SOURCE (map_source));

  priv = view->priv;
  g_object_ref (map_source);
  priv->overlay_sources = g_list_append (priv->overlay_sources, map_source);
  g_object_set_data (G_OBJECT (map_source), "opacity", GINT_TO_POINTER (opacity));
  g_object_notify (G_OBJECT (view), "map-source");

  shumate_view_reload_tiles (view);
}


/**
 * shumate_view_remove_overlay_source:
 * @view: a #ShumateView
 * @map_source: a #ShumateMapSource
 *
 * Removes an overlay source from #ShumateView.
 */
void
shumate_view_remove_overlay_source (ShumateView *view,
    ShumateMapSource *map_source)
{
  DEBUG_LOG ()

  ShumateViewPrivate *priv;

  g_return_if_fail (SHUMATE_IS_VIEW (view));
  g_return_if_fail (SHUMATE_IS_MAP_SOURCE (map_source));

  priv = view->priv;
  priv->overlay_sources = g_list_remove (priv->overlay_sources, map_source);
  g_object_unref (map_source);
  g_object_notify (G_OBJECT (view), "map-source");

  shumate_view_reload_tiles (view);
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
  DEBUG_LOG ()

  ShumateViewPrivate *priv;

  g_return_val_if_fail (SHUMATE_IS_VIEW (view), NULL);

  priv = view->priv;

  return g_list_copy (priv->overlay_sources);
}
