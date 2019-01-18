/*
 * Copyright (C) 2009 Simon Wenner <simon@wenner.ch>
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
 * SECTION:shumate-file-tile-source
 * @short_description: A map source that loads tile data from a file
 *
 * This tile source loads local <ulink role="online-location"
 * url="http://wiki.openstreetmap.org/wiki/.osm">
 * OpenStreetMap XML data files</ulink> (*.osm).
 */

#include "shumate-file-tile-source.h"

#include "shumate-debug.h"
#include "shumate-bounding-box.h"
#include "shumate-enum-types.h"
#include "shumate-tile.h"

G_DEFINE_TYPE (ShumateFileTileSource, shumate_file_tile_source, SHUMATE_TYPE_TILE_SOURCE)

static void fill_tile (ShumateMapSource *map_source,
    ShumateTile *tile);


static void
shumate_file_tile_source_dispose (GObject *object)
{
  G_OBJECT_CLASS (shumate_file_tile_source_parent_class)->dispose (object);
}


static void
shumate_file_tile_source_finalize (GObject *object)
{
  G_OBJECT_CLASS (shumate_file_tile_source_parent_class)->finalize (object);
}


static void
shumate_file_tile_source_class_init (ShumateFileTileSourceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ShumateMapSourceClass *map_source_class = SHUMATE_MAP_SOURCE_CLASS (klass);

  object_class->dispose = shumate_file_tile_source_dispose;
  object_class->finalize = shumate_file_tile_source_finalize;

  map_source_class->fill_tile = fill_tile;
}


static void
shumate_file_tile_source_init (ShumateFileTileSource *self)
{
}


/**
 * shumate_file_tile_source_new_full:
 * @id: the map source's id
 * @name: the map source's name
 * @license: the map source's license
 * @license_uri: the map source's license URI
 * @min_zoom: the map source's minimum zoom level
 * @max_zoom: the map source's maximum zoom level
 * @tile_size: the map source's tile size (in pixels)
 * @projection: the map source's projection
 * @renderer: the #ShumateRenderer used to render tiles
 *
 * Constructor of #ShumateFileTileSource.
 *
 * Returns: a constructed #ShumateFileTileSource object
 *
 * Since: 0.8
 */
ShumateFileTileSource *
shumate_file_tile_source_new_full (const gchar *id,
    const gchar *name,
    const gchar *license,
    const gchar *license_uri,
    guint min_zoom,
    guint max_zoom,
    guint tile_size,
    ShumateMapProjection projection,
    ShumateRenderer *renderer)
{
  ShumateFileTileSource *source;

  source = g_object_new (SHUMATE_TYPE_FILE_TILE_SOURCE,
        "id", id,
        "name", name,
        "license", license,
        "license-uri", license_uri,
        "min-zoom-level", min_zoom,
        "max-zoom-level", max_zoom,
        "tile-size", tile_size,
        "projection", projection,
        "renderer", renderer,
        NULL);
  return source;
}


/**
 * shumate_file_tile_source_load_map_data:
 * @self: a #ShumateFileTileSource
 * @map_path: a path to a map data file
 *
 * Loads the OpenStreetMap XML file at the given path.
 *
 * Since: 0.8
 */
void
shumate_file_tile_source_load_map_data (ShumateFileTileSource *self,
    const gchar *map_path)
{
  g_return_if_fail (SHUMATE_IS_FILE_TILE_SOURCE (self));

  ShumateRenderer *renderer;
  gchar *data;
  gsize length;

  if (!g_file_get_contents (map_path, &data, &length, NULL))
    {
      g_critical ("Error: \"%s\" cannot be read.", map_path);
      return;
    }

  renderer = shumate_map_source_get_renderer (SHUMATE_MAP_SOURCE (self));
  shumate_renderer_set_data (renderer, data, length);
  g_free (data);
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
  g_return_if_fail (SHUMATE_IS_FILE_TILE_SOURCE (map_source));
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
