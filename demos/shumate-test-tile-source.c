/*
 * Copyright (C) 2021 James Westman <james@jwestman.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <https://www.gnu.org/licenses/>.
 */


#include "shumate-test-tile-source.h"

struct _ShumateTestTileSource
{
  ShumateMapSource parent_instance;
};

G_DEFINE_TYPE (ShumateTestTileSource, shumate_test_tile_source, SHUMATE_TYPE_MAP_SOURCE)


ShumateTestTileSource *
shumate_test_tile_source_new (void)
{
  return g_object_new (SHUMATE_TYPE_TEST_TILE_SOURCE,
                       "id", SHUMATE_MAP_SOURCE_TEST,
                       "name", "Test Pattern",
                       NULL);
}


static GdkTexture *
texture_new_for_surface (cairo_surface_t *surface)
{
  g_autoptr(GBytes) bytes = NULL;
  GdkTexture *texture;

  g_return_val_if_fail (cairo_surface_get_type (surface) == CAIRO_SURFACE_TYPE_IMAGE, NULL);
  g_return_val_if_fail (cairo_image_surface_get_width (surface) > 0, NULL);
  g_return_val_if_fail (cairo_image_surface_get_height (surface) > 0, NULL);

  bytes = g_bytes_new_with_free_func (cairo_image_surface_get_data (surface),
                                      (gsize) cairo_image_surface_get_height (surface)
                                      * (gsize) cairo_image_surface_get_stride (surface),
                                      (GDestroyNotify) cairo_surface_destroy,
                                      cairo_surface_reference (surface));

  texture = gdk_memory_texture_new (cairo_image_surface_get_width (surface),
                                    cairo_image_surface_get_height (surface),
                                    GDK_MEMORY_B8G8R8A8_PREMULTIPLIED,
                                    bytes,
                                    cairo_image_surface_get_stride (surface));

  return texture;
}


static void
shumate_test_tile_source_fill_tile_async (ShumateMapSource *map_source,
                                          ShumateTile *tile,
                                          GCancellable *cancellable,
                                          GAsyncReadyCallback callback,
                                          gpointer user_data)
{
  g_autoptr(GdkTexture) texture = NULL;
  g_autoptr(GTask) task = NULL;
  guint tile_size = shumate_tile_get_size (tile);
  int x = shumate_tile_get_x (tile), y = shumate_tile_get_y (tile);
  int zoom = shumate_tile_get_zoom_level (tile);
  int max_zoom = shumate_map_source_get_max_zoom_level (map_source);
  cairo_surface_t *surface;
  cairo_t *cr;
  float r, g, b;
  g_autofree char *text = NULL;

  g_return_if_fail (SHUMATE_IS_TEST_TILE_SOURCE (map_source));
  g_return_if_fail (SHUMATE_IS_TILE (tile));
  g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, tile_size, tile_size);
  cr = cairo_create (surface);

  gtk_hsv_to_rgb (zoom / (double) max_zoom, 1, ((x + y) % 2 == 0) ? 1 : 0.5, &r, &g, &b);
  cairo_set_source_rgb (cr, r, g, b);
  cairo_paint (cr);

  cairo_set_source_rgb (cr, 0, 0, 0);
  cairo_move_to (cr, 8, tile_size - 8);
  cairo_scale (cr, 2, 2);
  text = g_strdup_printf ("%d, %d (z%d)", x, y, zoom);
  cairo_show_text (cr, text);

  texture = texture_new_for_surface (surface);
  cairo_surface_destroy (surface);
  cairo_destroy (cr);

  shumate_tile_set_paintable (tile, GDK_PAINTABLE (texture));
  shumate_tile_set_fade_in (tile, TRUE);
  shumate_tile_set_state (tile, SHUMATE_STATE_DONE);

  task = g_task_new (map_source, cancellable, callback, user_data);
  g_task_set_source_tag (task, shumate_test_tile_source_fill_tile_async);
  g_task_return_boolean (task, TRUE);
}

static gboolean
shumate_test_tile_source_fill_tile_finish (ShumateMapSource *map_source,
                                           GAsyncResult *result,
                                           GError **error)
{
  ShumateTestTileSource *self = (ShumateTestTileSource *)map_source;

  g_return_val_if_fail (SHUMATE_IS_TEST_TILE_SOURCE (self), FALSE);
  g_return_val_if_fail (g_task_is_valid (result, self), FALSE);

  return g_task_propagate_boolean (G_TASK (result), error);
}

static void
shumate_test_tile_source_class_init (ShumateTestTileSourceClass *klass)
{
  ShumateMapSourceClass *map_source_class = SHUMATE_MAP_SOURCE_CLASS (klass);

  map_source_class->fill_tile_async = shumate_test_tile_source_fill_tile_async;
  map_source_class->fill_tile_finish = shumate_test_tile_source_fill_tile_finish;
}


static void
shumate_test_tile_source_init (ShumateTestTileSource *self)
{
}
