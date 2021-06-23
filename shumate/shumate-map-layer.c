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
#include "shumate-view.h"
#include "shumate-memory-cache.h"

struct _ShumateMapLayer
{
  ShumateLayer parent_instance;

  ShumateMapSource *map_source;

  GHashTable *tile_children;
  guint required_tiles_rows;
  guint required_tiles_columns;
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

typedef struct
{
  guint left_attach;
  guint top_attach;
} TileGridPosition;

static void
tile_grid_position_init (TileGridPosition *self,
                         guint left_attach,
                         guint top_attach)
{
  self->left_attach = left_attach;
  self->top_attach = top_attach;
}

static TileGridPosition *
tile_grid_position_new (guint left_attach,
                        guint top_attach)
{
  TileGridPosition *self = g_new0 (TileGridPosition, 1);
  tile_grid_position_init (self, left_attach, top_attach);
  return self;
}

static guint
tile_grid_position_hash (gconstpointer pointer)
{
  const TileGridPosition *self = pointer;
  return self->left_attach ^ self->top_attach;
}

static gboolean
tile_grid_position_equal (gconstpointer a, gconstpointer b)
{
  const TileGridPosition *pos_a = a;
  const TileGridPosition *pos_b = b;
  return pos_a->left_attach == pos_b->left_attach && pos_a->top_attach == pos_b->top_attach;
}

static void
tile_grid_position_free (gpointer pointer)
{
  TileGridPosition *self = pointer;
  if (!self)
    return;

  self->left_attach = 0;
  self->top_attach = 0;
  g_free (self);
}

G_DEFINE_AUTOPTR_CLEANUP_FUNC (TileGridPosition, tile_grid_position_free);

static ShumateTile *
shumate_map_layer_get_tile_child (ShumateMapLayer *self,
                                  guint            left_attach,
                                  guint            top_attach)
{
  TileGridPosition pos;
  tile_grid_position_init (&pos, left_attach, top_attach);
  return g_hash_table_lookup (self->tile_children, &pos);
}

static inline int
modadd (int current,
        int shift,
        int size)
{
  /* This is scary, but the idea behind it is simple: the regular modulo operator
   * does *not* wrap around when giving it negative numbers. For example, -1 % 8
   * yields -1 instead of 7.
   *
   * The following pair of lines do exactly that, on top of adding a number. This
   * is so that we can do e.g. (0 + -1) % 8 = 7.
   */
  current = current % size;
  return (size + current + shift) % size;
}

static void
maybe_shift_grid (ShumateMapLayer *self,
                  guint            n_columns,
                  guint            n_rows,
                  int              new_first_tile_column,
                  int              new_first_tile_row)
{
  ShumateTile *first_tile;
  int first_tile_column;
  int first_tile_row;
  int column_backward_diff;
  int column_forward_diff;
  int row_backward_diff;
  int row_forward_diff;
  int column_diff;
  int row_diff;
  GHashTableIter iter;
  gpointer key, value;
  GHashTable *new_tile_children;

  first_tile = shumate_map_layer_get_tile_child (self, 0, 0);
  if (!first_tile)
    return;

  first_tile_column = shumate_tile_get_x (first_tile);
  first_tile_row = shumate_tile_get_y (first_tile);

  /* The allocation function uses unsigned ints everywhere, and they do wrap
   * around, so we can often receive super large values here.
   */
  new_first_tile_column = new_first_tile_column % n_columns;
  new_first_tile_row = new_first_tile_row % n_rows;

  if (new_first_tile_column == first_tile_column && new_first_tile_row == first_tile_row)
    return;

  /* This too looks more complicated than it is. Because all the calculations
   * are modular, we check which is closest: moving forward or backward.
   *
   * For example, in a 8x8 grid, if the first tile is going from 7x0 to 0x0,
   * the forward diff is 7x0, and the backward diff is 1x0. We want to pick
   * the backward diff in this case.
   */
  column_backward_diff = (new_first_tile_column - first_tile_column) % n_columns;
  column_forward_diff = (first_tile_column - new_first_tile_column) % n_columns;

  row_backward_diff = (new_first_tile_row - first_tile_row) % n_rows;
  row_forward_diff = (first_tile_row - new_first_tile_row) % n_rows;

  column_diff = ABS (column_backward_diff) < ABS (column_forward_diff) ? column_backward_diff : -column_forward_diff;
  row_diff = ABS (row_backward_diff) < ABS (row_forward_diff) ? row_backward_diff : -row_forward_diff;

  /* If the diff is bigger than the number of tiles being displayed in any axis,
   * there's no point shifting them; they all will need to reload.
   */
  if (ABS (column_diff) >= self->required_tiles_columns || ABS (row_diff) >= self->required_tiles_rows)
    return;

  new_tile_children = g_hash_table_new_full (tile_grid_position_hash, tile_grid_position_equal, tile_grid_position_free, g_object_unref);
  g_hash_table_iter_init (&iter, self->tile_children);

  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      TileGridPosition *pos = key;
      ShumateTile *tile = value;

      g_hash_table_iter_steal (&iter);

      pos->left_attach = modadd (pos->left_attach,
                                 -column_diff,
                                 self->required_tiles_columns);

      pos->top_attach = modadd (pos->top_attach,
                                -row_diff,
                                self->required_tiles_rows);

      g_hash_table_insert (new_tile_children, pos, tile);
    }

