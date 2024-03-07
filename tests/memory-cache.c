#undef G_DISABLE_ASSERT

#include <shumate/shumate.h>
#include "shumate/shumate-memory-cache-private.h"

static GdkPaintable *
create_paintable ()
{
  g_autoptr(GdkPixbuf) pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, 256, 256);
  return GDK_PAINTABLE (gdk_texture_new_for_pixbuf (pixbuf));
}


/* Test that storing and retrieving a texture from the cache works */
static void
test_memory_cache_store_retrieve ()
{
  g_autoptr(ShumateMemoryCache) cache = shumate_memory_cache_new_full (100);
  g_autoptr(ShumateTile) tile = shumate_tile_new_full (0, 0, 256, 0);
  g_autoptr(GdkPaintable) paintable = create_paintable ();

  /* Store the tile */
  shumate_tile_set_paintable (tile, paintable);
  shumate_memory_cache_store_tile (cache, tile, "A");

  /* Now retrieve it */
  g_assert_true (shumate_memory_cache_try_fill_tile (cache, tile, "A"));
  g_assert_true (paintable == shumate_tile_get_paintable (tile));
}


/* Test that cache misses work properly */
static void
test_memory_cache_miss ()
{
  g_autoptr(ShumateMemoryCache) cache = shumate_memory_cache_new_full (100);
  g_autoptr(ShumateTile) tile1 = shumate_tile_new_full (0, 0, 256, 0);
  g_autoptr(ShumateTile) tile2 = shumate_tile_new_full (0, 0, 256, 1);
  g_autoptr(GdkPaintable) paintable = create_paintable ();

  /* Store a tile */
  shumate_tile_set_paintable (tile1, paintable);
  shumate_memory_cache_store_tile (cache, tile1, "A");

  /* Now retrieve a different one */
  g_assert_false (shumate_memory_cache_try_fill_tile (cache, tile2, "A"));
  g_assert_null (shumate_tile_get_paintable (tile2));
}


/* Test that multiple sources can be cached in parallel */
static void
test_memory_cache_source_id ()
{
  g_autoptr(ShumateMemoryCache) cache = shumate_memory_cache_new_full (100);
  g_autoptr(ShumateTile) tile1 = shumate_tile_new_full (0, 0, 256, 0);
  g_autoptr(ShumateTile) tile2 = shumate_tile_new_full (0, 0, 256, 0);
  g_autoptr(GdkPaintable) paintable1 = create_paintable ();
  g_autoptr(GdkPaintable) paintable2 = create_paintable ();

  /* Store the tiles */
  shumate_tile_set_paintable (tile1, paintable1);
  shumate_tile_set_paintable (tile2, paintable2);
  shumate_memory_cache_store_tile (cache, tile1, "A");
  shumate_memory_cache_store_tile (cache, tile2, "B");

  /* Now retrieve them */
  g_assert_true (shumate_memory_cache_try_fill_tile (cache, tile1, "A"));
  g_assert_true (paintable1 == shumate_tile_get_paintable (tile1));

  g_assert_true (shumate_memory_cache_try_fill_tile (cache, tile2, "B"));
  g_assert_true (paintable2 == shumate_tile_get_paintable (tile2));
}


/* Test that the cache is purged properly */
static void
test_memory_cache_purge ()
{
  g_autoptr(ShumateMemoryCache) cache = shumate_memory_cache_new_full (3);
  g_autoptr(ShumateTile) tile = shumate_tile_new_full (0, 0, 256, 0);

  /* Store a few tiles */
  shumate_memory_cache_store_tile (cache, tile, "A");
  shumate_memory_cache_store_tile (cache, tile, "B");
  shumate_memory_cache_store_tile (cache, tile, "C");

  /* Make sure they're all still cached */
  g_assert_true (shumate_memory_cache_try_fill_tile (cache, tile, "B"));
  g_assert_true (shumate_memory_cache_try_fill_tile (cache, tile, "A"));
  g_assert_true (shumate_memory_cache_try_fill_tile (cache, tile, "C"));

  /* Store another one */
  shumate_memory_cache_store_tile (cache, tile, "D");

  /* Since B was the least recently accessed, it should be the one that was
   * dropped */
  g_assert_false (shumate_memory_cache_try_fill_tile (cache, tile, "B"));
  g_assert_true (shumate_memory_cache_try_fill_tile (cache, tile, "A"));
  g_assert_true (shumate_memory_cache_try_fill_tile (cache, tile, "C"));
  g_assert_true (shumate_memory_cache_try_fill_tile (cache, tile, "D"));
}


/* Test that cleaning the cache works */
static void
test_memory_cache_clean ()
{
  g_autoptr(ShumateMemoryCache) cache = shumate_memory_cache_new_full (100);
  g_autoptr(ShumateTile) tile = shumate_tile_new_full (0, 0, 256, 0);

  /* Store a tile */
  shumate_memory_cache_store_tile (cache, tile, "A");

  /* Clean the cache */
  shumate_memory_cache_clean (cache);

  /* Make sure it worked */
  g_assert_false (shumate_memory_cache_try_fill_tile (cache, tile, "A"));
}


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/file-cache/store-retrieve", test_memory_cache_store_retrieve);
  g_test_add_func ("/file-cache/miss", test_memory_cache_miss);
  g_test_add_func ("/file-cache/source-id", test_memory_cache_source_id);
  g_test_add_func ("/file-cache/purge", test_memory_cache_purge);
  g_test_add_func ("/file-cache/clean", test_memory_cache_clean);

  return g_test_run ();
}
