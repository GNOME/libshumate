/* Serves tiles over HTTP for the network source tests. */


#include <libsoup/soup.h>
#include <gdk/gdk.h>


static cairo_status_t
write_func (void *bytearray, const unsigned char *data, unsigned int length)
{
  g_byte_array_append ((GByteArray *) bytearray, data, length);
  return CAIRO_STATUS_SUCCESS;
}


static GBytes *
generate_image ()
{
  GByteArray *bytearray = g_byte_array_new ();
  cairo_surface_t *surface;
  cairo_t *cr;

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 256, 256);

  cr = cairo_create (surface);
  cairo_set_source_rgb (cr, 1, 0, 0);
  cairo_paint (cr);
  cairo_destroy (cr);

  cairo_surface_write_to_png_stream (surface, write_func, bytearray);
  cairo_surface_destroy (surface);

  return g_byte_array_free_to_bytes (bytearray);
}


static void
server_callback (SoupServer *server,
                 SoupMessage *msg,
                 const char *path,
                 GHashTable *query,
                 SoupClientContext *client,
                 gpointer user_data)
{
  gsize data_size;
  const char *data = g_bytes_get_data ((GBytes *) user_data, &data_size);

  soup_message_set_response (msg, "image/png", SOUP_MEMORY_STATIC, data, data_size);
  soup_message_set_status (msg, 200);
}


char *
testserver_start (void)
{
  SoupServer *server = soup_server_new (NULL, NULL);
  g_autoptr(GError) error = NULL;
  g_autoptr(GSList) uris = NULL;

  soup_server_add_handler (server, NULL, server_callback, generate_image (), (GDestroyNotify) g_bytes_unref);

  soup_server_listen_local (server, 0, 0, &error);
  g_assert_no_error (error);

  uris = soup_server_get_uris (server);
  g_assert_true (g_slist_length (uris) >= 1);
  return soup_uri_to_string (uris->data, FALSE);
}
