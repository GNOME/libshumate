/*
 * Copyright (C) 2021 James Westman <james@jwestman.net>
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
 * License along with this library; if not, see <https://www.gnu.org/licenses/>.
 */

#include "shumate-vector-symbol-container-private.h"
#include "shumate-vector-symbol-private.h"
#include "shumate-vector-collision-private.h"
#include "shumate-symbol-event-private.h"
#include "shumate-profiling-private.h"
#include "shumate-inspector-settings-private.h"

typedef struct {
  int layer_idx;
  GPtrArray *symbols;
} LayerBucket;

struct _ShumateVectorSymbolContainer
{
  ShumateLayer parent_instance;

  ShumateMapSource *map_source;

  GPtrArray *layer_buckets;
  ShumateVectorCollision *collision;

  int child_count, visible_count;

  double last_rotation;
  double last_zoom;
  double last_center_x, last_center_y;
  double last_width, last_height;
  gboolean labels_changed : 1;
};

G_DEFINE_TYPE (ShumateVectorSymbolContainer, shumate_vector_symbol_container, SHUMATE_TYPE_LAYER)

enum {
  PROP_0,
  PROP_MAP_SOURCE,
  N_PROPS,
};

static GParamSpec *obj_properties[N_PROPS] = { NULL, };

enum
{
  SYMBOL_CLICKED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };


typedef struct {
  graphene_rect_t bounds;

  // do not need to be freed because they're owned by the widget
  ShumateVectorSymbol *symbol;
  ShumateVectorSymbolInfo *symbol_info;

  // These are coordinates [0, 1) within the tile
  double x;
  double y;

  int tile_x;
  int tile_y;
  int zoom;

  gboolean visible : 1;
} ChildInfo;

static void
layer_bucket_free (LayerBucket *bucket)
{
  g_clear_pointer (&bucket->symbols, g_ptr_array_unref);
  g_free (bucket);
}

static void
add_symbol_to_layer_buckets (ShumateVectorSymbolContainer *self,
                             ChildInfo                   *info)
{
  LayerBucket *bucket;

  for (int i = 0; i < self->layer_buckets->len; i ++)
    {
      bucket = g_ptr_array_index (self->layer_buckets, i);

      if (bucket->layer_idx == info->symbol_info->details->layer_idx)
        {
          g_ptr_array_add (bucket->symbols, info);
          return;
        }
    }

  bucket = g_new0 (LayerBucket, 1);
  bucket->layer_idx = info->symbol_info->details->layer_idx;
  bucket->symbols = g_ptr_array_new_with_free_func (g_free);
  g_ptr_array_add (self->layer_buckets, bucket);

  g_ptr_array_add (bucket->symbols, info);
}

static int
child_info_compare (gconstpointer a, gconstpointer b)
{
  const ChildInfo *child_a = *(ChildInfo **)a;
  const ChildInfo *child_b = *(ChildInfo **)b;
  return child_a->symbol_info->details->symbol_sort_key - child_b->symbol_info->details->symbol_sort_key;
}

static int
layer_bucket_compare (gconstpointer a, gconstpointer b)
{
  const LayerBucket *bucket_a = *(LayerBucket **)a;
  const LayerBucket *bucket_b = *(LayerBucket **)b;
  return bucket_a->layer_idx - bucket_b->layer_idx;
}

static void
sort_layer_buckets (ShumateVectorSymbolContainer *self)
{
  g_ptr_array_sort (self->layer_buckets, (GCompareFunc)layer_bucket_compare);

  for (int i = 0; i < self->layer_buckets->len; i ++)
    {
      LayerBucket *bucket = self->layer_buckets->pdata[i];
      g_ptr_array_sort (bucket->symbols, (GCompareFunc)child_info_compare);
    }
}

ShumateVectorSymbolContainer *
shumate_vector_symbol_container_new (ShumateMapSource *map_source,
                                     ShumateViewport  *viewport)
{
  return g_object_new (SHUMATE_TYPE_VECTOR_SYMBOL_CONTAINER,
                       "map-source", map_source,
                       "viewport", viewport,
                       NULL);
}


