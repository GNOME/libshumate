#undef G_DISABLE_ASSERT

#include <shumate/shumate.h>

#define TEST_ETAG "0123456789ABCDEFG"
#define TEST_DATA "The quick brown fox \0 jumps over the lazy dog"


static void
on_tile_stored (GObject *object, GAsyncResult *res, gpointer user_data)
{
  g_autoptr(GError) error = NULL;
  GMainLoop *loop = user_data;

  shumate_file_cache_store_tile_finish ((ShumateFileCache *) object, res, &error);
  g_assert_no_error (error);

  g_main_loop_quit (loop);
}

static void
on_tile_retrieved (GObject *object, GAsyncResult *res, gpointer user_data)
{
  GMainLoop *loop = user_data;
  g_autoptr(GError) error = NULL;
  g_autoptr(GBytes) bytes = NULL;
  g_autofree char *etag = NULL;
  g_autoptr(GBytes) expected_bytes = g_bytes_new_static (TEST_DATA, sizeof TEST_DATA);
  g_autoptr(GDateTime) modtime = NULL;
  g_autoptr(GDateTime) now = g_date_time_new_now_utc ();

  bytes = shumate_file_cache_get_tile_finish ((ShumateFileCache *) object, &etag, &modtime, res, &error);
  g_assert_no_error (error);

  g_assert_true (g_bytes_equal (bytes, expected_bytes));
  g_assert_cmpstr (etag, ==, TEST_ETAG);
  /* the modification time should be very, very recent */
  g_assert_true (g_date_time_difference (now, modtime) < G_TIME_SPAN_SECOND * 10);

  g_main_loop_quit (loop);
}

/* Test that storing and retrieving a file from the cache works */
static void
test_file_cache_store_retrieve ()
{
  g_autoptr(ShumateFileCache) cache = shumate_file_cache_new_full (100000000, "test", NULL);
  g_autoptr(GBytes) bytes = g_bytes_new_static (TEST_DATA, sizeof TEST_DATA);
  g_autoptr(GMainLoop) loop = NULL;

  /* Store the tile */
  loop = g_main_loop_new (NULL, TRUE);
  shumate_file_cache_store_tile_async (cache, 0, 0, 256, bytes, TEST_ETAG, NULL, on_tile_stored, loop);
  g_main_loop_run (loop);

  /* Now retrieve it */
  g_main_loop_unref (loop);
  loop = g_main_loop_new (NULL, TRUE);
  shumate_file_cache_get_tile_async (cache, 0, 0, 256, NULL, on_tile_retrieved, loop);
  g_main_loop_run (loop);
}


static void
on_no_tile_retrieved (GObject *object, GAsyncResult *res, gpointer user_data)
{
  GMainLoop *loop = user_data;
  g_autoptr(GError) error = NULL;
  g_autoptr(GBytes) bytes = NULL;
  g_autofree char *etag = NULL;
  g_autoptr(GDateTime) modtime = NULL;

  /* Make sure retrieving the tile returns NULL */
  bytes = shumate_file_cache_get_tile_finish ((ShumateFileCache *) object, &etag, &modtime, res, &error);
  g_assert_no_error (error);
  g_assert_null (bytes);
  g_assert_null (etag);
  g_assert_null (modtime);

  g_main_loop_quit (loop);
}

/* Test that cache misses work properly */
static void
test_file_cache_miss ()
{
  g_autoptr(ShumateFileCache) cache = shumate_file_cache_new_full (100000000, "test", NULL);
  g_autoptr(GMainLoop) loop = NULL;

  loop = g_main_loop_new (NULL, TRUE);
  shumate_file_cache_get_tile_async (cache, 0, 0, 256, NULL, on_no_tile_retrieved, loop);
  g_main_loop_run (loop);
}


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, G_TEST_OPTION_ISOLATE_DIRS, NULL);

  g_test_add_func ("/file-cache/store-retrieve", test_file_cache_store_retrieve);
  g_test_add_func ("/file-cache/miss", test_file_cache_miss);

  return g_test_run ();
}
