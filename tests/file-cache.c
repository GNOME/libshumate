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
on_tile_retrieved_no_etag (GObject *object, GAsyncResult *res, gpointer user_data)
{
  g_autoptr(GError) error = NULL;
  GMainLoop *loop = user_data;
  g_autofree char *etag = NULL;
  g_autoptr(GBytes) bytes = NULL;
  g_autoptr(GBytes) expected_bytes = g_bytes_new_static (TEST_DATA, sizeof TEST_DATA);

  /* Make sure the tile is there and the data is correct */
  bytes = shumate_file_cache_get_tile_finish ((ShumateFileCache *) object, &etag, res, &error);
  g_assert_no_error (error);
  g_assert_true (g_bytes_equal (bytes, expected_bytes));

  /* There should be no etag because the tile is recent */
  g_assert_null (etag);

  g_main_loop_quit (loop);
}

/* Test that storing and retrieving a file from the cache works */
static void
test_file_cache_store_retrieve ()
{
  g_autoptr(ShumateFileCache) cache = shumate_file_cache_new_full (100000000, "test", NULL);
  g_autoptr(ShumateTile) tile = shumate_tile_new_full (0, 0, 256, 0);
  g_autoptr(GBytes) bytes = g_bytes_new_static (TEST_DATA, sizeof TEST_DATA);
  g_autoptr(GMainLoop) loop = NULL;

  g_object_ref_sink (tile);

  /* Store the tile */
  loop = g_main_loop_new (NULL, TRUE);
  shumate_file_cache_store_tile_async (cache, tile, bytes, TEST_ETAG, NULL, on_tile_stored, loop);
  g_main_loop_run (loop);

  /* Now retrieve it */
  g_main_loop_unref (loop);
  loop = g_main_loop_new (NULL, TRUE);
  shumate_file_cache_get_tile_async (cache, tile, NULL, on_tile_retrieved_no_etag, loop);
  g_main_loop_run (loop);
}


static void
on_no_tile_retrieved (GObject *object, GAsyncResult *res, gpointer user_data)
{
  GMainLoop *loop = user_data;
  g_autoptr(GError) error = NULL;
  g_autoptr(GBytes) bytes = NULL;
  g_autofree char *etag = NULL;

  /* Make sure retrieving the tile returns NULL */
  bytes = shumate_file_cache_get_tile_finish ((ShumateFileCache *) object, &etag, res, &error);
  g_assert_no_error (error);
  g_assert_null (bytes);
  g_assert_null (etag);

  g_main_loop_quit (loop);
}

/* Test that cache misses work properly */
static void
test_file_cache_miss ()
{
  g_autoptr(ShumateFileCache) cache = shumate_file_cache_new_full (100000000, "test", NULL);
  g_autoptr(ShumateTile) tile = shumate_tile_new_full (0, 0, 256, 0);
  g_autoptr(GMainLoop) loop = NULL;

  g_object_ref_sink (tile);

  loop = g_main_loop_new (NULL, TRUE);
  shumate_file_cache_get_tile_async (cache, tile, NULL, on_no_tile_retrieved, loop);
  g_main_loop_run (loop);
}



static void
on_tile_retrieved_with_etag (GObject *object, GAsyncResult *res, gpointer user_data)
{
  g_autoptr(GError) error = NULL;
  GMainLoop *loop = user_data;
  g_autofree char *etag = NULL;
  g_autoptr(GBytes) bytes = NULL;
  g_autoptr(GBytes) expected_bytes = g_bytes_new_static (TEST_DATA, sizeof TEST_DATA);

  /* Make sure the tile is there and the data is correct */
  bytes = shumate_file_cache_get_tile_finish ((ShumateFileCache *) object, &etag, res, &error);
  g_assert_no_error (error);
  g_assert_true (g_bytes_equal (bytes, expected_bytes));
  g_assert_cmpstr (etag, ==, TEST_ETAG);

  g_main_loop_quit (loop);
}

/* Test that potentially out-of-date tile data returns an ETag */
static void
test_file_cache_etag ()
{
  g_autoptr(ShumateFileCache) cache = shumate_file_cache_new_full (100000000, "test", NULL);
  g_autoptr(ShumateTile) tile = shumate_tile_new_full (0, 0, 256, 0);
  g_autoptr(GBytes) bytes = g_bytes_new_static (TEST_DATA, sizeof TEST_DATA);
  g_autoptr(GMainLoop) loop = NULL;
  g_autoptr(GFile) file = NULL;
  guint64 mod_time = 1000;

  g_object_ref_sink (tile);

  /* Store the tile */
  loop = g_main_loop_new (NULL, TRUE);
  shumate_file_cache_store_tile_async (cache, tile, bytes, TEST_ETAG, NULL, on_tile_stored, loop);
  g_main_loop_run (loop);

  /* Change the modified date of the file */
  file = g_file_new_build_filename (g_get_user_cache_dir(), "shumate/test/0/0/0.png", NULL);
  g_file_set_attribute (file, G_FILE_ATTRIBUTE_TIME_MODIFIED,
                        G_FILE_ATTRIBUTE_TYPE_UINT64, &mod_time,
                        G_FILE_QUERY_INFO_NONE, NULL, NULL);

  /* Now retrieve it */
  g_main_loop_unref (loop);
  loop = g_main_loop_new (NULL, TRUE);
  shumate_file_cache_get_tile_async (cache, tile, NULL, on_tile_retrieved_with_etag, loop);
  g_main_loop_run (loop);

  /* Mark the tile up to date */
  shumate_file_cache_mark_up_to_date (cache, tile);

  /* There should be no ETag this time */
  g_main_loop_unref (loop);
  loop = g_main_loop_new (NULL, TRUE);
  shumate_file_cache_get_tile_async (cache, tile, NULL, on_tile_retrieved_no_etag, loop);
  g_main_loop_run (loop);
}


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, G_TEST_OPTION_ISOLATE_DIRS, NULL);
  gtk_init ();

  g_test_add_func ("/file-cache/store-retrieve", test_file_cache_store_retrieve);
  g_test_add_func ("/file-cache/miss", test_file_cache_miss);
  g_test_add_func ("/file-cache/etag", test_file_cache_etag);

  return g_test_run ();
}
