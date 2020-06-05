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
  ShumateMarkerLayer *layer;
  gdouble latitude;
  gdouble longitude;
} MarkerData;

/**
 * Returns a GdkPixbuf from a given SoupMessage. This function assumes that the
 * message has completed successfully.
 * If there's an error building the GdkPixbuf the function will return NULL and
 * set error accordingly.
 *
 * The GdkPixbuf has to be freed with g_object_unref.
 */
static GdkPixbuf *
pixbuf_new_from_message (SoupMessage *message,
    GError **error)
{
  const gchar *mime_type = NULL;
  GdkPixbufLoader *loader = NULL;
  GdkPixbuf *pixbuf = NULL;
  gboolean pixbuf_is_open = FALSE;

  *error = NULL;

  /*  Use a pixbuf loader that can load images of the same mime-type as the
      message.
   */
  mime_type = soup_message_headers_get_one (message->response_headers,
        "Content-Type");
  loader = gdk_pixbuf_loader_new_with_mime_type (mime_type, error);
  if (loader != NULL)
    pixbuf_is_open = TRUE;
  if (*error != NULL)
    goto cleanup;


  gdk_pixbuf_loader_write (
      loader,
      (guchar *) message->response_body->data,
      message->response_body->length,
      error);
  if (*error != NULL)
    goto cleanup;

  gdk_pixbuf_loader_close (loader, error);
  pixbuf_is_open = FALSE;
  if (*error != NULL)
    goto cleanup;

  pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
  if (pixbuf == NULL)
    goto cleanup;
  g_object_ref (G_OBJECT (pixbuf));

cleanup:
  if (pixbuf_is_open)
    gdk_pixbuf_loader_close (loader, NULL);

  if (loader != NULL)
    g_object_unref (G_OBJECT (loader));

  return pixbuf;
}


/**
 * Called when an image has been downloaded. This callback will transform the
 * image data (binary chunk sent by the remote web server) into a valid Clutter
 * actor (a texture) and will use this as the source image for a new marker.
 * The marker will then be added to an existing layer.
 *
 * This callback expects the parameter data to be a valid ShumateMarkerLayer.
 */
static void
image_downloaded_cb (SoupSession *session,
    SoupMessage *message,
    gpointer data)
{
  MarkerData *marker_data = NULL;
  SoupURI *uri = NULL;
  char *url = NULL;
  GError *error = NULL;
  GdkPixbuf *pixbuf = NULL;
  ShumateLabel *marker = NULL;

  if (data == NULL)
    goto cleanup;
  marker_data = (MarkerData *) data;

  /* Deal only with finished messages */
  uri = soup_message_get_uri (message);
  url = soup_uri_to_string (uri, FALSE);
  if (!SOUP_STATUS_IS_SUCCESSFUL (message->status_code))
    {
      g_print ("Download of %s failed with error code %d\n", url,
          message->status_code);
      goto cleanup;
    }

  pixbuf = pixbuf_new_from_message (message, &error);
  if (error != NULL)
    {
      g_print ("Failed to convert %s into an image: %s\n", url, error->message);
      goto cleanup;
    }

  /* Finally create a marker with the texture */
  marker = shumate_label_new_with_image (pixbuf);
  shumate_location_set_location (SHUMATE_LOCATION (marker),
      marker_data->latitude, marker_data->longitude);
  shumate_marker_layer_add_marker (marker_data->layer, SHUMATE_MARKER (marker));

cleanup:
  if (marker_data)
    g_object_unref (marker_data->layer);
  g_slice_free (MarkerData, marker_data);
  g_free (url);

  if (error != NULL)
    g_error_free (error);

  if (pixbuf != NULL)
    g_object_unref (G_OBJECT (pixbuf));
}


/**
 * Creates a marker at the given position with an image that's downloaded from
 * the given URL.
 *
 */
static void
create_marker_from_url (ShumateMarkerLayer *layer,
    SoupSession *session,
    gdouble latitude,
    gdouble longitude,
    const gchar *url)
{
  SoupMessage *message;
  MarkerData *data;

  data = g_slice_new (MarkerData);
  data->layer = g_object_ref (layer);
  data->latitude = latitude;
  data->longitude = longitude;

  message = soup_message_new ("GET", url);
  soup_session_queue_message (session, message, image_downloaded_cb, data);
}

static void
activate (GtkApplication* app,
          gpointer        user_data)
{
  GtkWindow *window;
  GtkWidget *overlay;
  ShumateView *view;
  ShumateMarkerLayer *layer;
  SoupSession *session;

  /* Create the map view */
  overlay = gtk_overlay_new ();
  view = shumate_view_new ();
  gtk_overlay_set_child (GTK_OVERLAY (overlay), GTK_WIDGET (view));

  /* Create the markers and marker layer */
  layer = shumate_marker_layer_new_full (SHUMATE_SELECTION_SINGLE);
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
