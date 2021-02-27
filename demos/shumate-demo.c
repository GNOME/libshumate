/*
 * Copyright (C) 2010-2013 Jiri Techet <techet@gmail.com>
 * Copyright (C) 2019 Marcus Lundblad <ml@update.uu.se>
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

#include <gtk/gtk.h>
#include <shumate/shumate.h>

static void
activate (GtkApplication* app,
          gpointer        user_data)
{
  GtkWindow *window;
  GtkWidget *overlay;
  ShumateView *view;
  ShumateViewport *viewport;
  ShumateScale *scale;
  ShumateLicense *license;
  ShumatePathLayer *path_layer;
  ShumateMapSourceFactory *factory;
  ShumateMapSource *source;
  ShumateMapLayer *layer;

  /* Create the map view */
  overlay = gtk_overlay_new ();
  view = shumate_view_new ();

  viewport = shumate_view_get_viewport (view);
  factory = shumate_map_source_factory_dup_default ();
  source = shumate_map_source_factory_create_cached_source (factory, SHUMATE_MAP_SOURCE_OSM_MAPNIK);
  shumate_viewport_set_reference_map_source (viewport, source);

  layer = shumate_map_layer_new (source, viewport);
  shumate_view_add_layer (view, SHUMATE_LAYER (layer));
  
  gtk_overlay_set_child (GTK_OVERLAY (overlay), GTK_WIDGET (view));
  scale = shumate_scale_new (shumate_view_get_viewport (view));
  g_object_set (G_OBJECT (scale),
                "valign", GTK_ALIGN_END,
                "halign", GTK_ALIGN_START,
                NULL);
  gtk_overlay_add_overlay (GTK_OVERLAY (overlay), GTK_WIDGET (scale));

  license = shumate_license_new ();
  g_object_set (G_OBJECT (license),
                "valign", GTK_ALIGN_END,
                "halign", GTK_ALIGN_END,
                NULL);
  shumate_license_append_map_source (license, source);
  gtk_overlay_add_overlay (GTK_OVERLAY (overlay), GTK_WIDGET (license));

  path_layer = shumate_path_layer_new (viewport);
  shumate_view_add_layer (view, SHUMATE_LAYER (path_layer));

  window = GTK_WINDOW (gtk_application_window_new (app));
  gtk_window_set_title (window, "Window");
  gtk_window_set_default_size (window, 800, 600);
  gtk_window_set_child (window, GTK_WIDGET (overlay));
  gtk_window_present (window);
}

int
main (int argc, char *argv[])
{
  g_autoptr(GtkApplication) app = NULL;

  app = gtk_application_new ("org.gnome.Shumate.Demo", G_APPLICATION_FLAGS_NONE);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);

  return g_application_run (G_APPLICATION (app), argc, argv);
}
