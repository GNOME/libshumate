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
#include "shumate-memory-cache.h"

struct _ShumateMapLayer
{
  ShumateLayer parent_instance;

  ShumateMapSource *map_source;

  GHashTable *tile_children;
  int tile_initial_row;
  int tile_initial_column;
  int required_tiles_rows;
  int required_tiles_columns;
  GHashTable *tile_fill;

  guint recompute_grid_idle_id;

  ShumateMemoryCache *memcache;
};

G_DEFINE_TYPE (ShumateMapLayer, shumate_map_layer, SHUMATE_TYPE_LAYER)

enum
{
  PROP_MAP_SOURCE = 1,
  N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

/* This struct represents the location of a tile on the screen. It is the key
 * for the hash table tile_children which stores all visible tiles.
 *
 * Note that, unlike the values given to ShumateTile, the x and y coordinates
 * here are *not* wrapped. For example, a ShumateTile at level 3 might have
 * coordinates of (7, 2) but have a TileGridPosition of (-1, 2). */

typedef struct
{
  int x;
  int y;
  int zoom;
} TileGridPosition;

static void
tile_grid_position_init (TileGridPosition *self,
                         int x,
                         int y,
                         int zoom)
{
  self->x = x;
  self->y = y;
  self->zoom = zoom;
}

static TileGridPosition *
tile_grid_position_new (int x,
                        int y,
                        int zoom)
{
  TileGridPosition *self = g_new0 (TileGridPosition, 1);
  tile_grid_position_init (self, x, y, zoom);
  return self;
}

static guint
tile_grid_position_hash (gconstpointer pointer)
{
  const TileGridPosition *self = pointer;
  return self->x ^ self->y ^ self->zoom;
}

static gboolean
tile_grid_position_equal (gconstpointer a, gconstpointer b)
{
  const TileGridPosition *pos_a = a;
  const TileGridPosition *pos_b = b;
  return pos_a->x == pos_b->x && pos_a->y == pos_b->y && pos_a->zoom == pos_b->zoom;
}

static void
tile_grid_position_free (gpointer pointer)
{
  TileGridPosition *self = pointer;
  if (!self)
    return;

  g_free (self);
}

G_DEFINE_AUTOPTR_CLEANUP_FUNC (TileGridPosition, tile_grid_position_free);

static int
positive_mod (int i, int n)
{
  return (i % n + n) % n;
}

typedef struct {
  ShumateMapLayer *self;
  ShumateTile *tile;
  char *source_id;
} TileFilledData;

static void
tile_filled_data_free (TileFilledData *data)
{
  g_clear_object (&data->self);
  g_clear_object (&data->tile);
  g_clear_pointer (&data->source_id, g_free);
  g_free (data);
}
G_DEFINE_AUTOPTR_CLEANUP_FUNC (TileFilledData, tile_filled_data_free);

static void
on_tile_filled (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  g_autoptr(TileFilledData) data = user_data;
  g_autoptr(GError) error = NULL;
  gboolean success;

  success = shumate_map_source_fill_tile_finish (SHUMATE_MAP_SOURCE (source_object), res, &error);

  // TODO: Report the error
  if (!success)
    return;

  shumate_memory_cache_store_texture (data->self->memcache,
                                      data->tile,
                                      shumate_tile_get_texture (data->tile),
                                      data->source_id);
}

static void
add_tile (ShumateMapLayer *self,
          ShumateTile     *tile)
{
  const char *source_id = shumate_map_source_get_id (self->map_source);

  if (!shumate_memory_cache_try_fill_tile (self->memcache, tile, source_id))
    {
      GCancellable *cancellable = g_cancellable_new ();
      TileFilledData *data = g_new0 (TileFilledData, 1);
      data->self = g_object_ref (self);
      data->tile = g_object_ref (tile);
      data->source_id = g_strdup (source_id);

      shumate_tile_set_texture (tile, NULL);
      shumate_map_source_fill_tile_async (self->map_source, tile, cancellable, on_tile_filled, data);
      g_hash_table_insert (self->tile_fill, g_object_ref (tile), cancellable);
    }

  gtk_widget_insert_before (GTK_WIDGET (tile), GTK_WIDGET (self), NULL);
}

static void
remove_tile (ShumateMapLayer *self,
             ShumateTile     *tile)
{
  GCancellable *cancellable = g_hash_table_lookup (self->tile_fill, tile);
  if (cancellable)
    {
      g_cancellable_cancel (cancellable);
      g_hash_table_remove (self->tile_fill, tile);
    }

  gtk_widget_unparent (GTK_WIDGET (tile));
}

static void
recompute_grid (ShumateMapLayer *self)
{
  /* Computes which tile positions are visible, ensures that all the right
   * tiles are added to the ShumateMapLayer widget, and removes tiles which are
   * no longer visible. */

  GHashTableIter iter;
  gpointer key, value;

  int width = gtk_widget_get_width (GTK_WIDGET (self));
  int height = gtk_widget_get_height (GTK_WIDGET (self));
  ShumateViewport *viewport = shumate_layer_get_viewport (SHUMATE_LAYER (self));
  int tile_size = shumate_map_source_get_tile_size (self->map_source);
  int zoom_level = shumate_viewport_get_zoom_level (viewport);
  double latitude = shumate_location_get_latitude (SHUMATE_LOCATION (viewport));
  double longitude = shumate_location_get_longitude (SHUMATE_LOCATION (viewport));
  int latitude_y = shumate_map_source_get_y (self->map_source, zoom_level, latitude);
  int longitude_x = shumate_map_source_get_x (self->map_source, zoom_level, longitude);
  int source_rows = shumate_map_source_get_row_count (self->map_source, zoom_level);
  int source_columns = shumate_map_source_get_column_count (self->map_source, zoom_level);

  // This is the (column, row) of the top left tile
  int tile_initial_column = floor ((longitude_x - width/2) / (double) tile_size);
  int tile_initial_row = floor ((latitude_y - height/2) / (double) tile_size);

  int required_columns = (width / tile_size) + 2;
  int required_rows = (height / tile_size) + 2;

  gboolean all_filled = TRUE;

  /* First, remove all the tiles that aren't in bounds. For now, ignore tiles
   * that aren't on the current zoom level--those are only removed once the
   * current level is fully loaded */
  g_hash_table_iter_init (&iter, self->tile_children);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      TileGridPosition *pos = key;
      ShumateTile *tile = value;

      if ((pos->x < tile_initial_column
          || pos->x >= tile_initial_column + required_columns
          || pos->y < tile_initial_row
          || pos->y >= tile_initial_row + required_rows)
          && pos->zoom == zoom_level)
        {
          remove_tile (self, tile);
          g_hash_table_iter_remove (&iter);
        }
    }

