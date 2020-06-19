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

static GtkWidget *view = NULL;

static gboolean
callback (GtkWidget *parent)
{
  if (!view)
    {
      ShumateViewport *viewport;
      /* Create the map view */
      view = GTK_WIDGET (shumate_view_new ());
      gtk_widget_insert_after (view, parent, NULL);

      viewport = shumate_view_get_viewport (SHUMATE_VIEW (view));
      shumate_viewport_set_zoom_level (viewport, 12);
      shumate_location_set_location (SHUMATE_LOCATION (viewport), 45.466, -73.75);
    }
  else
    g_clear_pointer (&view, gtk_widget_unparent);

  return G_SOURCE_CONTINUE;
}

static gboolean
on_close_request (GtkWindow *window,
                  gpointer   user_data)
{
  g_source_remove (GPOINTER_TO_UINT (user_data));
  return FALSE;
}

static void
activate (GtkApplication *app,
          gpointer        user_data)
{
  GtkWidget *grid;
  guint timeout_id;
  GtkWindow *window = GTK_WINDOW (gtk_application_window_new (app));
  gtk_window_set_title (window, "Window");
  gtk_window_set_default_size (window, 800, 600);
  grid = gtk_grid_new ();
  gtk_window_set_child (window, grid);
  gtk_window_present (window);

  timeout_id = g_timeout_add (100, G_SOURCE_FUNC (callback), grid);
  g_signal_connect (window, "close-request", G_CALLBACK (on_close_request), GUINT_TO_POINTER (timeout_id));
}

int
main (int argc, char *argv[])
{
  g_autoptr(GtkApplication) app = NULL;

  app = gtk_application_new ("org.shumate.example", G_APPLICATION_FLAGS_NONE);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);

  return g_application_run (G_APPLICATION (app), argc, argv);
}
