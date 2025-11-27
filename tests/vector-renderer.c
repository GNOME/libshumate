#undef G_DISABLE_ASSERT

#include <glib-object.h>
#include <shumate/shumate.h>
#include "shumate/shumate-tile-private.h"
#include "shumate/shumate-vector-renderer-private.h"
#include "shumate/shumate-utils-private.h"
#include "shumate/shumate-vector-value-private.h"

static void
test_vector_renderer_render (void)
{
  GError *error = NULL;
  g_autoptr(GBytes) style_json = NULL;
  g_autoptr(GBytes) tile_data = NULL;
  g_autoptr(ShumateVectorRenderer) renderer = NULL;
  g_autoptr(ShumateTile) tile = shumate_tile_new_full (0, 0, 512, 0);
  g_autoptr(GdkPaintable) paintable = NULL;
  g_autoptr(GPtrArray) symbols = NULL;
  ShumateGridPosition source_position = { 0, 0, 0 };

  style_json = g_resources_lookup_data ("/org/gnome/shumate/Tests/style.json", G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
  g_assert_no_error (error);

  renderer = shumate_vector_renderer_new ("", g_bytes_get_data (style_json, NULL), &error);
  g_assert_no_error (error);

  tile_data = g_resources_lookup_data ("/org/gnome/shumate/Tests/0.pbf", G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
  g_assert_no_error (error);

  shumate_vector_renderer_render (renderer, tile, tile_data, &source_position, &paintable, &symbols);
  g_assert_no_error (error);
  g_assert_true (GDK_IS_PAINTABLE (paintable));
  g_assert_nonnull (symbols);
}

void
test_vector_renderer_global_state (void)
{
  GError *error = NULL;
  g_autoptr(GBytes) style_json = NULL;
  g_autoptr(ShumateVectorRenderer) renderer = NULL;
  g_auto(ShumateVectorValue) in_value = SHUMATE_VECTOR_VALUE_INIT;
  ShumateVectorValue *out_value = NULL;
  double number;

  style_json = g_resources_lookup_data ("/org/gnome/shumate/Tests/style.json", G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
  g_assert_no_error (error);

  renderer = shumate_vector_renderer_new ("", g_bytes_get_data (style_json, NULL), &error);
  g_assert_no_error (error);

  out_value = shumate_vector_renderer_get_global_state (renderer, "test_number");
  g_assert_true (shumate_vector_value_get_number (out_value, &number));
  g_assert_cmpfloat (number, ==, 1.0);

  shumate_vector_value_set_number (&in_value, 42.0);
  shumate_vector_renderer_set_global_state (renderer, "test_number", &in_value);
  shumate_vector_value_unset (&in_value);
  out_value = shumate_vector_renderer_get_global_state (renderer, "test_number");
  g_assert_true (shumate_vector_value_get_number (out_value, &number));
  g_assert_cmpfloat (number, ==, 42.0);

  shumate_vector_renderer_reset_global_state (renderer, "test_number");
  out_value = shumate_vector_renderer_get_global_state (renderer, "test_number");
  g_assert_true (shumate_vector_value_get_number (out_value, &number));
  g_assert_cmpfloat (number, ==, 1.0);

  out_value = shumate_vector_renderer_get_global_state (renderer, "non_existent_key");
  g_assert_null (out_value);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/vector-renderer/render", test_vector_renderer_render);
  g_test_add_func ("/vector-renderer/global-state", test_vector_renderer_global_state);

  return g_test_run ();
}
