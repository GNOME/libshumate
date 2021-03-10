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


#include "shumate-demo-window.h"
#include "shumate-test-tile-source.h"


struct _ShumateDemoWindow
{
  GtkApplicationWindow parent_instance;

  ShumateView *view;
  GtkOverlay *overlay;
  ShumateScale *scale;
  ShumateLicense *license;
  GtkDropDown *layers_dropdown;

  ShumateMapLayer *tile_layer;
  ShumateMarkerLayer *marker_layer;
  ShumatePathLayer *path_layer;

  ShumateMapSource *current_source;
};

G_DEFINE_TYPE (ShumateDemoWindow, shumate_demo_window, GTK_TYPE_APPLICATION_WINDOW)


ShumateDemoWindow *
shumate_demo_window_new (GtkApplication *app)
{
  return g_object_new (SHUMATE_TYPE_DEMO_WINDOW,
                       "application", app,
                       NULL);
}


static void
create_marker (ShumateDemoWindow *self, double lat, double lng)
{
  GtkWidget *image = gtk_image_new_from_icon_name ("map-marker-symbolic");
  ShumateMarker *marker = shumate_marker_new ();

  shumate_location_set_location (SHUMATE_LOCATION (marker), lat, lng);
  shumate_marker_set_child (marker, image);

  shumate_marker_layer_add_marker (self->marker_layer, marker);
  shumate_path_layer_add_node (self->path_layer, SHUMATE_LOCATION (marker));
}


static void
set_map_source (ShumateDemoWindow *self, ShumateMapSource *new_source)
{
  ShumateViewport *viewport = shumate_view_get_viewport (self->view);
  ShumateMapLayer *tile_layer;

  if (self->current_source) {
    shumate_license_remove_map_source (self->license, self->current_source);
  }

  g_clear_object (&self->current_source);
  self->current_source = new_source;

  shumate_viewport_set_reference_map_source (viewport, new_source);
  shumate_view_set_map_source (self->view, new_source);

  tile_layer = shumate_map_layer_new (new_source, viewport);
  shumate_view_insert_layer_behind (self->view, SHUMATE_LAYER (tile_layer), SHUMATE_LAYER (self->tile_layer));
  if (self->tile_layer) {
    shumate_view_remove_layer (self->view, SHUMATE_LAYER (self->tile_layer));
  }
  self->tile_layer = tile_layer;

  shumate_license_append_map_source (self->license, new_source);
}

static void
on_layers_dropdown_notify_selected (ShumateDemoWindow *self, GParamSpec *pspec, GtkDropDown *dropdown)
{
  g_autoptr(ShumateMapSourceFactory) factory = NULL;

  switch (gtk_drop_down_get_selected (dropdown)) {
  case 0:
    factory = shumate_map_source_factory_dup_default ();
    set_map_source (self, shumate_map_source_factory_create_cached_source (factory, SHUMATE_MAP_SOURCE_OSM_MAPNIK));
    break;
  case 1:
    set_map_source (self, SHUMATE_MAP_SOURCE (shumate_test_tile_source_new ()));
    break;
  }
}


static void
shumate_demo_window_finalize (GObject *object)
{
  ShumateDemoWindow *self = SHUMATE_DEMO_WINDOW (object);

  g_clear_object (&self->current_source);

  G_OBJECT_CLASS (shumate_demo_window_parent_class)->finalize (object);
}


static void
shumate_demo_window_class_init (ShumateDemoWindowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = shumate_demo_window_finalize;

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/Shumate/Demo/ui/shumate-demo-window.ui");
  gtk_widget_class_bind_template_child (widget_class, ShumateDemoWindow, view);
  gtk_widget_class_bind_template_child (widget_class, ShumateDemoWindow, overlay);
  gtk_widget_class_bind_template_child (widget_class, ShumateDemoWindow, scale);
  gtk_widget_class_bind_template_child (widget_class, ShumateDemoWindow, license);
  gtk_widget_class_bind_template_child (widget_class, ShumateDemoWindow, layers_dropdown);
  gtk_widget_class_bind_template_callback (widget_class, on_layers_dropdown_notify_selected);
}


static void
shumate_demo_window_init (ShumateDemoWindow *self)
{
  ShumateViewport *viewport;

  gtk_widget_init_template (GTK_WIDGET (self));

  viewport = shumate_view_get_viewport (self->view);

  /* Set the map source */
  on_layers_dropdown_notify_selected (self, NULL, self->layers_dropdown);

  shumate_scale_set_viewport (self->scale, viewport);

  /* Add the marker layers */
  self->marker_layer = shumate_marker_layer_new (viewport);
  shumate_view_add_layer (self->view, SHUMATE_LAYER (self->marker_layer));
  self->path_layer = shumate_path_layer_new (viewport);
  shumate_view_add_layer (self->view, SHUMATE_LAYER (self->path_layer));

  create_marker (self, 35.426667, -116.89);
  create_marker (self, 40.431389, -4.248056);
  create_marker (self, -35.401389, 148.981667);
}
