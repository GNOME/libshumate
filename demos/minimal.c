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
  ShumateScale *scale;
  ShumateLicense *license;

  /* Create the map view */
  overlay = gtk_overlay_new ();
  view = shumate_view_new ();
  gtk_overlay_set_child (GTK_OVERLAY (overlay), GTK_WIDGET (view));
  scale = shumate_scale_new ();
  g_object_set (G_OBJECT (scale),
                "valign", GTK_ALIGN_END,
                "halign", GTK_ALIGN_START,
                NULL);
  shumate_scale_connect_view (scale, view);
  gtk_overlay_add_overlay (GTK_OVERLAY (overlay), GTK_WIDGET (scale));

  license = shumate_license_new ();
  g_object_set (G_OBJECT (license),
                "valign", GTK_ALIGN_END,
                "halign", GTK_ALIGN_END,
                NULL);
  shumate_license_connect_view (license, view);
  gtk_overlay_add_overlay (GTK_OVERLAY (overlay), GTK_WIDGET (license));

  window = GTK_WINDOW (gtk_application_window_new (app));
  gtk_window_set_title (window, "Window");
  gtk_window_set_default_size (window, 200, 200);
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
