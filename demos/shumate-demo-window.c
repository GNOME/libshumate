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

  ShumateSimpleMap *map;
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
  gtk_widget_class_bind_template_child (widget_class, ShumateDemoWindow, layers_dropdown);
}

static gchar *
get_map_source_name (ShumateMapSource *map_source)
{
  return g_strdup (shumate_map_source_get_name (map_source));
}

static void
activate_goto_europe (GSimpleAction *simple,
                      GVariant      *parameter,
                      gpointer       user_data)
{
  shumate_map_go_to_full (SHUMATE_MAP (user_data), 49.531565, 17.532806, 4.5);
}

static void
activate_goto_nyc (GSimpleAction *simple,
                   GVariant      *parameter,
                   gpointer       user_data)
{
  shumate_map_go_to_full (SHUMATE_MAP (user_data), 40.718820, -74.001605, 9);
}

static void
activate_goto_eiffel_tower (GSimpleAction *simple,
                            GVariant      *parameter,
                            gpointer       user_data)
{
  shumate_map_go_to_full (SHUMATE_MAP (user_data), 48.858279, 2.294486, 18);
}

static void
shumate_demo_window_init (ShumateDemoWindow *self)
{
  ShumateViewport *viewport;
  GtkExpression *expression;
  g_autoptr(GBytes) bytes = NULL;
  g_autoptr(GSimpleActionGroup) action_map = NULL;
  const char *style_json;
  GError *error = NULL;
  const GActionEntry action_entries[] = {
    { "goto-europe", activate_goto_europe },
    { "goto-nyc", activate_goto_nyc },
    { "goto-eiffel-tower", activate_goto_eiffel_tower },
  };

  gtk_widget_init_template (GTK_WIDGET (self));

  action_map = g_simple_action_group_new ();
  g_action_map_add_action_entries (G_ACTION_MAP (action_map),
                                   action_entries,
                                   G_N_ELEMENTS (action_entries),
                                   shumate_simple_map_get_map (self->map));
  gtk_widget_insert_action_group (GTK_WIDGET (self), "win", G_ACTION_GROUP (action_map));

  self->registry = shumate_map_source_registry_new_with_defaults ();
  shumate_map_source_registry_add (self->registry, SHUMATE_MAP_SOURCE (shumate_test_tile_source_new ()));
  expression = gtk_cclosure_expression_new (G_TYPE_STRING, NULL, 0, NULL,
                                            (GCallback)get_map_source_name, NULL, NULL);
  gtk_drop_down_set_expression (self->layers_dropdown, expression);
  gtk_drop_down_set_model (self->layers_dropdown, G_LIST_MODEL (self->registry));

  bytes = g_resources_lookup_data ("/org/gnome/Shumate/Demo/styles/map-style.json", G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
  style_json = g_bytes_get_data (bytes, NULL);

  if (shumate_vector_renderer_is_supported ())
    {
      ShumateVectorRenderer *renderer = shumate_vector_renderer_new (
        "vector-tiles",
        style_json,
        &error
      );

      if (error)
        {
          g_warning ("Failed to create vector map style: %s", error->message);
          g_clear_error (&error);
        }
      else
        {
          shumate_map_source_set_license (SHUMATE_MAP_SOURCE (renderer), "© OpenStreetMap contributors");
          shumate_map_source_registry_add (self->registry, SHUMATE_MAP_SOURCE (renderer));
        }
    }

  viewport = shumate_simple_map_get_viewport (self->map);

  /* Add the marker layers */
  self->marker_layer = shumate_marker_layer_new (viewport);
  shumate_simple_map_add_overlay_layer (self->map, SHUMATE_LAYER (self->marker_layer));
  self->path_layer = shumate_path_layer_new (viewport);
  shumate_simple_map_add_overlay_layer (self->map, SHUMATE_LAYER (self->path_layer));

  create_marker (self, 35.426667, -116.89);
  create_marker (self, 40.431389, -4.248056);
  create_marker (self, -35.401389, 148.981667);
}
