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
 *
 * This map source does not provide any input data to the associated renderer.
 * This can be useful in situations when the tile can be rendered independently
 * of any input such as in the case of the #ShumateErrorTileRenderer.
 */

#include "shumate-error-tile-source.h"

#include "shumate-debug.h"
#include "shumate-bounding-box.h"
#include "shumate-enum-types.h"
#include "shumate-tile.h"

G_DEFINE_TYPE (ShumateErrorTileSource, shumate_error_tile_source, SHUMATE_TYPE_TILE_SOURCE)

static void fill_tile (ShumateMapSource *map_source,
    ShumateTile *tile);

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
 * @renderer: the #ShumateRenderer used to render tiles
 *
 * Constructor of #ShumateErrorTileSource.
 *
 * Returns: a constructed #ShumateErrorTileSource object
 */
ShumateErrorTileSource *
shumate_error_tile_source_new_full (ShumateRenderer *renderer)
{
  ShumateErrorTileSource *source;

  source = g_object_new (SHUMATE_TYPE_ERROR_TILE_SOURCE,
        "renderer", renderer,
        NULL);
  return source;
}


static void
tile_rendered_cb (ShumateTile *tile,
    gpointer data,
    guint size,
    gboolean error,
    ShumateMapSource *map_source)
{
  ShumateMapSource *next_source;

  g_signal_handlers_disconnect_by_func (tile, tile_rendered_cb, map_source);

  next_source = shumate_map_source_get_next_source (map_source);

  if (!error)
    {
      ShumateTileSource *tile_source = SHUMATE_TILE_SOURCE (map_source);
      ShumateTileCache *tile_cache = shumate_tile_source_get_cache (tile_source);

      if (tile_cache && data)
        shumate_tile_cache_store_tile (tile_cache, tile, data, size);

      shumate_tile_set_fade_in (tile, TRUE);
      shumate_tile_set_state (tile, SHUMATE_STATE_DONE);
      shumate_tile_display_content (tile);
    }
  else if (next_source)
    shumate_map_source_fill_tile (next_source, tile);

  g_object_unref (map_source);
  g_object_unref (tile);
}


static void
fill_tile (ShumateMapSource *map_source,
    ShumateTile *tile)
{
  g_return_if_fail (SHUMATE_IS_ERROR_TILE_SOURCE (map_source));
  g_return_if_fail (SHUMATE_IS_TILE (tile));

  ShumateMapSource *next_source = shumate_map_source_get_next_source (map_source);

  if (shumate_tile_get_state (tile) == SHUMATE_STATE_DONE)
    return;

  if (shumate_tile_get_state (tile) != SHUMATE_STATE_LOADED)
    {
      ShumateRenderer *renderer;

      renderer = shumate_map_source_get_renderer (map_source);

      g_return_if_fail (SHUMATE_IS_RENDERER (renderer));

      g_object_ref (map_source);
      g_object_ref (tile);

      g_signal_connect (tile, "render-complete", G_CALLBACK (tile_rendered_cb), map_source);

      shumate_renderer_render (renderer, tile);
    }
  else if (SHUMATE_IS_MAP_SOURCE (next_source))
    shumate_map_source_fill_tile (next_source, tile);
  else if (shumate_tile_get_state (tile) == SHUMATE_STATE_LOADED)
    {
      /* if we have some content, use the tile even if it wasn't validated */
      shumate_tile_set_state (tile, SHUMATE_STATE_DONE);
      shumate_tile_display_content (tile);
    }
}
