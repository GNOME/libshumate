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

struct _ShumateMapLayer
{
  ShumateLayer parent_instance;

  ShumateMapSource *map_source;

  GPtrArray *tiles_positions;
  guint required_tiles_x;
  guint required_tiles_y;
  GHashTable *tile_fill;
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
  ShumateTile *tile;
  guint left_attach;
  guint top_attach;
} TileGridPosition;

static TileGridPosition *
tile_grid_position_new (ShumateTile *tile,
                        guint        left_attach,
                        guint        top_attach)
{
  TileGridPosition *self = g_new0 (TileGridPosition, 1);
  self->tile = g_object_ref (tile);
  self->left_attach = left_attach;
  self->top_attach = top_attach;
  return self;
}

static void
tile_grid_position_free (TileGridPosition *self)
{
  if (!self)
    return;

  g_clear_object (&self->tile);
  self->left_attach = 0;
  self->top_attach = 0;
  g_free (self);
}

G_DEFINE_AUTOPTR_CLEANUP_FUNC (TileGridPosition, tile_grid_position_free);

static TileGridPosition *
shumate_map_layer_get_tile_child (ShumateMapLayer *self,
                                  guint            left_attach,
                                  guint            top_attach)
{
  for (guint index = 0; index < self->tiles_positions->len; index++)
    {
      TileGridPosition *tile_child = g_ptr_array_index (self->tiles_positions, index);
      if (tile_child->left_attach == left_attach && tile_child->top_attach == top_attach)
        return tile_child;
    }

  return NULL;
}

static void
shumate_map_layer_compute_grid (ShumateMapLayer *self)
{
  guint tile_size;
  guint zoom_level;
  double center_latitude, center_longitude;
  guint center_x, center_y;
  int x_offset, y_offset;
  guint tile_x, tile_y;
  guint tile_initial_x, tile_initial_y;
  guint source_rows, source_columns;
  int width, height;
  GtkAllocation child_allocation;
  ShumateViewport *viewport;

  g_assert (SHUMATE_IS_MAP_LAYER (self));

  viewport = shumate_layer_get_viewport (SHUMATE_LAYER (self));
  tile_size = shumate_map_source_get_tile_size (self->map_source);
  zoom_level = shumate_viewport_get_zoom_level (viewport);
  center_latitude = shumate_location_get_latitude (SHUMATE_LOCATION (viewport));
  center_longitude = shumate_location_get_longitude (SHUMATE_LOCATION (viewport));
  center_x = (guint) shumate_map_source_get_x (self->map_source, zoom_level, center_longitude);
  center_y = (guint) shumate_map_source_get_y (self->map_source, zoom_level, center_latitude);
  source_rows = shumate_map_source_get_row_count (self->map_source, zoom_level);
  source_columns = shumate_map_source_get_column_count (self->map_source, zoom_level);
  width = gtk_widget_get_width (GTK_WIDGET (self));
  height = gtk_widget_get_height (GTK_WIDGET (self));

  // This is the (x,y) of the top left ShumateTile
  tile_initial_x = (center_x - width/2)/tile_size;
  tile_initial_y = (center_y - height/2)/tile_size;

  x_offset = (center_x - tile_initial_x * tile_size) - width/2;
  y_offset = (center_y - tile_initial_y * tile_size) - height/2;
  child_allocation.x = -x_offset;
  child_allocation.width = tile_size;
  child_allocation.height = tile_size;

  tile_x = tile_initial_x;
  for (int x = 0; x < self->required_tiles_x; x++)
    {
      child_allocation.y = -y_offset;
      tile_y = tile_initial_y;
      for (int y = 0; y < self->required_tiles_y; y++)
        {
          TileGridPosition *tile_child;
          ShumateTile *child;

          tile_child = shumate_map_layer_get_tile_child (self, x, y);
          if (!tile_child)
            {
              g_critical ("Unable to find tile at (%u;%u)", x, y);
            }
          else
            {
              child = tile_child->tile;
              gtk_widget_measure (GTK_WIDGET (child), GTK_ORIENTATION_HORIZONTAL, 0, NULL, NULL, NULL, NULL);
              gtk_widget_size_allocate (GTK_WIDGET (child), &child_allocation, -1);
              if (shumate_tile_get_zoom_level (child) != zoom_level ||
                  shumate_tile_get_x (child) != x ||
                  shumate_tile_get_y (child) != y ||
                  shumate_tile_get_state (child) == SHUMATE_STATE_NONE)
                {
                  GCancellable *cancellable = g_hash_table_lookup (self->tile_fill, child);
                  if (cancellable)
                    g_cancellable_cancel (cancellable);

                  shumate_tile_set_zoom_level (child, zoom_level);
                  shumate_tile_set_x (child, tile_x % source_columns);
                  shumate_tile_set_y (child, tile_y % source_rows);

                  cancellable = g_cancellable_new ();
                  shumate_tile_set_texture (child, NULL);
                  shumate_map_source_fill_tile (self->map_source, child, cancellable);
                  g_hash_table_insert (self->tile_fill, g_object_ref (child), cancellable);
                }
            }

          child_allocation.y += tile_size;
          tile_y++;
        }

      child_allocation.x += tile_size;
      tile_x++;
    }
}