static void
on_viewport_changed (ShumateVectorSymbolContainer  *self,
                     G_GNUC_UNUSED GParamSpec      *pspec,
                     G_GNUC_UNUSED ShumateViewport *view)
{
  gtk_widget_queue_allocate (GTK_WIDGET (self));
}


static void
shumate_vector_symbol_container_constructed (GObject *object)
{
  ShumateVectorSymbolContainer *self = (ShumateVectorSymbolContainer *)object;
  ShumateInspectorSettings *settings = shumate_inspector_settings_get_default ();
  ShumateViewport *viewport;

  G_OBJECT_CLASS (shumate_vector_symbol_container_parent_class)->constructed (object);

  self->collision = shumate_vector_collision_new ();

  viewport = shumate_layer_get_viewport (SHUMATE_LAYER (self));

  g_signal_connect_swapped (viewport, "notify::longitude", G_CALLBACK (on_viewport_changed), self);
  g_signal_connect_swapped (viewport, "notify::latitude", G_CALLBACK (on_viewport_changed), self);
  g_signal_connect_swapped (viewport, "notify::zoom-level", G_CALLBACK (on_viewport_changed), self);
  g_signal_connect_swapped (viewport, "notify::rotation", G_CALLBACK (on_viewport_changed), self);

  g_signal_connect_object (settings, "notify::show-collision-boxes", G_CALLBACK (on_viewport_changed), self, G_CONNECT_SWAPPED);
}


static void
shumate_vector_symbol_container_finalize (GObject *object)
{
  ShumateVectorSymbolContainer *self = (ShumateVectorSymbolContainer *)object;

  g_clear_pointer (&self->layer_buckets, g_ptr_array_unref);
  g_clear_pointer (&self->collision, shumate_vector_collision_free);

  G_OBJECT_CLASS (shumate_vector_symbol_container_parent_class)->finalize (object);
}


static void
shumate_vector_symbol_container_dispose (GObject *object)
{
  ShumateVectorSymbolContainer *self = (ShumateVectorSymbolContainer *)object;
  ShumateViewport *viewport = shumate_layer_get_viewport (SHUMATE_LAYER (self));
  GtkWidget *child;

  g_signal_handlers_disconnect_by_data (viewport, self);

  while ((child = gtk_widget_get_first_child (GTK_WIDGET (object))))
    gtk_widget_unparent (child);

  G_OBJECT_CLASS (shumate_vector_symbol_container_parent_class)->dispose (object);
}


