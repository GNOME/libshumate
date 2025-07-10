/*
 * Copyright 2020 Collabora, Ltd. (https://www.collabora.com)
 * Copyright 2020 Corentin Noël <corentin.noel@collabora.com>
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

#include "shumate-map-layer.h"
#include "shumate-marshal.h"
#include "shumate-memory-cache-private.h"
#include "shumate-tile-private.h"
#include "shumate-symbol-event.h"
#include "shumate-profiling-private.h"
#include "shumate-utils-private.h"
#include "shumate-inspector-settings-private.h"

#ifdef SHUMATE_HAS_VECTOR_RENDERER
#  include "vector/shumate-vector-symbol-container-private.h"
#endif

/**
 * ShumateMapLayer:
 *
 * A [class@Shumate.Layer] implementation that fetches tiles from a [class@Shumate.MapSource]
 * and draws them as a grid.
 *
 * ## CSS nodes
 *
 * `ShumatePoint` has a single CSS node with the name “map-layer”.
 */

struct _ShumateMapLayer
{
  ShumateLayer parent_instance;

  ShumateMapSource *map_source;

  GHashTable *tile_children;

  guint recompute_grid_idle_id;

  float last_recompute_x, last_recompute_y;

  ShumateMemoryCache *memcache;

  gint64 profile_all_tiles_filled_begin;
  gint64 profile_all_tiles_done_begin;

  guint defer_callback_id;
  double defer_latitude_y, defer_longitude_x, defer_zoom_level;
  gint64 defer_frame_time;
  gboolean deferring;

#ifdef SHUMATE_HAS_VECTOR_RENDERER
  ShumateVectorSymbolContainer *symbols;
#endif
};

G_DEFINE_TYPE (ShumateMapLayer, shumate_map_layer, SHUMATE_TYPE_LAYER)

