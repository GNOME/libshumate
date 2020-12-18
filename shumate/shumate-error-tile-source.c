/*
 * Copyright (C) 2010-2013 Jiri Techet <techet@gmail.com>
 * Copyright (C) 2019 Marcus Lundblad <ml@update.uu.se>
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/**
 * SECTION:shumate-error-tile-source
 * @short_description: A tile source that doesn't load map data from anywhere
 */

#include "shumate-error-tile-source.h"

#include "shumate-debug.h"
#include "shumate-enum-types.h"
#include "shumate-tile.h"

static GdkTexture *
texture_new_for_surface (cairo_surface_t *surface)
{
  g_autoptr(GBytes) bytes = NULL;
  GdkTexture *texture;

  g_return_val_if_fail (cairo_surface_get_type (surface) == CAIRO_SURFACE_TYPE_IMAGE, NULL);
  g_return_val_if_fail (cairo_image_surface_get_width (surface) > 0, NULL);
  g_return_val_if_fail (cairo_image_surface_get_height (surface) > 0, NULL);

  bytes = g_bytes_new_with_free_func (cairo_image_surface_get_data (surface),
                                      cairo_image_surface_get_height (surface)
                                      * cairo_image_surface_get_stride (surface),
                                      (GDestroyNotify) cairo_surface_destroy,
                                      cairo_surface_reference (surface));
  
  texture = gdk_memory_texture_new (cairo_image_surface_get_width (surface),
                                    cairo_image_surface_get_height (surface),
                                    GDK_MEMORY_B8G8R8A8_PREMULTIPLIED,
                                    bytes,
                                    cairo_image_surface_get_stride (surface));

  return texture;
}


G_DEFINE_TYPE (ShumateErrorTileSource, shumate_error_tile_source, SHUMATE_TYPE_TILE_SOURCE)


static void
fill_tile (ShumateMapSource *map_source,
           ShumateTile      *tile,
           GCancellable     *cancellable)
{
  g_return_if_fail (SHUMATE_IS_ERROR_TILE_SOURCE (map_source));
  g_return_if_fail (SHUMATE_IS_TILE (tile));

  ShumateMapSource *next_source = shumate_map_source_get_next_source (map_source);

  if (shumate_tile_get_state (tile) == SHUMATE_STATE_DONE)
    return;

  if (shumate_tile_get_state (tile) != SHUMATE_STATE_LOADED)
    {
      g_autoptr(GdkTexture) texture = NULL;
      guint tile_size = shumate_tile_get_size (tile);
      cairo_surface_t *surface;
      cairo_t *cr;
      cairo_pattern_t *pat;

      surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, tile_size, tile_size);
      cr = cairo_create (surface);

      // draw a linear gray to white pattern
      pat = cairo_pattern_create_linear (tile_size / 2.0, 0.0, tile_size, tile_size / 2.0);
      cairo_pattern_add_color_stop_rgb (pat, 0, 0.686, 0.686, 0.686);
      cairo_pattern_add_color_stop_rgb (pat, 1, 0.925, 0.925, 0.925);
      cairo_set_source (cr, pat);
      cairo_rectangle (cr, 0, 0, tile_size, tile_size);
      cairo_fill (cr);

      cairo_pattern_destroy (pat);

      // draw the red cross
      cairo_set_source_rgb (cr, 0.424, 0.078, 0.078);
      cairo_set_line_width (cr, 14.0);
      cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
      cairo_move_to (cr, 24, 24);
      cairo_line_to (cr, 50, 50);
      cairo_move_to (cr, 50, 24);
      cairo_line_to (cr, 24, 50);
      cairo_stroke (cr);

      cairo_destroy (cr);
      cairo_surface_flush (surface);

      texture = texture_new_for_surface (surface);
      cairo_surface_destroy (surface);

      shumate_tile_set_texture (tile, texture);
      shumate_tile_set_fade_in (tile, TRUE);
      shumate_tile_set_state (tile, SHUMATE_STATE_DONE);
    }
  else if (SHUMATE_IS_MAP_SOURCE (next_source))
    shumate_map_source_fill_tile (next_source, tile, cancellable);
  else if (shumate_tile_get_state (tile) == SHUMATE_STATE_LOADED)
    {
      /* if we have some content, use the tile even if it wasn't validated */
      shumate_tile_set_state (tile, SHUMATE_STATE_DONE);
    }
}

static void
shumate_error_tile_source_class_init (ShumateErrorTileSourceClass *klass)
{
  ShumateMapSourceClass *map_source_class = SHUMATE_MAP_SOURCE_CLASS (klass);

  map_source_class->fill_tile = fill_tile;
}


static void
shumate_error_tile_source_init (ShumateErrorTileSource *self)
{
}


/**
 * shumate_error_tile_source_new_full:
 *
 * Constructor of #ShumateErrorTileSource.
 *
 * Returns: a constructed #ShumateErrorTileSource object
 */
ShumateErrorTileSource *
shumate_error_tile_source_new_full (void)
{
  return g_object_new (SHUMATE_TYPE_ERROR_TILE_SOURCE, NULL);
}