static void
on_view_longitude_changed (ShumateMapLayer *self,
                           GParamSpec      *pspec,
                           ShumateViewport *view)
{
  g_assert (SHUMATE_IS_MAP_LAYER (self));

  shumate_map_layer_compute_grid (self);
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
on_view_latitude_changed (ShumateMapLayer *self,
                          GParamSpec      *pspec,
                          ShumateViewport *view)
{
  g_assert (SHUMATE_IS_MAP_LAYER (self));

  shumate_map_layer_compute_grid (self);
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
on_view_zoom_level_changed (ShumateMapLayer *self,
                            GParamSpec      *pspec,
                            ShumateViewport *view)
{
  g_assert (SHUMATE_IS_MAP_LAYER (self));

  shumate_map_layer_compute_grid (self);
  gtk_widget_queue_draw (GTK_WIDGET (self));
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

  g_clear_pointer (&self->tile_fill, g_hash_table_unref);
  g_clear_pointer (&self->tiles_positions, g_ptr_array_unref);
  g_clear_object (&self->map_source);

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
  guint tile_size;
  guint required_tiles_x, required_tiles_y;

  tile_size = shumate_map_source_get_tile_size (self->map_source);
  required_tiles_x = (width / tile_size) + 2;
  required_tiles_y = (height / tile_size) + 2;
  if (self->required_tiles_x != required_tiles_x)
    {
      if (required_tiles_x > self->required_tiles_x)
        {
          for (guint x = self->required_tiles_x; x < required_tiles_x; x++)
            {
              for (guint y = 0; y < self->required_tiles_y; y++)
                {
                  ShumateTile *tile;
                  TileGridPosition *tile_child;

                  tile = shumate_tile_new ();
                  shumate_tile_set_size (tile, tile_size);
                  gtk_widget_insert_before (GTK_WIDGET (tile), widget, NULL);
                  tile_child = tile_grid_position_new (tile, x, y);
                  g_ptr_array_add (self->tiles_positions, tile_child);
                }
            }
        }
      else
        {
          for (guint x = self->required_tiles_x - 1; x >= required_tiles_x; x--)
            {
              for (guint y = 0; y < self->required_tiles_y; y++)
                {
                  TileGridPosition *tile_child = shumate_map_layer_get_tile_child (self, x, y);
                  if (!tile_child)
                    {
                      g_critical ("Unable to find tile to remove at (%u;%u)", x, y);
                      continue;
                    }

                  gtk_widget_unparent (GTK_WIDGET (tile_child->tile));
                  g_ptr_array_remove_fast (self->tiles_positions, tile_child);
                }
            }
        }

      self->required_tiles_x = required_tiles_x;
    }

  if (self->required_tiles_y != required_tiles_y)
    {
      if (required_tiles_y > self->required_tiles_y)
        {
          for (guint x = 0; x < self->required_tiles_x; x++)
            {
              for (guint y = self->required_tiles_y; y < required_tiles_y; y++)
                {
                  ShumateTile *tile;
                  TileGridPosition *tile_child;

                  tile = shumate_tile_new ();
                  shumate_tile_set_size (tile, tile_size);
                  gtk_widget_insert_before (GTK_WIDGET (tile), widget, NULL);
                  tile_child = tile_grid_position_new (tile, x, y);
                  g_ptr_array_add (self->tiles_positions, tile_child);
                }
            }
        }
      else
        {
          for (guint x = 0; x < self->required_tiles_x; x++)
            {
              for (guint y = self->required_tiles_y - 1; y >= required_tiles_y; y--)
                {
                  TileGridPosition *tile_child = shumate_map_layer_get_tile_child (self, x, y);
                  if (!tile_child)
                    {
                      g_critical ("Unable to find tile to remove at (%u;%u)", x, y);
                      continue;
                    }

                  gtk_widget_unparent (GTK_WIDGET (tile_child->tile));
                  g_ptr_array_remove_fast (self->tiles_positions, tile_child);
                }
            }
        }

      self->required_tiles_y = required_tiles_y;
    }

  shumate_map_layer_compute_grid (self);
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
      guint tile_size;
      guint zoom_level;
      guint count;

      viewport = shumate_layer_get_viewport (SHUMATE_LAYER (self));
      tile_size = shumate_map_source_get_tile_size (self->map_source);
      zoom_level = shumate_viewport_get_zoom_level (viewport);
      if (orientation == GTK_ORIENTATION_HORIZONTAL)
        count = shumate_map_source_get_column_count (self->map_source, zoom_level);
      else
        count = shumate_map_source_get_row_count (self->map_source, zoom_level);

      *natural = count * tile_size;
    }
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
  g_object_set (G_OBJECT (self),
                "overflow", GTK_OVERFLOW_HIDDEN,
                NULL);
  self->tiles_positions = g_ptr_array_new_with_free_func ((GDestroyNotify) tile_grid_position_free);
  self->tile_fill = g_hash_table_new_full (g_direct_hash, g_direct_equal, g_object_unref, g_object_unref);
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
