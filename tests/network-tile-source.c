#include <gtk/gtk.h>
#include <shumate/shumate.h>
#include "test-tile-server.h"


static ShumateMapSource *
create_tile_source (const char *uri)
{
  // TODO: Disable the file cache to make sure we're really testing the network
  // source

  g_autofree char *template = g_strdup_printf ("%s/#X#/#Y#/#Z#", uri);

  return SHUMATE_MAP_SOURCE (
    shumate_network_tile_source_new_full ("testsrc",
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
  g_autofree char *uri = testserver_start ();
  g_autoptr(ShumateMapSource) source = create_tile_source (uri);
  g_autoptr(ShumateTile) tile = shumate_tile_new_full (0, 0, 256, 0);
  g_autoptr(GMainLoop) loop = NULL;

  g_object_ref_sink (tile);

  /* Fill the tile */
  loop = g_main_loop_new (NULL, TRUE);
  shumate_map_source_fill_tile_async (source, tile, NULL, on_tile_filled, loop);
  g_main_loop_run (loop);

  g_assert_nonnull (shumate_tile_get_texture (tile));
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);
  gtk_init ();

  g_test_add_func ("/network-tile-source/tile", test_network_tile_source_tile);

  return g_test_run ();
}
