/* Serves tiles over HTTP for the network source tests. */


#include <libsoup/soup.h>
#include <gdk/gdk.h>

#include "test-tile-server.h"


struct _TestTileServer
{
  GObject parent_instance;
  SoupServer *server;
};

G_DEFINE_TYPE (TestTileServer, test_tile_server, G_TYPE_OBJECT)


TestTileServer *
test_tile_server_new (void)
{
  return g_object_new (TEST_TYPE_TILE_SERVER, NULL);
}

static void
test_tile_server_finalize (GObject *object)
{
  TestTileServer *self = (TestTileServer *)object;

  g_clear_object (&self->server);

  G_OBJECT_CLASS (test_tile_server_parent_class)->finalize (object);
}

static void
test_tile_server_class_init (TestTileServerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = test_tile_server_finalize;
}

static void
test_tile_server_init (TestTileServer *self)
{
  self->server = soup_server_new (NULL, NULL);
}


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
test_tile_server_start (TestTileServer *self)
{
  g_autoptr(GError) error = NULL;
  g_autoptr(GSList) uris = NULL;
  soup_server_add_handler (self->server, NULL, server_callback, generate_image (), (GDestroyNotify) g_bytes_unref);

  soup_server_listen_local (self->server, 0, 0, &error);
  g_assert_no_error (error);

  uris = soup_server_get_uris (self->server);
  g_assert_true (g_slist_length (uris) >= 1);
  return soup_uri_to_string (uris->data, FALSE);
}