static void
shumate_vector_symbol_container_get_property (GObject    *object,
                                              guint       property_id,
                                              GValue     *value,
                                              GParamSpec *pspec)
{
  ShumateVectorSymbolContainer *self = (ShumateVectorSymbolContainer *)object;

  switch (property_id)
    {
    case PROP_MAP_SOURCE:
      g_value_set_object (value, self->map_source);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}


static void
shumate_vector_symbol_container_set_property (GObject      *object,
                                              guint         property_id,
                                              const GValue *value,
                                              GParamSpec   *pspec)
{
  ShumateVectorSymbolContainer *self = SHUMATE_VECTOR_SYMBOL_CONTAINER (object);

  switch (property_id)
    {
    case PROP_MAP_SOURCE:
      g_set_object (&self->map_source, g_value_get_object (value));
      gtk_widget_queue_allocate (GTK_WIDGET (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static double
get_effective_zoom_level (ShumateMapSource *map_source, ShumateViewport *viewport)
{
  double zoom_level = shumate_viewport_get_zoom_level (viewport);
  double our_tile_size = shumate_map_source_get_tile_size (map_source);
  double reference_tile_size = shumate_map_source_get_tile_size (shumate_viewport_get_reference_map_source (viewport));
  return log2 (reference_tile_size / our_tile_size) + zoom_level;
}

static void
rotate_around_origin (double *x,
                      double *y,
                      double  angle)
{
  /* Rotate (x, y) around (0, 0) */

  if (angle == 0)
    return;

  double old_x = *x;
  double old_y = *y;

  *x = cosf (angle) * old_x - sinf (angle) * old_y;
  *y = sinf (angle) * old_x + cosf (angle) * old_y;
}


static void
rotate_around_center (double *x,
                      double *y,
                      double  width,
                      double  height,
                      double  angle)
{
  /* Rotate (x, y) around (width / 2, height / 2) */

  double center_x = width / 2.0;
  double center_y = height / 2.0;

  *x -= center_x;
  *y -= center_y;
  rotate_around_origin (x, y, angle);
  *x += center_x;
  *y += center_y;
}


static void
shumate_vector_symbol_container_size_allocate (GtkWidget *widget,
                                               int        width,
                                               int        height,
                                               int        baseline)
{
  SHUMATE_PROFILE_START ();

  ShumateVectorSymbolContainer *self = SHUMATE_VECTOR_SYMBOL_CONTAINER (widget);
  GtkAllocation alloc;
  double tile_size = shumate_map_source_get_tile_size (self->map_source);
  ShumateViewport *viewport = shumate_layer_get_viewport (SHUMATE_LAYER (self));
  double zoom_level = get_effective_zoom_level (self->map_source, viewport);
  double rotation = shumate_viewport_get_rotation (viewport);
  double center_x = shumate_map_source_get_x (self->map_source, zoom_level, shumate_location_get_longitude (SHUMATE_LOCATION (viewport)));
  double center_y = shumate_map_source_get_y (self->map_source, zoom_level, shumate_location_get_latitude (SHUMATE_LOCATION (viewport)));

  gboolean recalc = self->labels_changed
                    || self->last_zoom != zoom_level
                    || self->last_rotation != rotation
                    || self->last_width != width
                    || self->last_height != height;

  if (recalc)
    {
      shumate_vector_collision_clear (self->collision);
      self->last_center_x = center_x;
      self->last_center_y = center_y;
      self->collision->delta_x = 0;
      self->collision->delta_y = 0;
      self->visible_count = 0;
    }
  else
    {
      self->collision->delta_x = center_x - self->last_center_x;
      self->collision->delta_y = center_y - self->last_center_y;
      rotate_around_origin (&self->collision->delta_x, &self->collision->delta_y, rotation);
    }

  /* Higher layers have priority during placement, so iterate the
     array from back to front */
  for (int i = self->layer_buckets->len - 1; i >= 0; i --)
    {
      LayerBucket *bucket = g_ptr_array_index (self->layer_buckets, i);

      for (int j = 0; j < bucket->symbols->len; j ++)
        {
          ChildInfo *child = g_ptr_array_index (bucket->symbols, j);
          double tile_size_at_zoom = tile_size * pow (2, zoom_level - child->zoom);
          double x = (child->tile_x + child->x) * tile_size_at_zoom - center_x + width/2.0;
          double y = (child->tile_y + child->y) * tile_size_at_zoom - center_y + height/2.0;

          rotate_around_center (&x, &y, width, height, rotation);

          if (recalc)
            {
              gboolean now_visible =
                shumate_vector_symbol_calculate_collision (child->symbol,
                                                          self->collision,
                                                          x,
                                                          y,
                                                          tile_size_at_zoom,
                                                          rotation,
                                                          &child->bounds);

              if (now_visible != child->visible)
                {
                  gtk_widget_set_child_visible (GTK_WIDGET (child->symbol), now_visible);
                  child->visible = now_visible;
                }

              if (now_visible)
                self->visible_count ++;
            }

          if (!child->visible)
            continue;

          gtk_widget_measure (GTK_WIDGET (child->symbol), GTK_ORIENTATION_HORIZONTAL, -1, NULL, NULL, NULL, NULL);
          gtk_widget_measure (GTK_WIDGET (child->symbol), GTK_ORIENTATION_VERTICAL, -1, NULL, NULL, NULL, NULL);

          alloc.x = child->bounds.origin.x - self->collision->delta_x;
          alloc.y = child->bounds.origin.y - self->collision->delta_y;
          alloc.width = child->bounds.size.width;
          alloc.height = child->bounds.size.height;

          gtk_widget_size_allocate (GTK_WIDGET (child->symbol), &alloc, -1);
          if (child->symbol_info->line != NULL)
            gtk_widget_queue_draw (GTK_WIDGET (child->symbol));
        }
    }

  self->labels_changed = FALSE;
  self->last_rotation = rotation;
  self->last_zoom = zoom_level;
  self->last_width = width;
  self->last_height = height;
}


static void
shumate_vector_symbol_container_snapshot (GtkWidget   *widget,
                                          GtkSnapshot *snapshot)
{
  SHUMATE_PROFILE_START ();

  ShumateVectorSymbolContainer *self = SHUMATE_VECTOR_SYMBOL_CONTAINER (widget);
  ShumateInspectorSettings *settings = shumate_inspector_settings_get_default ();

  for (int i = 0; i < self->layer_buckets->len; i ++)
    {
      LayerBucket *bucket = g_ptr_array_index (self->layer_buckets, i);

      for (int j = 0; j < bucket->symbols->len; j ++)
        {
          ChildInfo *child = g_ptr_array_index (bucket->symbols, j);
          double correct_x = child->bounds.origin.x - self->collision->delta_x;
          double correct_y = child->bounds.origin.y - self->collision->delta_y;

          gtk_snapshot_save (snapshot);
          gtk_snapshot_translate (snapshot,
                                  &GRAPHENE_POINT_INIT (
                                    correct_x - (int) correct_x,
                                    correct_y - (int) correct_y
                                  ));
          gtk_widget_snapshot_child (widget, GTK_WIDGET (child->symbol), snapshot);
          gtk_snapshot_restore (snapshot);
        }
    }

  if (shumate_inspector_settings_get_show_collision_boxes (settings))
    {
      ShumateViewport *viewport = shumate_layer_get_viewport (SHUMATE_LAYER (self));
      double rotation = shumate_viewport_get_rotation (viewport);
      double delta_x = -self->collision->delta_x;
      double delta_y = -self->collision->delta_y;

      rotate_around_origin (&delta_x, &delta_y, rotation);

      gtk_snapshot_save (snapshot);
      gtk_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (delta_x, delta_y));
      shumate_vector_collision_visualize (self->collision, snapshot);
      gtk_snapshot_restore (snapshot);
    }
}


static void
shumate_vector_symbol_container_class_init (ShumateVectorSymbolContainerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = shumate_vector_symbol_container_constructed;
  object_class->dispose = shumate_vector_symbol_container_dispose;
  object_class->finalize = shumate_vector_symbol_container_finalize;
  object_class->get_property = shumate_vector_symbol_container_get_property;
  object_class->set_property = shumate_vector_symbol_container_set_property;
  widget_class->size_allocate = shumate_vector_symbol_container_size_allocate;
  widget_class->snapshot = shumate_vector_symbol_container_snapshot;

  obj_properties[PROP_MAP_SOURCE] =
    g_param_spec_object ("map-source",
                         "Map source",
                         "Map source",
                         SHUMATE_TYPE_MAP_SOURCE,
                         G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, N_PROPS, obj_properties);

  signals[SYMBOL_CLICKED] =
    g_signal_new ("symbol-clicked",
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE,
                  1,
                  SHUMATE_TYPE_SYMBOL_EVENT);
}

static void
shumate_vector_symbol_container_init (ShumateVectorSymbolContainer *self)
{
  self->layer_buckets = g_ptr_array_new_with_free_func ((GDestroyNotify)layer_bucket_free);
}

static void
on_symbol_clicked (ShumateVectorSymbolContainer *self,
                   ShumateSymbolEvent           *event,
                   ShumateVectorSymbol          *symbol)
{
  ShumateVectorSymbolInfo *symbol_info = shumate_vector_symbol_get_symbol_info (symbol);

  double tile_size = shumate_map_source_get_tile_size (self->map_source);
  double lat = shumate_map_source_get_latitude (self->map_source,
                                                symbol_info->details->tile_zoom_level,
                                                (symbol_info->details->tile_y + symbol_info->y) * tile_size);
  double lon = shumate_map_source_get_longitude (self->map_source,
                                                 symbol_info->details->tile_zoom_level,
                                                 (symbol_info->details->tile_x + symbol_info->x) * tile_size);

  shumate_symbol_event_set_lat_lon (event, lat, lon);

  g_signal_emit (self, signals[SYMBOL_CLICKED], 0, event);
}


void
shumate_vector_symbol_container_add_symbols (ShumateVectorSymbolContainer *self,
                                             GPtrArray                    *symbol_infos,
                                             int                           tile_x,
                                             int                           tile_y,
                                             int                           zoom)
{
  SHUMATE_PROFILE_START ();

  g_return_if_fail (SHUMATE_IS_VECTOR_SYMBOL_CONTAINER (self));

  for (int i = 0; i < symbol_infos->len; i ++)
    {
      ChildInfo *info = g_new0 (ChildInfo, 1);
      ShumateVectorSymbolInfo *symbol_info = symbol_infos->pdata[i];
      ShumateVectorSymbol *symbol = shumate_vector_symbol_new (symbol_info);

      info->symbol = symbol;
      info->symbol_info = symbol_info;
      info->x = symbol_info->x;
      info->y = symbol_info->y;
      info->tile_x = tile_x;
      info->tile_y = tile_y;
      info->zoom = zoom;
      info->visible = TRUE;

      add_symbol_to_layer_buckets (self, info);
      gtk_widget_set_parent (GTK_WIDGET (info->symbol), GTK_WIDGET (self));
      self->child_count ++;

      g_signal_connect_object (info->symbol,
                               "clicked",
                               G_CALLBACK (on_symbol_clicked),
                               self,
                               G_CONNECT_SWAPPED);
    }

  sort_layer_buckets (self);
  self->labels_changed = TRUE;
}


void
shumate_vector_symbol_container_remove_symbols (ShumateVectorSymbolContainer *self,
                                                int                           tile_x,
                                                int                           tile_y,
                                                int                           zoom)
{
  SHUMATE_PROFILE_START ();

  g_return_if_fail (SHUMATE_IS_VECTOR_SYMBOL_CONTAINER (self));

  for (int i = 0; i < self->layer_buckets->len; i ++)
    {
      LayerBucket *bucket = g_ptr_array_index (self->layer_buckets, i);
      int k = 0;

      for (int j = 0; j < bucket->symbols->len; j ++)
        {
          ChildInfo *info = g_ptr_array_index (bucket->symbols, j);

          if (info->tile_x == tile_x && info->tile_y == tile_y && info->zoom == zoom)
            {
              gtk_widget_unparent (GTK_WIDGET (info->symbol));
              self->child_count --;
              g_clear_pointer (&g_ptr_array_index (bucket->symbols, j), g_free);
            }
          else
            {
              g_ptr_array_index (bucket->symbols, j) = NULL;
              g_ptr_array_index (bucket->symbols, k) = info;
              k ++;
            }
        }

      g_ptr_array_set_size (bucket->symbols, k);
    }

  self->labels_changed = TRUE;
}


ShumateMapSource *
shumate_vector_symbol_container_get_map_source (ShumateVectorSymbolContainer *self)
{
  g_return_val_if_fail (SHUMATE_IS_VECTOR_SYMBOL_CONTAINER (self), NULL);
  return self->map_source;
}


ShumateVectorCollision *
shumate_vector_symbol_container_get_collision (ShumateVectorSymbolContainer *self)
{
  g_return_val_if_fail (SHUMATE_IS_VECTOR_SYMBOL_CONTAINER (self), NULL);
  return self->collision;
}


char *
shumate_vector_symbol_container_get_debug_text (ShumateVectorSymbolContainer *self)
{
  return g_strdup_printf ("symbols: %d, %d visible\n", self->child_count, self->visible_count);
}
