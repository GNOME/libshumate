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


struct _ShumateVectorSymbolContainer
{
  ShumateLayer parent_instance;

  ShumateMapSource *map_source;

  GList *children;
  ShumateVectorCollision *collision;
};

G_DEFINE_TYPE (ShumateVectorSymbolContainer, shumate_vector_symbol_container, SHUMATE_TYPE_LAYER)

enum {
  PROP_0,
  PROP_MAP_SOURCE,
  N_PROPS,
};

static GParamSpec *obj_properties[N_PROPS] = { NULL, };


typedef struct {
  // do not need to be freed because they're owned by the widget
  ShumateVectorSymbol *symbol;
  ShumateVectorSymbolInfo *symbol_info;

  ShumateVectorCollisionMarker *marker;

  // These are coordinates [0, 1) within the tile
  float x;
  float y;

  // We assume these don't change so we don't have to measure them again
  // every time. We can do this because children are all created internally
  int width;
  int height;

  int tile_x;
  int tile_y;
  int zoom;
} ChildInfo;


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
  ShumateViewport *viewport;

  G_OBJECT_CLASS (shumate_vector_symbol_container_parent_class)->constructed (object);

  viewport = shumate_layer_get_viewport (SHUMATE_LAYER (self));

  g_signal_connect_swapped (viewport, "notify::longitude", G_CALLBACK (on_viewport_changed), self);
  g_signal_connect_swapped (viewport, "notify::latitude", G_CALLBACK (on_viewport_changed), self);
  g_signal_connect_swapped (viewport, "notify::zoom-level", G_CALLBACK (on_viewport_changed), self);
  g_signal_connect_swapped (viewport, "notify::rotation", G_CALLBACK (on_viewport_changed), self);
}