enum
{
  PROP_MAP_SOURCE = 1,
  N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

enum
{
  SYMBOL_CLICKED,
  MAP_LOADED,
  TILE_ERROR,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };

/* This struct represents the location of a tile on the screen. It is the key
 * for the hash table tile_children which stores all visible tiles.
 *
 * Note that, unlike the values given to ShumateTile, the x and y coordinates
 * here are *not* wrapped. For example, a ShumateTile at level 3 might have
 * coordinates of (7, 2) but have a TileGridPosition of (-1, 2). */


static int
positive_mod (int i, int n)
{
  return (i % n + n) % n;
}

typedef struct {
  ShumateTile *tile;
  GCancellable *cancellable;
  guint error;
} TileChild;

static void
tile_child_free (TileChild *child)
{
  g_clear_object (&child->tile);
  g_clear_object (&child->cancellable);
  g_free (child);
}


typedef struct {
  ShumateMapLayer *self;
  TileChild *tile_child;
  char *source_id;
  ShumateGridPosition pos;
} TileFilledData;

static void
tile_filled_data_free (TileFilledData *data)
{
  g_clear_object (&data->self);
  g_clear_pointer (&data->source_id, g_free);
  g_free (data);
}
G_DEFINE_AUTOPTR_CLEANUP_FUNC (TileFilledData, tile_filled_data_free);


static void
add_symbols (ShumateMapLayer     *self,
             ShumateTile         *tile,
             ShumateGridPosition *pos)
{
#ifdef SHUMATE_HAS_VECTOR_RENDERER
  GPtrArray *symbols;

  g_assert (SHUMATE_IS_MAP_LAYER (self));
  g_assert (SHUMATE_IS_TILE (tile));

  if ((symbols = shumate_tile_get_symbols (tile)))
    shumate_vector_symbol_container_add_symbols (self->symbols,
                                                 symbols,
                                                 pos->x,
                                                 pos->y,
                                                 pos->zoom);
#endif
}

static void recompute_grid (ShumateMapLayer *self);

static void
on_tile_notify_state (ShumateMapLayer          *self,
                      G_GNUC_UNUSED GParamSpec *pspec,
                      ShumateTile              *tile)
{
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
on_tile_filled (GObject      *source_object,
                GAsyncResult *res,
                gpointer      user_data)
{
  g_autoptr(TileFilledData) data = user_data;
  g_autoptr(GError) error = NULL;
  gboolean success;

  success = shumate_map_source_fill_tile_finish (SHUMATE_MAP_SOURCE (source_object), res, &error);

  if (!success)
    {
      if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        {
          data->tile_child->error = TRUE;
          g_signal_emit (data->self, signals[TILE_ERROR], 0, data->tile_child->tile, error);
        }
    }
  else
    {
      add_symbols (data->self, data->tile_child->tile, &data->pos);

      shumate_memory_cache_store_tile (data->self->memcache,
                                       data->tile_child->tile,
                                       data->source_id);
    }

  recompute_grid (data->self);
}

static TileChild *
add_tile (ShumateMapLayer     *self,
          ShumateTile         *tile,
          ShumateGridPosition *pos)
{
  TileChild *tile_child = g_new0 (TileChild, 1);
  const char *source_id = shumate_map_source_get_id (self->map_source);

  self->profile_all_tiles_filled_begin = SHUMATE_PROFILE_CURRENT_TIME;
  self->profile_all_tiles_done_begin = SHUMATE_PROFILE_CURRENT_TIME;

  if (shumate_memory_cache_try_fill_tile (self->memcache, tile, source_id))
    {
      add_symbols (self, tile, pos);
    }
  else
    {
      TileFilledData *data = g_new0 (TileFilledData, 1);

      tile_child->cancellable = g_cancellable_new ();

      data->self = g_object_ref (self);
      data->tile_child = tile_child;
      data->source_id = g_strdup (source_id);
      data->pos = *pos;

      shumate_tile_set_paintable (tile, NULL);
      shumate_map_source_fill_tile_async (self->map_source, tile, tile_child->cancellable, on_tile_filled, data);
    }

  tile_child->tile = tile;

  g_hash_table_insert (self->tile_children, pos, tile_child);
  gtk_widget_queue_draw (GTK_WIDGET (self));
  g_signal_connect_object (tile, "notify::state", (GCallback)on_tile_notify_state, self, G_CONNECT_SWAPPED);

  return tile_child;
}

static void
remove_tile (ShumateMapLayer     *self,
             TileChild           *tile_child,
             ShumateGridPosition *pos)
{
  g_cancellable_cancel (tile_child->cancellable);

#ifdef SHUMATE_HAS_VECTOR_RENDERER
  shumate_vector_symbol_container_remove_symbols (self->symbols, pos->x, pos->y, pos->zoom);
#endif

  g_signal_handlers_disconnect_by_func (tile_child->tile, on_tile_notify_state, self);
}

static double
get_effective_zoom_level (ShumateMapLayer *self)
{
  double zoom_level = shumate_viewport_get_zoom_level (shumate_layer_get_viewport (SHUMATE_LAYER (self)));
  ShumateMapSource *map_source = shumate_viewport_get_reference_map_source (shumate_layer_get_viewport (SHUMATE_LAYER (self)));

  if (map_source)
    {
      double reference_tile_size = shumate_map_source_get_tile_size (map_source);
      double our_tile_size = shumate_map_source_get_tile_size (self->map_source);
      return log2 (reference_tile_size / our_tile_size) + zoom_level;
    }
  else
    return zoom_level;
}

static gboolean
defer_tick_callback (GtkWidget     *widget,
                     GdkFrameClock *frame_clock,
                     gpointer       user_data)
{
  ShumateMapLayer *self = SHUMATE_MAP_LAYER (widget);
  self->defer_callback_id = 0;

  recompute_grid (self);
  return G_SOURCE_REMOVE;
}

static gboolean
should_defer (ShumateMapLayer *self)
{
  /* If the map is moving quickly, we may defer loading tiles until it slows back
     down. That way, we don't waste resources loading tiles that will likely be gone
     before they are done loading. */

  ShumateViewport *viewport = shumate_layer_get_viewport (SHUMATE_LAYER (self));
  ShumateMapSource *map_source = self->map_source;
  double zoom_level = get_effective_zoom_level (self);
  double tile_size = shumate_map_source_get_tile_size_at_zoom (map_source, zoom_level);
  double map_height = shumate_map_source_get_row_count (map_source, zoom_level) * tile_size;
  double map_width = shumate_map_source_get_column_count (map_source, zoom_level) * tile_size;

  double longitude_x = shumate_map_source_get_x (map_source, zoom_level, shumate_location_get_longitude (SHUMATE_LOCATION (viewport)));
  double latitude_y = shumate_map_source_get_y (map_source, zoom_level, shumate_location_get_latitude (SHUMATE_LOCATION (viewport)));

  double delta_x = self->defer_longitude_x * map_width - longitude_x;
  double delta_y = self->defer_latitude_y * map_height - latitude_y;
  double velocity = sqrt (delta_x * delta_x + delta_y * delta_y);
  double width = gtk_widget_get_width (GTK_WIDGET (self));
  double height = gtk_widget_get_height (GTK_WIDGET (self));
  double diagonal = sqrt (width * width + height * height);
  double zoom_velocity = self->defer_zoom_level - zoom_level;

  gint64 frame_time;

  if (!gtk_widget_get_realized (GTK_WIDGET (self)))
    return FALSE;

  frame_time = gdk_frame_clock_get_frame_time (gtk_widget_get_frame_clock (GTK_WIDGET (self)));
  /* Only compare between frames, otherwise we might mistakenly think the
     velocity is 0. */
  if (frame_time == self->defer_frame_time)
    return self->deferring;

  if (velocity > diagonal * 0.25 || fabs (zoom_velocity) > 0.25)
    {
      if (self->defer_callback_id == 0)
        self->defer_callback_id = gtk_widget_add_tick_callback (GTK_WIDGET (self), defer_tick_callback, NULL, NULL);

      self->deferring = TRUE;
    }
  else
    self->deferring = FALSE;

  self->defer_latitude_y = latitude_y / map_height;
  self->defer_longitude_x = longitude_x / map_width;
  self->defer_zoom_level = zoom_level;
  self->defer_frame_time = frame_time;

  return self->deferring;
}

static void
recompute_grid (ShumateMapLayer *self)
{
  /* Computes which tile positions are visible, ensures that all the right
   * tiles are loaded, and removes tiles which are no longer visible. */

  GHashTableIter iter;
  gpointer key, value;

  int width = gtk_widget_get_width (GTK_WIDGET (self));
  int height = gtk_widget_get_height (GTK_WIDGET (self));
  ShumateViewport *viewport = shumate_layer_get_viewport (SHUMATE_LAYER (self));
  int tile_size = shumate_map_source_get_tile_size (self->map_source);
  int zoom_level = (int)floor (get_effective_zoom_level (self));
  double latitude = shumate_location_get_latitude (SHUMATE_LOCATION (viewport));
  double longitude = shumate_location_get_longitude (SHUMATE_LOCATION (viewport));
  double latitude_y = shumate_map_source_get_y (self->map_source, zoom_level, latitude);
  double longitude_x = shumate_map_source_get_x (self->map_source, zoom_level, longitude);
  guint64 source_rows = shumate_map_source_get_row_count (self->map_source, zoom_level);
  guint64 source_columns = shumate_map_source_get_column_count (self->map_source, zoom_level);

  double rotation = shumate_viewport_get_rotation (viewport);
  int n_tiles = 0;

  int size_x = MAX (
    abs ((int) (cos (rotation) *  width/2.0 - sin (rotation) * height/2.0)),
    abs ((int) (cos (rotation) * -width/2.0 - sin (rotation) * height/2.0))
  );
  int size_y = MAX (
    abs ((int) (sin (rotation) *  width/2.0 + cos (rotation) * height/2.0)),
    abs ((int) (sin (rotation) * -width/2.0 + cos (rotation) * height/2.0))
  );

  // This is the (column, row) of the top left tile
  gint64 tile_initial_column = floor ((longitude_x - size_x) / (double) tile_size) - 1;
  gint64 tile_initial_row = floor ((latitude_y - size_y) / (double) tile_size) - 1;
  gint64 tile_final_column = ceil ((longitude_x + size_x) / (double) tile_size) + 1;
  gint64 tile_final_row = ceil ((latitude_y + size_y) / (double) tile_size) + 1;
  int required_columns = tile_final_column - tile_initial_column;
  int required_rows = tile_final_row - tile_initial_row;

  gboolean defer = should_defer (self);

  gboolean all_filled = TRUE, all_done = TRUE, all_succeeded = TRUE;

  /* First, remove all the tiles that aren't in bounds, or that are on the
   * wrong zoom level and haven't finished loading */
  g_hash_table_iter_init (&iter, self->tile_children);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      ShumateGridPosition *pos = key;
      TileChild *tile_child = value;
      float size = powf (2, zoom_level - pos->zoom);
      float x = pos->x * size;
      float y = pos->y * size;

      if (x + size <= tile_initial_column
          || x >= tile_initial_column + required_columns
          || y + size <= tile_initial_row
          || y >= tile_initial_row + required_rows
          || (pos->zoom != zoom_level && shumate_tile_get_state (tile_child->tile) != SHUMATE_STATE_DONE))
        {
          remove_tile (self, tile_child, pos);
          g_hash_table_iter_remove (&iter);
        }
    }

  /* Next, make sure every visible tile position has a matching ShumateTile. */
  for (int x = tile_initial_column; x < tile_initial_column + required_columns; x ++)
    {
      for (int y = tile_initial_row; y < tile_initial_row + required_rows; y ++)
        {
          g_autoptr(ShumateGridPosition) pos = shumate_grid_position_new (x, y, zoom_level);
          TileChild *tile_child = g_hash_table_lookup (self->tile_children, pos);

          n_tiles ++;

          if (!tile_child && !defer)
            {
              ShumateTile *tile = shumate_tile_new_full (positive_mod (x, source_columns), positive_mod (y, source_rows), tile_size, zoom_level);
              shumate_tile_set_scale_factor (tile, gtk_widget_get_scale_factor (GTK_WIDGET (self)));
              tile_child = add_tile (self, tile, g_steal_pointer (&pos));
            }

          if (tile_child == NULL || shumate_tile_get_paintable (tile_child->tile) == NULL)
            all_filled = FALSE;

          if (tile_child == NULL || shumate_tile_get_state (tile_child->tile) != SHUMATE_STATE_DONE)
            all_done = FALSE;

          if (tile_child != NULL && tile_child->error)
            all_succeeded = FALSE;
        }
    }

  if (all_done && self->profile_all_tiles_done_begin > 0)
    {
      g_autofree char *desc = g_strdup_printf ("Visible tiles done (%d)", n_tiles);
      SHUMATE_PROFILE_COLLECT (self->profile_all_tiles_done_begin, desc, NULL);
      self->profile_all_tiles_done_begin = 0;

      g_signal_emit (self, signals[MAP_LOADED], 0, !all_succeeded);
    }
  if (all_filled && self->profile_all_tiles_filled_begin > 0)
    {
      g_autofree char *desc = g_strdup_printf ("Visible tiles filled (%d)", n_tiles);
      SHUMATE_PROFILE_COLLECT (self->profile_all_tiles_filled_begin, desc, NULL);
      self->profile_all_tiles_filled_begin = 0;
    }

  /* If all the tiles on the current zoom level are filled, delete tiles on all
   * other zoom levels */
  if (all_done)
    {
      g_hash_table_iter_init (&iter, self->tile_children);
      while (g_hash_table_iter_next (&iter, &key, &value))
        {
          ShumateGridPosition *pos = key;
          TileChild *tile_child = value;

          if (pos->zoom != zoom_level)
            {
              remove_tile (self, tile_child, pos);
              g_hash_table_iter_remove (&iter);
            }
        }
    }

  self->last_recompute_y = latitude_y / (double) (tile_size * source_rows);
  self->last_recompute_x = longitude_x / (double) (tile_size * source_columns);

  gtk_widget_queue_draw (GTK_WIDGET (self));
}

static gboolean
recompute_grid_in_idle_cb (gpointer user_data)
{
  ShumateMapLayer *self = user_data;

  g_assert (SHUMATE_IS_MAP_LAYER (self));

  recompute_grid (self);

  self->recompute_grid_idle_id = 0;
  return G_SOURCE_REMOVE;
}

static void
queue_recompute_grid_in_idle (ShumateMapLayer *self)
{
  /* recompute_grid might add symbols to the map, which we can't do during
   * certain operations, like size_allocate. So, in most cases, we schedule
   * it to run later (but before the next frame) instead.
   * Also, since we make sure to only have one queued recompute_grid at once,
   * it has a nice side effect of running the function only once even if
   * several viewport properties change at once. */

  g_assert (SHUMATE_IS_MAP_LAYER (self));

  if (self->recompute_grid_idle_id > 0)
    return;

  self->recompute_grid_idle_id = g_idle_add (recompute_grid_in_idle_cb, self);
  g_source_set_name_by_id (self->recompute_grid_idle_id,
                           "[shumate] recompute_grid_in_idle_cb");
}


static void
on_viewport_changed (ShumateMapLayer *self,
                     GParamSpec      *pspec,
                     ShumateViewport *view)
{
  g_assert (SHUMATE_IS_MAP_LAYER (self));

  queue_recompute_grid_in_idle (self);
}

static void
shumate_map_layer_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  ShumateMapLayer *self = SHUMATE_MAP_LAYER (object);

