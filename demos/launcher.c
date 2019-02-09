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

static gboolean
map_view_button_release_cb (G_GNUC_UNUSED GtkWidget *widget,
    GdkEvent *event,
    ShumateView *view)
{
  gdouble lat, lon;

  lon = shumate_view_x_to_longitude (view, event->button.x);
  lat = shumate_view_y_to_latitude (view, event->button.y);

  g_print ("Map clicked at %f, %f \n", lat, lon);

  return TRUE;
}


static gboolean
zoom_in (G_GNUC_UNUSED GtkWidget *widget,
    G_GNUC_UNUSED GdkEventButton *event,
    ShumateView *view)
{
  shumate_view_zoom_in (view);
  return TRUE;
}


static gboolean
zoom_out (G_GNUC_UNUSED GtkWidget *widget,
    G_GNUC_UNUSED GdkEventButton *event,
    ShumateView *view)
{
  shumate_view_zoom_out (view);
  return TRUE;
}


int
main (int argc,
    char *argv[])
{
  GtkWidget *window;
  ShumateView *view;
  ShumateMarkerLayer *layer;
  ShumatePathLayer *path;
  gfloat width, total_width = 0;

  gtk_init ();

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_size_request (window, 800, 600);
  g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

  /* Create the map view */
  view = shumate_view_new ();
  gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (view));

  /* Create the markers and marker layer */
  layer = create_marker_layer (view, &path);
  shumate_view_add_layer (view, SHUMATE_LAYER (layer));

  g_signal_connect (G_OBJECT (view), "button-press-event",
                    G_CALLBACK (map_view_button_release_cb), view);

  /* Finish initialising the map view */
  g_object_set (G_OBJECT (view), "zoom-level", 12,
      "kinetic-mode", TRUE, NULL);
  shumate_view_center_on (view, 45.466, -73.75);

  gtk_widget_show (window);
  gtk_widget_show (GTK_WIDGET (view));
  gtk_main ();

  return 0;
}