static void
shumate_vector_symbol_container_finalize (GObject *object)
{
  ShumateVectorSymbolContainer *self = (ShumateVectorSymbolContainer *)object;

  g_list_free_full (self->children, (GDestroyNotify) g_free);
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


static void
rotate_around_center (float *x,
                      float *y,
                      float  width,
                      float  height,
                      float  angle)
{
  /* Rotate (x, y) around (width / 2, height / 2) */

  float old_x = *x;
  float old_y = *y;
  float center_x = width / 2.0;
  float center_y = height / 2.0;

  *x = cosf (angle) * (old_x - center_x) - sinf (angle) * (old_y - center_y) + center_x;
  *y = sinf (angle) * (old_x - center_x) + cosf (angle) * (old_y - center_y) + center_y;
}


static void
shumate_vector_symbol_container_size_allocate (GtkWidget *widget,
                                               int        width,
                                               int        height,
                                               int        baseline)
{
  ShumateVectorSymbolContainer *self = SHUMATE_VECTOR_SYMBOL_CONTAINER (widget);
  GtkAllocation alloc;
  float tile_size = shumate_map_source_get_tile_size (self->map_source);
  ShumateViewport *viewport = shumate_layer_get_viewport (SHUMATE_LAYER (self));
  float zoom_level = shumate_viewport_get_zoom_level (viewport);
  float rotation = shumate_viewport_get_rotation (viewport);
  float center_x = shumate_map_source_get_x (self->map_source, zoom_level, shumate_location_get_longitude (SHUMATE_LOCATION (viewport)));
  float center_y = shumate_map_source_get_y (self->map_source, zoom_level, shumate_location_get_latitude (SHUMATE_LOCATION (viewport)));

  shumate_vector_collision_recalc (self->collision, rotation, zoom_level);

  for (GList *l = self->children; l != NULL; l = l->next)
    {
      ChildInfo *child = l->data;
      float tile_size_at_zoom = tile_size * powf (2, zoom_level - child->zoom);
      float x = (child->tile_x + child->x) * tile_size_at_zoom - center_x + width/2.0;
      float y = (child->tile_y + child->y) * tile_size_at_zoom - center_y + height/2.0;

      gtk_widget_set_child_visible (GTK_WIDGET (child->symbol), child->marker->visible);
      if (!child->marker->visible)
        continue;

      rotate_around_center (&x, &y, width, height, rotation);
      alloc.x = x - child->width/2.0;
      alloc.y = y - child->height/2.0;

      alloc.width = child->width;
      alloc.height = child->height;

      gtk_widget_size_allocate (GTK_WIDGET (child->symbol), &alloc, -1);
      if (child->symbol_info->line_placement)
        gtk_widget_queue_draw (GTK_WIDGET (child->symbol));
    }
}


static void
shumate_vector_symbol_container_snapshot (GtkWidget   *widget,
                                          GtkSnapshot *snapshot)
{
  ShumateVectorSymbolContainer *self = SHUMATE_VECTOR_SYMBOL_CONTAINER (widget);

  for (GList *l = self->children; l != NULL; l = l->next)
    {
      ChildInfo *child = (ChildInfo *)l->data;

      gtk_snapshot_save (snapshot);

      gtk_widget_snapshot_child (widget, GTK_WIDGET (child->symbol), snapshot);
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
}

static void
shumate_vector_symbol_container_init (ShumateVectorSymbolContainer *self)
{
  self->collision = shumate_vector_collision_new ();
}


void
shumate_vector_symbol_container_add_symbols (ShumateVectorSymbolContainer *self,
                                             GPtrArray                    *symbol_infos,
                                             int                           tile_x,
                                             int                           tile_y,
                                             int                           zoom)
{
  double tile_size;

  g_return_if_fail (SHUMATE_IS_VECTOR_SYMBOL_CONTAINER (self));

  tile_size = shumate_map_source_get_tile_size (self->map_source);

  for (int i = 0; i < symbol_infos->len; i ++)
    {
      ChildInfo *info = g_new0 (ChildInfo, 1);
      ShumateVectorSymbolInfo *symbol_info = symbol_infos->pdata[i];
      ShumateVectorSymbol *symbol = shumate_vector_symbol_new (symbol_info);

      info->symbol = symbol;
      info->symbol_info = symbol_info;
      gtk_widget_measure (GTK_WIDGET (symbol), GTK_ORIENTATION_HORIZONTAL, -1, NULL, &info->width, NULL, NULL);
      gtk_widget_measure (GTK_WIDGET (symbol), GTK_ORIENTATION_VERTICAL, -1, NULL, &info->height, NULL, NULL);
      info->x = symbol_info->x;
      info->y = symbol_info->y;
      info->tile_x = tile_x;
      info->tile_y = tile_y;
      info->zoom = zoom;

      info->marker = shumate_vector_collision_insert (self->collision,
                                                      zoom,
                                                      (tile_x + info->x) * tile_size,
                                                      (tile_y + info->y) * tile_size,
                                                      -info->width / 2,
                                                      info->width / 2,
                                                      -info->height / 2,
                                                      info->height / 2);

      self->children = g_list_prepend (self->children, info);
      gtk_widget_set_parent (GTK_WIDGET (info->symbol), GTK_WIDGET (self));
    }
}


void
shumate_vector_symbol_container_remove_symbols (ShumateVectorSymbolContainer *self,
                                                int                           tile_x,
                                                int                           tile_y,
                                                int                           zoom)
{
  g_return_if_fail (SHUMATE_IS_VECTOR_SYMBOL_CONTAINER (self));

  for (GList *l = self->children; l != NULL; l = l->next)
    {
      ChildInfo *info = l->data;

      if (info->tile_x != tile_x || info->tile_y != tile_y || info->zoom != zoom)
        continue;

      shumate_vector_collision_remove (self->collision, info->marker);

      gtk_widget_unparent (GTK_WIDGET (info->symbol));
      g_clear_pointer (&l->data, g_free);
    }

  self->children = g_list_remove_all (self->children, NULL);
}


ShumateMapSource *
shumate_vector_symbol_container_get_map_source (ShumateVectorSymbolContainer *self)
{
  g_return_val_if_fail (SHUMATE_IS_VECTOR_SYMBOL_CONTAINER (self), NULL);
  return self->map_source;
}
