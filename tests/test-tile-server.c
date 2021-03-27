/* Serves tiles over HTTP for the network source tests. */


#include <libsoup/soup.h>
#include <gdk/gdk.h>

#include "test-tile-server.h"


struct _TestTileServer
{
  GObject parent_instance;
  SoupServer *server;
  GBytes *bytes;
  int requests;
  int status;
  char *etag;
};

G_DEFINE_TYPE (TestTileServer, test_tile_server, G_TYPE_OBJECT)


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
  g_clear_pointer (&self->bytes, g_bytes_unref);
  g_clear_pointer (&self->etag, g_free);

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
  self->bytes = generate_image ();
  self->status = 200;
}


static void
server_callback (SoupServer *server,
                 SoupMessage *msg,
                 const char *path,
                 GHashTable *query,
                 SoupClientContext *client,
                 gpointer user_data)
{
  TestTileServer *self = user_data;
  gsize data_size;
  const char *data = NULL;

  self->requests ++;

  if (self->bytes)
    {
      data = g_bytes_get_data (self->bytes, &data_size);
      soup_message_set_response (msg, "image/png", SOUP_MEMORY_STATIC, data, data_size);
    }

  soup_message_set_status (msg, self->status);
}


char *
test_tile_server_start (TestTileServer *self)
{
  g_autoptr(GError) error = NULL;
  g_autoptr(GSList) uris = NULL;
  soup_server_add_handler (self->server, NULL, server_callback, self, NULL);

  soup_server_listen_local (self->server, 0, 0, &error);
  g_assert_no_error (error);

  uris = soup_server_get_uris (self->server);
  g_assert_true (g_slist_length (uris) >= 1);
  return soup_uri_to_string (uris->data, FALSE);
}

void
test_tile_server_assert_requests (TestTileServer *self, int times)
{
  g_assert_cmpint (self->requests, ==, times);
  self->requests = 0;
}

void
test_tile_server_set_status (TestTileServer *self, int status)
{
  self->status = status;
}

void
test_tile_server_set_data (TestTileServer *self, const char *data)
{
  g_bytes_unref (self->bytes);
  if (data)
    self->bytes = g_bytes_new (data, strlen (data));
  else
    self->bytes = NULL;
}

void
test_tile_server_set_etag (TestTileServer *self, const char *etag)
{
  g_clear_pointer (&self->etag, g_free);
  self->etag = g_strdup (etag);
}