  g_clear_pointer (&self->tile_children, g_hash_table_unref);
  self->tile_children = new_tile_children;
}

static void
recompute_grid (ShumateMapLayer *self,
                int              width,
                int              height)
{
  GtkWidget *widget = GTK_WIDGET (self);
  guint required_tiles_columns, required_tiles_rows;
  guint tile_size;

  tile_size = shumate_map_source_get_tile_size (self->map_source);
  required_tiles_columns = (width / tile_size) + 2;
  required_tiles_rows = (height / tile_size) + 2;
  if (self->required_tiles_columns != required_tiles_columns)
    {
      if (required_tiles_columns > self->required_tiles_columns)
        {
          for (guint column = self->required_tiles_columns;
               column < required_tiles_columns;
               column++)
            {
              for (guint row = 0; row < self->required_tiles_rows; row++)
                {
                  ShumateTile *tile;
                  TileGridPosition *pos;

                  tile = shumate_tile_new ();
                  shumate_tile_set_size (tile, tile_size);
                  gtk_widget_insert_before (GTK_WIDGET (tile), widget, NULL);
                  pos = tile_grid_position_new (column, row);
                  g_hash_table_insert (self->tile_children, pos, g_object_ref (tile));
                }
            }
        }
      else
        {
          for (guint column = self->required_tiles_columns - 1;
               column>= required_tiles_columns;
               column--)
            {
              for (guint row = 0; row < self->required_tiles_rows; row++)
                {
                  TileGridPosition pos;
                  ShumateTile *tile;

                  tile_grid_position_init (&pos, column, row);
                  tile = g_hash_table_lookup (self->tile_children, &pos);

                  if (!tile)
                    {
                      g_critical ("Unable to find tile to remove at (%u;%u)", column, row);
                      continue;
                    }

                  gtk_widget_unparent (GTK_WIDGET (tile));
                  g_hash_table_remove (self->tile_children, &pos);
                }
            }
        }

      self->required_tiles_columns = required_tiles_columns;
    }

  if (self->required_tiles_rows != required_tiles_rows)
    {
      if (required_tiles_rows > self->required_tiles_rows)
        {
          for (guint column = 0; column < self->required_tiles_columns; column++)
            {
              for (guint row = self->required_tiles_rows; row < required_tiles_rows; row++)
                {
                  ShumateTile *tile;
                  TileGridPosition *pos;

                  tile = shumate_tile_new ();
                  shumate_tile_set_size (tile, tile_size);
                  gtk_widget_insert_before (GTK_WIDGET (tile), widget, NULL);
                  pos = tile_grid_position_new (column, row);
                  g_hash_table_insert (self->tile_children, pos, g_object_ref (tile));
                }
            }
        }
      else
        {
          for (guint column = 0; column < self->required_tiles_columns; column++)
            {
              for (guint row = self->required_tiles_rows - 1; row >= required_tiles_rows; row--)
                {
                  TileGridPosition pos;
                  ShumateTile *tile;

                  tile_grid_position_init (&pos, column, row);
                  tile = g_hash_table_lookup (self->tile_children, &pos);

                  if (!tile)
                    {
                      g_critical ("Unable to find tile to remove at (%u;%u)", column, row);
                      continue;
                    }

                  gtk_widget_unparent (GTK_WIDGET (tile));
                  g_hash_table_remove (self->tile_children, &pos);
                }
            }
        }

      self->required_tiles_rows = required_tiles_rows;
    }
}

static gboolean
grid_needs_recompute (ShumateMapLayer *self,
                      int              width,
                      int              height)
{
  guint required_tiles_columns, required_tiles_rows;
  guint tile_size;

  g_assert (SHUMATE_IS_MAP_LAYER (self));

  tile_size = shumate_map_source_get_tile_size (self->map_source);
  required_tiles_columns = (width / tile_size) + 2;
  required_tiles_rows = (height / tile_size) + 2;

  return self->required_tiles_columns != required_tiles_columns ||
         self->required_tiles_rows != required_tiles_rows;
}

static void
maybe_recompute_grid (ShumateMapLayer *self)
{
  int width, height;

  g_assert (SHUMATE_IS_MAP_LAYER (self));

  width = gtk_widget_get_width (GTK_WIDGET (self));
  height = gtk_widget_get_height (GTK_WIDGET (self));

  if (grid_needs_recompute (self, width, height))
    recompute_grid (self, width, height);
}

static gboolean
recompute_grid_in_idle_cb (gpointer user_data)
{
  ShumateMapLayer *self = user_data;

  g_assert (SHUMATE_IS_MAP_LAYER (self));

  maybe_recompute_grid (self);

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

  maybe_recompute_grid (self);
  gtk_widget_queue_allocate (GTK_WIDGET (self));
}

