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

#include <shumate/shumate.h>
#include <gtk/gtk.h>

static GtkWidget *window;

static ShumateView *
create_view ()
{
  ShumateView *view;

  /* Create the map view */
  view = shumate_view_new ();
  gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (view));

  shumate_view_set_zoom_level (view, 12);
  shumate_view_center_on (view, 45.466, -73.75);
  
  return view;
}


static gboolean
callback (void *data)
{
  static ShumateView *view = NULL;
  
  if (!view)
  {
    view = create_view();
  }
  else
  {
    gtk_widget_destroy (GTK_WIDGET (view));
    view = NULL;
  }
  
  return TRUE;
}


int
main (int argc, char *argv[])
{
  gtk_init ();

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_size_request (GTK_WIDGET (window), 800, 600);
  g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

  g_timeout_add (100, (GSourceFunc) callback, NULL);

  gtk_widget_show (window);
  gtk_main ();

  return 0;
}
