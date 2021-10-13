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

  ShumateMap *map;
  GtkOverlay *overlay;
  ShumateScale *scale;
  ShumateLicense *license;
  ShumateCompass *compass;
  GtkDropDown *layers_dropdown;
  ShumateMapSourceRegistry *registry;

  ShumateMapLayer *tile_layer;
  ShumateMarkerLayer *marker_layer;
  ShumatePathLayer *path_layer;
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
  ShumateViewport *viewport = shumate_map_get_viewport (self->map);
  ShumateMapLayer *tile_layer;

  shumate_viewport_set_reference_map_source (viewport, new_source);
  shumate_map_set_map_source (self->map, new_source);

  tile_layer = shumate_map_layer_new (new_source, viewport);
  shumate_map_insert_layer_behind (self->map, SHUMATE_LAYER (tile_layer), SHUMATE_LAYER (self->tile_layer));
  if (self->tile_layer) {
    shumate_map_remove_layer (self->map, SHUMATE_LAYER (self->tile_layer));
  }
  self->tile_layer = tile_layer;
}

static void
on_layers_dropdown_notify_selected (ShumateDemoWindow *self, GParamSpec *pspec, GtkDropDown *dropdown)
{
  set_map_source (self, gtk_drop_down_get_selected_item (dropdown));
}


static void
shumate_demo_window_dispose (GObject *object)
{
  ShumateDemoWindow *self = SHUMATE_DEMO_WINDOW (object);

  g_clear_object (&self->registry);

  G_OBJECT_CLASS (shumate_demo_window_parent_class)->dispose (object);
}


static void
shumate_demo_window_class_init (ShumateDemoWindowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = shumate_demo_window_dispose;

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/Shumate/Demo/ui/shumate-demo-window.ui");
  gtk_widget_class_bind_template_child (widget_class, ShumateDemoWindow, map);
  gtk_widget_class_bind_template_child (widget_class, ShumateDemoWindow, overlay);
  gtk_widget_class_bind_template_child (widget_class, ShumateDemoWindow, scale);
  gtk_widget_class_bind_template_child (widget_class, ShumateDemoWindow, license);
  gtk_widget_class_bind_template_child (widget_class, ShumateDemoWindow, compass);
  gtk_widget_class_bind_template_child (widget_class, ShumateDemoWindow, layers_dropdown);
  gtk_widget_class_bind_template_callback (widget_class, on_layers_dropdown_notify_selected);
}

static gchar *
get_map_source_name (ShumateMapSource *map_source)
{
  return g_strdup (shumate_map_source_get_name (map_source));
}

static void
shumate_demo_window_init (ShumateDemoWindow *self)
{
  ShumateViewport *viewport;
  GtkExpression *expression;
  g_autoptr(GBytes) bytes = NULL;
  const char *style_json;
  g_autoptr(ShumateVectorStyle) style = NULL;
  GError *error = NULL;

  gtk_widget_init_template (GTK_WIDGET (self));

  self->registry = shumate_map_source_registry_new_with_defaults ();
  shumate_map_source_registry_add (self->registry, SHUMATE_MAP_SOURCE (shumate_test_tile_source_new ()));
  expression = gtk_cclosure_expression_new (G_TYPE_STRING, NULL, 0, NULL,
                                            (GCallback)get_map_source_name, NULL, NULL);
  gtk_drop_down_set_expression (self->layers_dropdown, expression);
  gtk_drop_down_set_model (self->layers_dropdown, G_LIST_MODEL (self->registry));

  bytes = g_resources_lookup_data ("/org/gnome/Shumate/Demo/styles/map-style.json", G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
  style_json = g_bytes_get_data (bytes, NULL);

  if (!(style = shumate_vector_style_create (style_json, &error)))
    {
      g_warning ("Failed to create vector map style: %s", error->message);
      g_clear_error (&error);
    }
  else
    {
      ShumateMapSource *map_source = SHUMATE_MAP_SOURCE (shumate_network_tile_source_new_vector_full (
        "vector-tiles",
        "Vector Tiles",
        "© OpenStreetMap contributors", NULL, 0, 5, 512,
        SHUMATE_MAP_PROJECTION_MERCATOR,
        "https://jwestman.pages.gitlab.gnome.org/vector-tile-test-data/world_overview/#Z#/#X#/#Y#.pbf",
        style
      ));
      shumate_map_source_registry_add (self->registry, map_source);
    }

  viewport = shumate_map_get_viewport (self->map);

  /* Set the map source */
  on_layers_dropdown_notify_selected (self, NULL, self->layers_dropdown);

  shumate_scale_set_viewport (self->scale, viewport);
  shumate_compass_set_viewport (self->compass, viewport);

  /* Add the marker layers */
  self->marker_layer = shumate_marker_layer_new (viewport);
  shumate_map_add_layer (self->map, SHUMATE_LAYER (self->marker_layer));
  self->path_layer = shumate_path_layer_new (viewport);
  shumate_map_add_layer (self->map, SHUMATE_LAYER (self->path_layer));

  create_marker (self, 35.426667, -116.89);
  create_marker (self, 40.431389, -4.248056);
  create_marker (self, -35.401389, 148.981667);
}