static void
on_view_latitude_changed (ShumateMapLayer *self,
                          GParamSpec      *pspec,
                          ShumateViewport *view)
{
  g_assert (SHUMATE_IS_MAP_LAYER (self));

  maybe_recompute_grid (self);
  gtk_widget_queue_allocate (GTK_WIDGET (self));
}

static void
on_view_zoom_level_changed (ShumateMapLayer *self,
                            GParamSpec      *pspec,
                            ShumateViewport *view)
{
  g_assert (SHUMATE_IS_MAP_LAYER (self));

  maybe_recompute_grid (self);
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
shumate_map_layer_size_allocate (GtkWidget *widget,
                                 int        width,
                                 int        height,
                                 int        baseline)
{
  ShumateMapLayer *self = SHUMATE_MAP_LAYER (widget);
  ShumateViewport *viewport;
  GtkAllocation child_allocation;
  guint tile_size;
  guint zoom_level;
  double latitude, longitude;
  guint longitude_x, latitude_y;
  int x_offset, y_offset;
  guint tile_column, tile_row;
  guint tile_initial_column, tile_initial_row;
  guint source_rows, source_columns;

  viewport = shumate_layer_get_viewport (SHUMATE_LAYER (self));
  tile_size = shumate_map_source_get_tile_size (self->map_source);
  zoom_level = (guint) shumate_viewport_get_zoom_level (viewport);
  latitude = shumate_location_get_latitude (SHUMATE_LOCATION (viewport));
  longitude = shumate_location_get_longitude (SHUMATE_LOCATION (viewport));
  latitude_y = (guint) shumate_map_source_get_y (self->map_source, zoom_level, latitude);
  longitude_x = (guint) shumate_map_source_get_x (self->map_source, zoom_level, longitude);
  source_rows = shumate_map_source_get_row_count (self->map_source, zoom_level);
  source_columns = shumate_map_source_get_column_count (self->map_source, zoom_level);

  // This is the (column,row) of the top left ShumateTile
  tile_initial_row = (latitude_y - height/2)/tile_size;
  tile_initial_column = (longitude_x - width/2)/tile_size;

  x_offset = (longitude_x - tile_initial_column * tile_size) - width/2;
  y_offset = (latitude_y - tile_initial_row * tile_size) - height/2;
  child_allocation.y = -y_offset;
  child_allocation.width = tile_size;
  child_allocation.height = tile_size;

  maybe_shift_grid (self,
                    source_columns, source_rows,
                    tile_initial_column, tile_initial_row);

  tile_row = tile_initial_row;
  for (int row = 0; row < self->required_tiles_rows; row++)
    {
      child_allocation.x = -x_offset;
      tile_column = tile_initial_column;
      for (int column = 0; column < self->required_tiles_columns; column++)
        {
          ShumateTile *child;

          child = shumate_map_layer_get_tile_child (self, column, row);
          if (!child)
            {
              g_critical ("Unable to find tile at (%u;%u)", column, row);
              continue;
            }

          gtk_widget_measure (GTK_WIDGET (child), GTK_ORIENTATION_HORIZONTAL, 0, NULL, NULL, NULL, NULL);
          gtk_widget_size_allocate (GTK_WIDGET (child), &child_allocation, baseline);

          if (shumate_tile_get_zoom_level (child) != zoom_level ||
              shumate_tile_get_x (child) != (tile_column % source_columns) ||
              shumate_tile_get_y (child) != (tile_row % source_rows) ||
              shumate_tile_get_state (child) == SHUMATE_STATE_NONE)
            {
              GCancellable *cancellable = g_hash_table_lookup (self->tile_fill, child);
              const char *source_id = shumate_map_source_get_id (self->map_source);

              if (cancellable)
                g_cancellable_cancel (cancellable);

              shumate_tile_set_zoom_level (child, zoom_level);
              shumate_tile_set_x (child, tile_column % source_columns);
              shumate_tile_set_y (child, tile_row % source_rows);

              if (!shumate_memory_cache_try_fill_tile (self->memcache, child, source_id))
                {
                  TileFilledData *data = g_new0 (TileFilledData, 1);
                  data->self = g_object_ref (self);
                  data->tile = g_object_ref (child);
                  data->source_id = g_strdup (source_id);

                  cancellable = g_cancellable_new ();
                  shumate_tile_set_texture (child, NULL);
                  shumate_map_source_fill_tile_async (self->map_source, child, cancellable, on_tile_filled, data);
                  g_hash_table_insert (self->tile_fill, g_object_ref (child), cancellable);
                }
            }

          child_allocation.x += tile_size;
          tile_column++;
        }

      child_allocation.y += tile_size;
      tile_row++;
    }

  /* We can't recompute while allocating, so queue an idle callback to run
   * the recomputation outside the allocation cycle.
   */
  if (grid_needs_recompute (self, width, height))
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
