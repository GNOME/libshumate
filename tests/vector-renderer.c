#include <gtk/gtk.h>
#include <shumate/shumate.h>
#include "shumate/shumate-tile-private.h"
#include "shumate/shumate-vector-renderer-private.h"

static void
test_vector_renderer_render (void)
{
  GError *error = NULL;
  g_autoptr(GBytes) style_json = NULL;
  g_autoptr(GBytes) tile_data = NULL;
  g_autoptr(ShumateVectorRenderer) renderer = NULL;
  g_autoptr(ShumateTile) tile = shumate_tile_new_full (0, 0, 512, 0);

  style_json = g_resources_lookup_data ("/org/gnome/shumate/Tests/style.json", G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
  g_assert_no_error (error);

  renderer = shumate_vector_renderer_new ("", g_bytes_get_data (style_json, NULL), &error);
  g_assert_no_error (error);

  tile_data = g_resources_lookup_data ("/org/gnome/shumate/Tests/0.pbf", G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
  g_assert_no_error (error);

  shumate_vector_renderer_render (renderer, tile, tile_data, 0, 0, 0);
  g_assert_nonnull (shumate_tile_get_paintable (tile));
  g_assert_nonnull (shumate_tile_get_symbols (tile));
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/vector-renderer/render", test_vector_renderer_render);

  return g_test_run ();
}
