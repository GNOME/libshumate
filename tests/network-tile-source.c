#include <gtk/gtk.h>
#include <libsoup/soup.h>
#include <shumate/shumate.h>
#include "test-tile-server.h"


static ShumateMapSource *
create_tile_source (const char *uri)
{
  // TODO: Disable the file cache to make sure we're really testing the network
  // source

  g_autofree char *template = g_strdup_printf ("%s/#X#/#Y#/#Z#", uri);
  g_autofree char *r = g_uuid_string_random ();
  g_autofree char *id = g_strdup_printf ("test_%s", r);

  return SHUMATE_MAP_SOURCE (
    shumate_network_tile_source_new_full (id,
                                          "Test Source",
                                          NULL, NULL,
                                          0, 20,
                                          256, SHUMATE_MAP_PROJECTION_MERCATOR,
                                          template)
  );
}


static void
on_tile_filled (GObject *object, GAsyncResult *res, gpointer user_data)
{
  g_autoptr(GError) error = NULL;
  GMainLoop *loop = user_data;

  shumate_map_source_fill_tile_finish ((ShumateMapSource *) object, res, &error);
  g_assert_no_error (error);

  g_main_loop_quit (loop);
}

static void
test_network_tile_source_tile (void)
{
  g_autoptr(TestTileServer) server = test_tile_server_new ();
  g_autofree char *uri = test_tile_server_start (server);
  g_autoptr(ShumateMapSource) source = create_tile_source (uri);
  g_autoptr(ShumateTile) tile = shumate_tile_new_full (0, 0, 256, 0);
  g_autoptr(GMainLoop) loop = NULL;

  g_object_ref_sink (tile);

  /* Fill the tile */
  loop = g_main_loop_new (NULL, TRUE);
  shumate_map_source_fill_tile_async (source, tile, NULL, on_tile_filled, loop);
  g_main_loop_run (loop);

  g_assert_nonnull (shumate_tile_get_texture (tile));
  test_tile_server_assert_requests (server, 1);
}

/* Test that once a tile is requested, future requests go to the cache */
static void
test_network_tile_cache_hit (void)
{
  g_autoptr(TestTileServer) server = test_tile_server_new ();
  g_autofree char *uri = test_tile_server_start (server);
  g_autoptr(ShumateMapSource) source = create_tile_source (uri);
  g_autoptr(ShumateTile) tile = shumate_tile_new_full (0, 0, 256, 1);
  g_autoptr(GMainLoop) loop = NULL;

  g_object_ref_sink (tile);

  /* Fill the tile */
  loop = g_main_loop_new (NULL, TRUE);
  shumate_map_source_fill_tile_async (source, tile, NULL, on_tile_filled, loop);
  g_main_loop_run (loop);

  g_assert_nonnull (shumate_tile_get_texture (tile));

  shumate_tile_set_texture (tile, NULL);

  test_tile_server_assert_requests (server, 1);
  loop = g_main_loop_new (NULL, TRUE);
  shumate_map_source_fill_tile_async (source, tile, NULL, on_tile_filled, loop);
  g_main_loop_run (loop);

  g_assert_nonnull (shumate_tile_get_texture (tile));
  test_tile_server_assert_requests (server, 0);
}


static void
on_filled_invalid_url (GObject *object, GAsyncResult *res, gpointer user_data)
{
  g_autoptr(GError) error = NULL;
  GMainLoop *loop = user_data;

  shumate_map_source_fill_tile_finish ((ShumateMapSource *) object, res, &error);
  g_assert_error (error, SHUMATE_NETWORK_SOURCE_ERROR, SHUMATE_NETWORK_SOURCE_ERROR_MALFORMED_URL);

  g_main_loop_quit (loop);
}

/* Test that the proper error is thrown when the URL is invalid */
static void
test_network_tile_invalid_url (void)
{
  char *uri = "this isn't a valid url";
  g_autoptr(ShumateMapSource) source = create_tile_source (uri);
  g_autoptr(ShumateTile) tile = shumate_tile_new_full (0, 0, 256, 1);
  g_autoptr(GMainLoop) loop = NULL;

  g_object_ref_sink (tile);

  /* Fill the tile */
  loop = g_main_loop_new (NULL, TRUE);
  shumate_map_source_fill_tile_async (source, tile, NULL, on_filled_invalid_url, loop);
  g_main_loop_run (loop);

  g_assert_null (shumate_tile_get_texture (tile));
}


static void
on_filled_bad_response (GObject *object, GAsyncResult *res, gpointer user_data)
{
  g_autoptr(GError) error = NULL;
  GMainLoop *loop = user_data;

  shumate_map_source_fill_tile_finish ((ShumateMapSource *) object, res, &error);
  g_assert_error (error, SHUMATE_NETWORK_SOURCE_ERROR, SHUMATE_NETWORK_SOURCE_ERROR_BAD_RESPONSE);

  g_main_loop_quit (loop);
}

/* Test that the proper error is thrown when a bad response is returned */
static void
test_network_tile_bad_response (void)
{
  g_autoptr(TestTileServer) server = test_tile_server_new ();
  g_autofree char *uri = test_tile_server_start (server);
  g_autoptr(ShumateMapSource) source = create_tile_source (uri);
  g_autoptr(ShumateTile) tile = shumate_tile_new_full (0, 0, 256, 1);
  g_autoptr(GMainLoop) loop = NULL;

  g_object_ref_sink (tile);
  test_tile_server_set_status (server, 404);

  /* Fill the tile */
  loop = g_main_loop_new (NULL, TRUE);
  shumate_map_source_fill_tile_async (source, tile, NULL, on_filled_bad_response, loop);
  g_main_loop_run (loop);

  g_assert_null (shumate_tile_get_texture (tile));
}


static void
on_filled_invalid_data (GObject *object, GAsyncResult *res, gpointer user_data)
{
  g_autoptr(GError) error = NULL;
  GMainLoop *loop = user_data;

  shumate_map_source_fill_tile_finish ((ShumateMapSource *) object, res, &error);
  g_assert_error (error, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_UNKNOWN_TYPE);

  g_main_loop_quit (loop);
}

/* Test that the proper error is thrown when invalid data is returned */
static void
test_network_tile_invalid_data (void)
{
  g_autoptr(TestTileServer) server = test_tile_server_new ();
  g_autofree char *uri = test_tile_server_start (server);
  g_autoptr(ShumateMapSource) source = create_tile_source (uri);
  g_autoptr(ShumateTile) tile = shumate_tile_new_full (0, 0, 256, 1);
  g_autoptr(GMainLoop) loop = NULL;

  g_object_ref_sink (tile);
  test_tile_server_set_data (server, "this is not an image file");

  /* Fill the tile */
  loop = g_main_loop_new (NULL, TRUE);
  shumate_map_source_fill_tile_async (source, tile, NULL, on_filled_invalid_data, loop);
  g_main_loop_run (loop);

  g_assert_null (shumate_tile_get_texture (tile));
}


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);
  gtk_init ();

  g_test_add_func ("/network-tile-source/tile", test_network_tile_source_tile);
  /* shumate_network_source_fill_tile_async doesn't wait for the cache
   * operation to finish, so this is unreliable */
  /* g_test_add_func ("/network-tile-source/cache-hit", test_network_tile_cache_hit); */
  g_test_add_func ("/network-tile-source/invalid-url", test_network_tile_invalid_url);
  g_test_add_func ("/network-tile-source/bad-response", test_network_tile_bad_response);
  g_test_add_func ("/network-tile-source/invalid-data", test_network_tile_invalid_data);

  return g_test_run ();
}
