#undef G_DISABLE_ASSERT

#include <gtk/gtk.h>
#include <shumate/shumate.h>

static void
test_vector_style_create (void)
{
  GError *error = NULL;
  g_autoptr(GBytes) style_json = NULL;
  g_autoptr(ShumateVectorRenderer) renderer = NULL;

  style_json = g_resources_lookup_data ("/org/gnome/shumate/Tests/style.json", G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
  g_assert_no_error (error);

  renderer = shumate_vector_renderer_new ("", g_bytes_get_data (style_json, NULL), &error);
  g_assert_no_error (error);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/vector-style/create", test_vector_style_create);

  return g_test_run ();
}
