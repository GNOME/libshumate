/*
 * Copyright (C) 2009 Emmanuel Rodriguez <emmanuel.rodriguez@gmail.com>
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
#include <libsoup/soup.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

/* The data needed for constructing a marker */
typedef struct
{
  SoupMessage *message;
  ShumateMarkerLayer *layer;
  double latitude;
  double longitude;
} MarkerData;


/**
 * Called when an image has been downloaded. This callback will transform the
 * image data (binary chunk sent by the remote web server) into a valid Clutter
 * actor (a texture) and will use this as the source image for a new marker.
 * The marker will then be added to an existing layer.
 *
 * This callback expects the parameter data to be a valid ShumateMarkerLayer.
 */
static void
image_downloaded_cb (GObject      *source_object,
                     GAsyncResult *res,
                     gpointer      user_data)
{
  g_autofree MarkerData *marker_data = user_data;
  g_autoptr(GInputStream) stream = NULL;
  g_autoptr(GError) error = NULL;
  g_autoptr(GdkPixbuf) pixbuf = NULL;
  g_autoptr(ShumateMarkerLayer) layer = g_steal_pointer (&marker_data->layer);
  g_autoptr(SoupMessage) message = g_steal_pointer (&marker_data->message);
  g_autofree char *url = soup_uri_to_string (soup_message_get_uri (message), FALSE);
  ShumateMarker *marker = NULL;
  guint status_code = 0;

  stream = soup_session_send_finish (SOUP_SESSION (source_object), res, &error);
  if (!stream)
    {
      g_print ("Download of %s failed: %s\n", url, error->message);
      return;
    }

  g_object_get (message, "status-code", &status_code, NULL);
  if (!SOUP_STATUS_IS_SUCCESSFUL (status_code))
    {
      g_print ("Download of %s failed: %s (%u)\n", url, soup_status_get_phrase (status_code), status_code);
      return;
    }

  pixbuf = gdk_pixbuf_new_from_stream (stream, NULL, &error);
  if (!pixbuf)
    {
      g_print ("Failed to convert %s into an image: %s\n", url, error->message);
      return;
    }

  /* Finally create a marker with the texture */
  marker = shumate_marker_new ();
  gtk_widget_insert_before (gtk_image_new_from_pixbuf (pixbuf), GTK_WIDGET (marker), NULL);
  shumate_location_set_location (SHUMATE_LOCATION (marker),
      marker_data->latitude, marker_data->longitude);
  shumate_marker_layer_add_marker (layer, marker);
}


/**
 * Creates a marker at the given position with an image that's downloaded from
 * the given URL.
 *
 */
static void
create_marker_from_url (ShumateMarkerLayer *layer,
    SoupSession *session,
    double latitude,
    double longitude,
    const char *url)
{
  MarkerData *data;

  data = g_new0 (MarkerData, 1);
  data->message = soup_message_new ("GET", url);
  data->layer = g_object_ref (layer);
  data->latitude = latitude;
  data->longitude = longitude;

  soup_session_send_async (session, data->message, NULL, image_downloaded_cb, data);
}

static void
activate (GtkApplication *app,
          gpointer        user_data)
{
  GtkWindow *window;
  GtkWidget *overlay;
  ShumateView *view;
  ShumateMarkerLayer *layer;
  SoupSession *session;
  ShumateViewport *viewport;

  /* Create the map view */
  overlay = gtk_overlay_new ();
  view = shumate_view_new ();
  viewport = shumate_view_get_viewport (view);
  gtk_overlay_set_child (GTK_OVERLAY (overlay), GTK_WIDGET (view));

  /* Create the markers and marker layer */
  layer = shumate_marker_layer_new_full (viewport, GTK_SELECTION_SINGLE);
  shumate_view_add_layer (view, SHUMATE_LAYER (layer));
  session = soup_session_new ();
  create_marker_from_url (layer, session, 48.218611, 17.146397,
      "http://hexten.net/cpan-faces/potyl.jpg");
  create_marker_from_url (layer, session, 48.21066, 16.31476,
      "http://hexten.net/cpan-faces/jkutej.jpg");
  create_marker_from_url (layer, session, 48.14838, 17.10791,
      "http://bratislava.pm.org/images/whoiswho/jnthn.jpg");

  /* Finish initialising the map view */
  g_object_set (view,
                "zoom-level", 10,
                "kinetic-mode", TRUE,
                NULL);
  shumate_view_center_on (view, 48.22, 16.8);

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