  switch (property_id)
    {
    case PROP_MAP_SOURCE:
      g_set_object (&self->map_source, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
shumate_map_layer_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  ShumateMapLayer *self = SHUMATE_MAP_LAYER (object);

  switch (property_id)
    {
    case PROP_MAP_SOURCE:
      g_value_set_object (value, self->map_source);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
shumate_map_layer_dispose (GObject *object)
{
  ShumateMapLayer *self = SHUMATE_MAP_LAYER (object);
  ShumateViewport *viewport = shumate_layer_get_viewport (SHUMATE_LAYER (self));
  GtkWidget *child;

  g_signal_handlers_disconnect_by_data (viewport, self);
  while ((child = gtk_widget_get_first_child (GTK_WIDGET (object))))
    gtk_widget_unparent (child);

  g_clear_handle_id (&self->recompute_grid_idle_id, g_source_remove);
  g_clear_pointer (&self->tile_children, g_hash_table_unref);
  g_clear_object (&self->map_source);
  g_clear_object (&self->memcache);

  G_OBJECT_CLASS (shumate_map_layer_parent_class)->dispose (object);
}

#ifdef SHUMATE_HAS_VECTOR_RENDERER
static void
on_symbol_clicked (ShumateMapLayer              *self,
                   ShumateSymbolEvent           *event,
                   ShumateVectorSymbolContainer *container)
{
  g_signal_emit (self, signals[SYMBOL_CLICKED], 0, event);
}
#endif

static void
shumate_map_layer_constructed (GObject *object)
{
  ShumateInspectorSettings *settings = shumate_inspector_settings_get_default ();
  ShumateMapLayer *self = SHUMATE_MAP_LAYER (object);
  ShumateViewport *viewport;

  G_OBJECT_CLASS (shumate_map_layer_parent_class)->constructed (object);

  viewport = shumate_layer_get_viewport (SHUMATE_LAYER (self));
  g_signal_connect_swapped (viewport, "notify", G_CALLBACK (on_viewport_changed), self);

  g_signal_connect_object (settings, "notify::show-tile-bounds", G_CALLBACK (queue_recompute_grid_in_idle), self, G_CONNECT_SWAPPED);

#ifdef SHUMATE_HAS_VECTOR_RENDERER
  self->symbols = shumate_vector_symbol_container_new (self->map_source, viewport);
  g_signal_connect_object (self->symbols, "symbol-clicked", G_CALLBACK (on_symbol_clicked), self, G_CONNECT_SWAPPED);
  gtk_widget_set_parent (GTK_WIDGET (self->symbols), GTK_WIDGET (self));
#endif
}

static void
shumate_map_layer_size_allocate (GtkWidget *widget,
                                 int        width,
                                 int        height,
                                 int        baseline)
{
  ShumateMapLayer *self = SHUMATE_MAP_LAYER (widget);
  GtkAllocation child_allocation;

#ifdef SHUMATE_HAS_VECTOR_RENDERER
  /* gtk_widget_measure needs to be called during size_allocate, but we don't
   * care about the result here--the symbol container always gets the same
   * size as the map layer */
  gtk_widget_measure (GTK_WIDGET (self->symbols), GTK_ORIENTATION_VERTICAL, -1, NULL, NULL, NULL, NULL);

  child_allocation.x = 0;
  child_allocation.y = 0;
  child_allocation.width = width;
  child_allocation.height = height;
  gtk_widget_size_allocate (GTK_WIDGET (self->symbols), &child_allocation, baseline);
#endif

  /* Make sure the tile grid is up to date */
  queue_recompute_grid_in_idle (self);
}

static void
shumate_map_layer_measure (GtkWidget      *widget,
                           GtkOrientation  orientation,
                           int             for_size,
                           int            *minimum,
                           int            *natural,
                           int            *minimum_baseline,
                           int            *natural_baseline)
{
  if (minimum)
    *minimum = 0;

  if (natural)
    *natural = 0;
}

static double
snap_coordinate (double point,
                 double translate,
                 double size)
{
  return round ((point - translate) / size) * size + translate;
}

static double
round_px (double x, double scale_factor)
{
  return round (x * scale_factor) / scale_factor;
}

static void
shumate_map_layer_snapshot (GtkWidget *widget, GtkSnapshot *snapshot)
{
  ShumateMapLayer *self = SHUMATE_MAP_LAYER (widget);
  ShumateViewport *viewport = shumate_layer_get_viewport (SHUMATE_LAYER (self));
  double zoom_level = get_effective_zoom_level (self);
  int width = gtk_widget_get_width (GTK_WIDGET (self));
  int height = gtk_widget_get_height (GTK_WIDGET (self));
  double rotation = shumate_viewport_get_rotation (viewport);
  double latitude = shumate_location_get_latitude (SHUMATE_LOCATION (viewport));
  double longitude = shumate_location_get_longitude (SHUMATE_LOCATION (viewport));
  double latitude_y = shumate_map_source_get_y (self->map_source, zoom_level, latitude);
  double longitude_x = shumate_map_source_get_x (self->map_source, zoom_level, longitude);
  int tile_size = shumate_map_source_get_tile_size (self->map_source);
  double tile_size_for_zoom = shumate_map_source_get_tile_size_at_zoom (self->map_source, zoom_level);
  double map_width = shumate_map_source_get_column_count (self->map_source, zoom_level)
    * tile_size_for_zoom;
  double map_height = shumate_map_source_get_row_count (self->map_source, zoom_level)
    * tile_size_for_zoom;
  gboolean show_tile_bounds = shumate_inspector_settings_get_show_tile_bounds (shumate_inspector_settings_get_default ());
  double scale_factor = gtk_widget_get_scale_factor (widget);

  GHashTableIter iter;
  gpointer key;
  gpointer value;

  /* Because Earth is round [citation needed], cylindrical projections like
   * Mercator wrap around at the antimeridian. Moving across the antimeridian
   * is the same as teleporting across the world: at one frame the longitude
   * is just less than 180, and the next it's just more than -180.
   *
   * ShumateMapLayer doesn't handle teleportation well. Widgets can only be
   * added/removed between frames, but animations are calculated during the
   * frame. This means that by the time we know about the new viewport location,
   * it's too late to move tiles around. recompute_grid(), which will fix the
   * problem, won't be called until after the current frame.
   *
   * To fix this, recompute_grid() remembers the most recent location
   * it saw. Then, to reduce "teleportation", here in snapshot() we render
   * the "copy" of the new location that is closest to the one from
   * recompute_grid(). This just means snapping the current location to a grid
   * translated by the old location.
   * */
  longitude_x = snap_coordinate (self->last_recompute_x * map_width, longitude_x, map_width);
  latitude_y = snap_coordinate (self->last_recompute_y * map_height, latitude_y, map_height);

  /* Scale and rotate around the center of the view */
  gtk_snapshot_save (snapshot);
  gtk_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (width / 2.0, height / 2.0));
  gtk_snapshot_rotate (snapshot, rotation * 180 / G_PI);

  gtk_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (-width / 2.0, -height / 2.0));

  g_hash_table_iter_init (&iter, self->tile_children);

  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      ShumateGridPosition *pos = key;
      TileChild *tile_child = value;
      GdkPaintable *paintable = shumate_tile_get_paintable (tile_child->tile);
      double size = tile_size * pow (2, zoom_level - pos->zoom);
      double x = -(longitude_x - width/2.0) + size * pos->x;
      double y = -(latitude_y - height/2.0) + size * pos->y;

      if (paintable == NULL)
        continue;

      gtk_snapshot_save (snapshot);

      gtk_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (
        round_px (x, scale_factor),
        round_px (y, scale_factor)
      ));

      gdk_paintable_snapshot (
        paintable,
        snapshot,
        round_px (x + size, scale_factor) - round_px (x, scale_factor),
        round_px (y + size, scale_factor) - round_px (y, scale_factor)
      );

      if (show_tile_bounds)
        {
          g_autofree char *text = NULL;
          g_autoptr(PangoLayout) layout = NULL;
          int x = shumate_tile_get_x (tile_child->tile);
          GdkRGBA color = {1, 0, 1, 1};

          if (x == pos->x)
            text = g_strdup_printf (" %d, %d, %d", pos->zoom, pos->x, pos->y);
          else
            text = g_strdup_printf (" %d, %d (%d), %d", pos->zoom, x, pos->x, pos->y);

          layout = gtk_widget_create_pango_layout (widget, text);
          gtk_snapshot_append_layout (snapshot, layout, &color);
          gtk_snapshot_append_border (snapshot,
                                      &GSK_ROUNDED_RECT_INIT (0, 0, size, size),
                                      (float[]){1, 1, 1, 1},
                                      (GdkRGBA[]) { color, color, color, color });
        }

      gtk_snapshot_restore (snapshot);
    }

