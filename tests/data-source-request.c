#undef G_DISABLE_ASSERT

#include <gtk/gtk.h>
#include <shumate/shumate.h>

static void
test_data_source_request_data (void)
{
  g_autoptr(ShumateDataSourceRequest) req = shumate_data_source_request_new (1, 2, 3);
  const char *data1 = "Hello, world!";
  const char *data2 = "Goodbye!";
  g_autoptr(GBytes) bytes1 = g_bytes_new_static (data1, strlen (data1));
  g_autoptr(GBytes) bytes2 = g_bytes_new_static (data2, strlen (data2));

  g_assert_cmpint (shumate_data_source_request_get_x (req), ==, 1);
  g_assert_cmpint (shumate_data_source_request_get_y (req), ==, 2);
  g_assert_cmpint (shumate_data_source_request_get_zoom_level (req), ==, 3);

  shumate_data_source_request_emit_data (req, bytes1, FALSE);
  g_assert_true (g_bytes_equal (bytes1, shumate_data_source_request_get_data (req)));
  g_assert_false (shumate_data_source_request_is_completed (req));

  shumate_data_source_request_emit_data (req, bytes2, FALSE);
  g_assert_true (g_bytes_equal (bytes2, shumate_data_source_request_get_data (req)));
  g_assert_false (shumate_data_source_request_is_completed (req));

  shumate_data_source_request_emit_data (req, bytes1, TRUE);
  g_assert_true (g_bytes_equal (bytes1, shumate_data_source_request_get_data (req)));
  g_assert_true (shumate_data_source_request_is_completed (req));
}

static void
test_data_source_request_error (void)
{
  g_autoptr(ShumateDataSourceRequest) req = shumate_data_source_request_new (1, 2, 3);
  const char *data1 = "Hello, world!";
  g_autoptr(GBytes) bytes1 = g_bytes_new_static (data1, strlen (data1));
  g_autoptr(GError) error = g_error_new (G_IO_ERROR, G_IO_ERROR_EXISTS, "Error!");

  g_assert_false (shumate_data_source_request_is_completed (req));

  shumate_data_source_request_emit_data (req, bytes1, FALSE);
  g_assert_true (g_bytes_equal (bytes1, shumate_data_source_request_get_data (req)));
  g_assert_false (shumate_data_source_request_is_completed (req));

  shumate_data_source_request_emit_error (req, error);
  g_assert_true (shumate_data_source_request_is_completed (req));
  g_assert_null (shumate_data_source_request_get_data (req));
  g_assert_cmpstr (shumate_data_source_request_get_error (req)->message, ==, "Error!");
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/data-source-request/data", test_data_source_request_data);
  g_test_add_func ("/data-source-request/error", test_data_source_request_error);

  return g_test_run ();
}
