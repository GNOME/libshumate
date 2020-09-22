/*
 * Copyright (C) 2008 Pierre-Luc Beaudoin <pierre-luc@pierlux.com>
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

#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <shumate/shumate.h>
#include "markers.h"

#define PADDING 10

static void
map_view_button_release_cb (GtkGestureClick *gesture,
                            int              n_press,
                            double           x,
                            double           y,
                            ShumateView     *view)
{
  double lat, lon;
  ShumateViewport *viewport;

  viewport = shumate_view_get_viewport (view);
  lon = shumate_viewport_widget_x_to_longitude (viewport, GTK_WIDGET (view), x);
  lat = shumate_viewport_widget_y_to_latitude (viewport, GTK_WIDGET (view), y);

  g_print ("Map clicked at %f, %f\n", lon, lat);
}

static void
activate (GtkApplication* app,
          gpointer        user_data)
{
  GtkWindow *window;
  GtkWidget *overlay;
  GtkGesture *click_gesture;
  ShumateView *view;
  ShumateMarkerLayer *layer;
  ShumatePathLayer *path;
  ShumateViewport *viewport;

  /* Create the map view */
  overlay = gtk_overlay_new ();
  view = shumate_view_new_simple ();
  viewport = shumate_view_get_viewport (view);

  gtk_overlay_set_child (GTK_OVERLAY (overlay), GTK_WIDGET (view));

  /* Create the markers and marker layer */
  layer = create_marker_layer (view, &path);
  shumate_view_add_layer (view, SHUMATE_LAYER (path));
  shumate_view_add_layer (view, SHUMATE_LAYER (layer));

  click_gesture = gtk_gesture_click_new ();
  gtk_widget_add_controller (GTK_WIDGET (view), GTK_EVENT_CONTROLLER (click_gesture));
  g_signal_connect (click_gesture, "released", G_CALLBACK (map_view_button_release_cb), view);

  /* Finish initialising the map view */
  g_object_set (view,
                "kinetic-mode", TRUE,
                NULL);
  shumate_viewport_set_zoom_level (viewport, 12);
  shumate_view_center_on (view, 45.466, -73.75);

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

  app = gtk_application_new ("org.shumate.example", G_APPLICATION_FLAGS_NONE);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);

  return g_application_run (G_APPLICATION (app), argc, argv);
}