  /* Next, make sure every visible tile position has a matching ShumateTile
   * widget. */
  for (int x = tile_initial_column; x < tile_initial_column + required_columns; x ++)
    {
      for (int y = tile_initial_row; y < tile_initial_row + required_rows; y ++)
        {
          g_autoptr(TileGridPosition) pos = tile_grid_position_new (x, y, zoom_level);
          ShumateTile *tile = g_hash_table_lookup (self->tile_children, pos);

          if (!tile)
            {
              tile = shumate_tile_new_full (positive_mod (x, source_columns), positive_mod (y, source_rows), tile_size, zoom_level);
              g_hash_table_insert (self->tile_children, g_steal_pointer (&pos), g_object_ref (tile));
              add_tile (self, tile);
            }

          if (shumate_tile_get_state (tile) != SHUMATE_STATE_DONE)
            all_filled = FALSE;
        }
    }

  /* If all the tiles on the current zoom level are filled, delete tiles on all
   * other zoom levels */
  if (all_filled)
    {
      g_hash_table_iter_init (&iter, self->tile_children);
      while (g_hash_table_iter_next (&iter, &key, &value))
        {
          TileGridPosition *pos = key;
          ShumateTile *tile = value;

          if (pos->zoom != zoom_level)
            {
              remove_tile (self, tile);
              g_hash_table_iter_remove (&iter);
            }
        }
    }

  self->tile_initial_column = tile_initial_column;
  self->tile_initial_row = tile_initial_row;
  self->required_tiles_columns = required_columns;
  self->required_tiles_rows = required_rows;
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
  g_assert (SHUMATE_IS_MAP_LAYER (self));

  if (self->recompute_grid_idle_id > 0)
    return;

  self->recompute_grid_idle_id = g_idle_add (recompute_grid_in_idle_cb, self);
  g_source_set_name_by_id (self->recompute_grid_idle_id,
                           "[shumate] recompute_grid_in_idle_cb");
}

static void
on_view_longitude_changed (ShumateMapLayer *self,
                           GParamSpec      *pspec,
                           ShumateViewport *view)
{
  g_assert (SHUMATE_IS_MAP_LAYER (self));

  recompute_grid (self);
  gtk_widget_queue_allocate (GTK_WIDGET (self));
}

static void
on_view_latitude_changed (ShumateMapLayer *self,
                          GParamSpec      *pspec,
                          ShumateViewport *view)
{
  g_assert (SHUMATE_IS_MAP_LAYER (self));

  recompute_grid (self);
  gtk_widget_queue_allocate (GTK_WIDGET (self));
}

static void
on_view_zoom_level_changed (ShumateMapLayer *self,
                            GParamSpec      *pspec,
                            ShumateViewport *view)
{
  g_assert (SHUMATE_IS_MAP_LAYER (self));

  recompute_grid (self);
  gtk_widget_queue_allocate (GTK_WIDGET (self));
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
  g_clear_pointer (&self->tile_fill, g_hash_table_unref);
  g_clear_pointer (&self->tile_children, g_hash_table_unref);
  g_clear_object (&self->map_source);
  g_clear_object (&self->memcache);

  G_OBJECT_CLASS (shumate_map_layer_parent_class)->dispose (object);
}