  gtk_snapshot_restore (snapshot);

#ifdef SHUMATE_HAS_VECTOR_RENDERER
  gtk_widget_snapshot_child (widget, GTK_WIDGET (self->symbols), snapshot);
#endif
}

static char *
shumate_map_layer_get_debug_text (ShumateLayer *layer)
{
  ShumateMapLayer *self = SHUMATE_MAP_LAYER (layer);
  g_autoptr(GString) string = g_string_new ("");
  int n_loading = 0;
  GHashTableIter iter;
  TileChild *tile_child;

  g_hash_table_iter_init (&iter, self->tile_children);
  while (g_hash_table_iter_next (&iter, NULL, (gpointer *)&tile_child))
    {
      if (shumate_tile_get_state (tile_child->tile) != SHUMATE_STATE_DONE)
        n_loading ++;
    }

  g_string_append_printf (string,
                          "tiles: %d, %d loading\n",
                          g_hash_table_size (self->tile_children),
                          n_loading);

#ifdef SHUMATE_HAS_VECTOR_RENDERER
  {
    g_autofree char *symbol_debug = shumate_vector_symbol_container_get_debug_text (self->symbols);
    g_string_append (string, symbol_debug);
  }
#endif

  if (self->deferring)
    g_string_append (string, "deferring\n");

  return g_string_free_and_steal (g_steal_pointer (&string));
}

