/*
 * Copyright (C) 2010-2013 Jiri Techet <techet@gmail.com>
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
 * SECTION:shumate-null-tile-source
 * @short_description: A tile source that doesn't load map data from anywhere
 *
 * This map source does not provide any input data to the associated renderer.
 * This can be useful in situations when the tile can be rendered independently
 * of any input such as in the case of the #ShumateErrorTileRenderer.
 */

#include "shumate-null-tile-source.h"

#include "shumate-debug.h"
#include "shumate-bounding-box.h"
#include "shumate-enum-types.h"
#include "shumate-tile.h"

G_DEFINE_TYPE (ShumateNullTileSource, shumate_null_tile_source, SHUMATE_TYPE_TILE_SOURCE)

static void fill_tile (ShumateMapSource *map_source,
    ShumateTile *tile);


static void
shumate_null_tile_source_dispose (GObject *object)
{
  G_OBJECT_CLASS (shumate_null_tile_source_parent_class)->dispose (object);
}


static void
shumate_null_tile_source_finalize (GObject *object)
{
  G_OBJECT_CLASS (shumate_null_tile_source_parent_class)->finalize (object);
}


static void
shumate_null_tile_source_class_init (ShumateNullTileSourceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ShumateMapSourceClass *map_source_class = SHUMATE_MAP_SOURCE_CLASS (klass);

  object_class->dispose = shumate_null_tile_source_dispose;
  object_class->finalize = shumate_null_tile_source_finalize;

  map_source_class->fill_tile = fill_tile;
}


static void
shumate_null_tile_source_init (ShumateNullTileSource *self)
{
  g_return_if_fail (SHUMATE_IS_NULL_TILE_SOURCE (self));
}


/**
 * shumate_null_tile_source_new_full:
 * @renderer: the #ShumateRenderer used to render tiles
 *
 * Constructor of #ShumateNullTileSource.
 *
 * Returns: a constructed #ShumateNullTileSource object
 *
 * Since: 0.8
 */
ShumateNullTileSource *
shumate_null_tile_source_new_full (ShumateRenderer *renderer)
{
  ShumateNullTileSource *source;

  source = g_object_new (SHUMATE_TYPE_NULL_TILE_SOURCE,
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
  g_return_if_fail (SHUMATE_IS_NULL_TILE_SOURCE (map_source));
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