static void
shumate_map_layer_constructed (GObject *object)
{
  ShumateMapLayer *self = SHUMATE_MAP_LAYER (object);
  ShumateViewport *viewport;

  G_OBJECT_CLASS (shumate_map_layer_parent_class)->constructed (object);

  viewport = shumate_layer_get_viewport (SHUMATE_LAYER (self));
  g_signal_connect_swapped (viewport, "notify::longitude", G_CALLBACK (on_view_longitude_changed), self);
  g_signal_connect_swapped (viewport, "notify::latitude", G_CALLBACK (on_view_latitude_changed), self);
  g_signal_connect_swapped (viewport, "notify::zoom-level", G_CALLBACK (on_view_zoom_level_changed), self);
}

static void
shumate_map_layer_size_allocate (GtkWidget *widget,
                                 int        width,
                                 int        height,
                                 int        baseline)
{
  ShumateMapLayer *self = SHUMATE_MAP_LAYER (widget);
  ShumateViewport *viewport;
  GtkAllocation child_allocation;
  int tile_size;
  int zoom_level;
  double latitude, longitude;
  int longitude_x, latitude_y;

  GHashTableIter iter;
  gpointer key, value;

  viewport = shumate_layer_get_viewport (SHUMATE_LAYER (self));
  tile_size = shumate_map_source_get_tile_size (self->map_source);
  zoom_level = (guint) shumate_viewport_get_zoom_level (viewport);
  latitude = shumate_location_get_latitude (SHUMATE_LOCATION (viewport));
  longitude = shumate_location_get_longitude (SHUMATE_LOCATION (viewport));
  latitude_y = (guint) shumate_map_source_get_y (self->map_source, zoom_level, latitude);
  longitude_x = (guint) shumate_map_source_get_x (self->map_source, zoom_level, longitude);

  g_hash_table_iter_init (&iter, self->tile_children);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      TileGridPosition *pos = key;
      ShumateTile *tile = value;

      child_allocation.width = tile_size * pow (2, zoom_level - pos->zoom);
      child_allocation.height = child_allocation.width;
      child_allocation.x = -(longitude_x - width/2) + child_allocation.width * pos->x;
      child_allocation.y = -(latitude_y - height/2) + child_allocation.height * pos->y;
      gtk_widget_size_allocate (GTK_WIDGET (tile), &child_allocation, baseline);
    }

  /* We can't recompute while allocating, so queue an idle callback to run
   * the recomputation outside the allocation cycle.
   */
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
  ShumateMapLayer *self = SHUMATE_MAP_LAYER (widget);

  if (minimum)
    *minimum = 0;

  if (natural)
    {
      ShumateViewport *viewport;
      double tile_size;
      guint zoom_level;
      guint count;

      viewport = shumate_layer_get_viewport (SHUMATE_LAYER (self));
      zoom_level = shumate_viewport_get_zoom_level (viewport);
      tile_size = shumate_map_source_get_tile_size (self->map_source) * (fmod (zoom_level, 1.0) + 1.0);
      if (orientation == GTK_ORIENTATION_HORIZONTAL)
        count = shumate_map_source_get_column_count (self->map_source, zoom_level);
      else
        count = shumate_map_source_get_row_count (self->map_source, zoom_level);

      *natural = count * tile_size;
    }
}

static void
shumate_map_layer_snapshot (GtkWidget *widget, GtkSnapshot *snapshot)
{
  ShumateMapLayer *self = SHUMATE_MAP_LAYER (widget);
  ShumateViewport *viewport = shumate_layer_get_viewport (SHUMATE_LAYER (self));
  double zoom_level = shumate_viewport_get_zoom_level (viewport);
  double extra_zoom = fmod (zoom_level, 1.0) + 1.0;
  int width = gtk_widget_get_width (GTK_WIDGET (self));
  int height = gtk_widget_get_height (GTK_WIDGET (self));

  /* Scale around the center of the view */
  gtk_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (width / 2.0, height / 2.0));
  gtk_snapshot_scale (snapshot, extra_zoom, extra_zoom);
  gtk_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (-width / 2.0, -height / 2.0));

  GTK_WIDGET_CLASS (shumate_map_layer_parent_class)->snapshot (widget, snapshot);
}

static void
shumate_map_layer_class_init (ShumateMapLayerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->set_property = shumate_map_layer_set_property;
  object_class->get_property = shumate_map_layer_get_property;
  object_class->dispose = shumate_map_layer_dispose;
  object_class->constructed = shumate_map_layer_constructed;

  widget_class->size_allocate = shumate_map_layer_size_allocate;
  widget_class->snapshot = shumate_map_layer_snapshot;
  widget_class->measure = shumate_map_layer_measure;

  obj_properties[PROP_MAP_SOURCE] =
    g_param_spec_object ("map-source",
                         "Map Source",
                         "The Map Source",
                         SHUMATE_TYPE_MAP_SOURCE,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class,
                                     N_PROPERTIES,
                                     obj_properties);
}

static void
shumate_map_layer_init (ShumateMapLayer *self)
{
  self->tile_children = g_hash_table_new_full (tile_grid_position_hash, tile_grid_position_equal, tile_grid_position_free, g_object_unref);
  self->tile_fill = g_hash_table_new_full (g_direct_hash, g_direct_equal, g_object_unref, g_object_unref);
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