static void
shumate_map_layer_class_init (ShumateMapLayerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  ShumateLayerClass *layer_class = SHUMATE_LAYER_CLASS (klass);

  object_class->set_property = shumate_map_layer_set_property;
  object_class->get_property = shumate_map_layer_get_property;
  object_class->dispose = shumate_map_layer_dispose;
  object_class->constructed = shumate_map_layer_constructed;

  widget_class->size_allocate = shumate_map_layer_size_allocate;
  widget_class->snapshot = shumate_map_layer_snapshot;
  widget_class->measure = shumate_map_layer_measure;

  layer_class->get_debug_text = shumate_map_layer_get_debug_text;

  obj_properties[PROP_MAP_SOURCE] =
    g_param_spec_object ("map-source",
                         "Map Source",
                         "The Map Source",
                         SHUMATE_TYPE_MAP_SOURCE,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class,
                                     N_PROPERTIES,
                                     obj_properties);

  /**
   * ShumateMapLayer::symbol-clicked:
   * @self: the [class@MapLayer] emitting the signal
   * @event: a [class@SymbolEvent] with details about the clicked symbol.
   *
   * Emitted when a symbol in the map layer is clicked.
   *
   * Since: 1.1
   */
  signals[SYMBOL_CLICKED] =
    g_signal_new ("symbol-clicked",
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE,
                  1,
                  SHUMATE_TYPE_SYMBOL_EVENT);

  /**
   * ShumateMapLayer::map-loaded:
   * @self: the [class@MapLayer] emitting the signal
   * @errors: `TRUE` if any visible tiles failed to load
   *
   * Emitted when all the tiles in the map view are finished loading.
   *
   * As the map is panned or zoomed, this signal may be emitted multiple times.
   *
   * Since: 1.4
   */
  signals[MAP_LOADED] =
    g_signal_new ("map-loaded",
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__BOOLEAN,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_BOOLEAN);

  /**
   * ShumateMapLayer::tile-error:
   * @self: the [class@MapLayer] emitting the signal
   * @tile: the [class@Tile] that failed to load
   * @error: the error that occurred
   *
   * Emitted when a tile fails to load.
   *
   * Since: 1.4
   */
  signals[TILE_ERROR] =
    g_signal_new ("tile-error",
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  _shumate_marshal_VOID__OBJECT_BOXED,
                  G_TYPE_NONE,
                  2,
                  SHUMATE_TYPE_TILE,
                  G_TYPE_ERROR);
}

static void
shumate_map_layer_init (ShumateMapLayer *self)
{
  self->tile_children = g_hash_table_new_full (
    shumate_grid_position_hash,
    shumate_grid_position_equal,
    shumate_grid_position_free,
    (GDestroyNotify)tile_child_free
  );
  self->memcache = shumate_memory_cache_new_full (100);
}

ShumateMapLayer *
shumate_map_layer_new (ShumateMapSource *map_source,
                       ShumateViewport  *viewport)
{
  return g_object_new (SHUMATE_TYPE_MAP_LAYER,
                       "map-source", map_source,
                       "viewport", viewport,
                       NULL);
}

