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
 * SECTION:shumate-memory-cache
 * @short_description: Stores and loads cached tiles from the memory
 *
 * #ShumateMemoryCache is a cache that stores and retrieves tiles from the
 * memory. The cache contents is not preserved between application restarts
 * so this cache serves mostly as a quick access temporary cache to the
 * most recently used tiles.
 */

#define DEBUG_FLAG SHUMATE_DEBUG_CACHE
#include "shumate-debug.h"

#include "shumate-memory-cache.h"

#include <glib.h>
#include <string.h>

G_DEFINE_TYPE (ShumateMemoryCache, shumate_memory_cache, SHUMATE_TYPE_TILE_CACHE);

#define GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), SHUMATE_TYPE_MEMORY_CACHE, ShumateMemoryCachePrivate))

enum
{
  PROP_0,
  PROP_SIZE_LIMIT
};

struct _ShumateMemoryCachePrivate
{
  guint size_limit;
  GQueue *queue;
  GHashTable *hash_table;
};

typedef struct
{
  gchar *key;
  gchar *data;
  guint size;
} QueueMember;


static void fill_tile (ShumateMapSource *map_source,
    ShumateTile *tile);

static void store_tile (ShumateTileCache *tile_cache,
    ShumateTile *tile,
    const gchar *contents,
    gsize size);
static void refresh_tile_time (ShumateTileCache *tile_cache,
    ShumateTile *tile);
static void on_tile_filled (ShumateTileCache *tile_cache,
    ShumateTile *tile);


static void
shumate_memory_cache_get_property (GObject *object,
    guint property_id,
    GValue *value,
    GParamSpec *pspec)
{
  ShumateMemoryCache *memory_cache = SHUMATE_MEMORY_CACHE (object);

  switch (property_id)
    {
    case PROP_SIZE_LIMIT:
      g_value_set_uint (value, shumate_memory_cache_get_size_limit (memory_cache));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}


static void
shumate_memory_cache_set_property (GObject *object,
    guint property_id,
    const GValue *value,
    GParamSpec *pspec)
{
  ShumateMemoryCache *memory_cache = SHUMATE_MEMORY_CACHE (object);

  switch (property_id)
    {
    case PROP_SIZE_LIMIT:
      shumate_memory_cache_set_size_limit (memory_cache, g_value_get_uint (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}


static void
shumate_memory_cache_dispose (GObject *object)
{
  G_OBJECT_CLASS (shumate_memory_cache_parent_class)->dispose (object);
}


static void
shumate_memory_cache_finalize (GObject *object)
{
  ShumateMemoryCache *memory_cache = SHUMATE_MEMORY_CACHE (object);

  shumate_memory_cache_clean (memory_cache);
  g_queue_free (memory_cache->priv->queue);
  g_hash_table_destroy (memory_cache->priv->hash_table);

  G_OBJECT_CLASS (shumate_memory_cache_parent_class)->finalize (object);
}


static void
shumate_memory_cache_class_init (ShumateMemoryCacheClass *klass)
{
  ShumateMapSourceClass *map_source_class = SHUMATE_MAP_SOURCE_CLASS (klass);
  ShumateTileCacheClass *tile_cache_class = SHUMATE_TILE_CACHE_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (ShumateMemoryCachePrivate));

  object_class->finalize = shumate_memory_cache_finalize;
  object_class->dispose = shumate_memory_cache_dispose;
  object_class->get_property = shumate_memory_cache_get_property;
  object_class->set_property = shumate_memory_cache_set_property;

  /**
   * ShumateMemoryCache:size-limit:
   *
   * The maximum number of tiles that are stored in the cache.
   */
  pspec = g_param_spec_uint ("size-limit",
        "Size Limit",
        "Maximal number of stored tiles",
        1,
        G_MAXINT,
        100,
        G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_SIZE_LIMIT, pspec);

  tile_cache_class->store_tile = store_tile;
  tile_cache_class->refresh_tile_time = refresh_tile_time;
  tile_cache_class->on_tile_filled = on_tile_filled;

  map_source_class->fill_tile = fill_tile;
}


/**
 * shumate_memory_cache_new_full:
 * @size_limit: maximum number of tiles stored in the cache
 * @renderer: the #ShumateRenderer used for tiles rendering
 *
 * Constructor of #ShumateMemoryCache.
 *
 * Returns: a constructed #ShumateMemoryCache
 */
ShumateMemoryCache *
shumate_memory_cache_new_full (guint size_limit,
    ShumateRenderer *renderer)
{
  ShumateMemoryCache *cache;

  cache = g_object_new (SHUMATE_TYPE_MEMORY_CACHE,
        "size-limit", size_limit,
        "renderer", renderer,
        NULL);

  return cache;
}


static void
shumate_memory_cache_init (ShumateMemoryCache *memory_cache)
{
  ShumateMemoryCachePrivate *priv = GET_PRIVATE (memory_cache);

  memory_cache->priv = priv;

  priv->queue = g_queue_new ();
  priv->hash_table = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
}


/**
 * shumate_memory_cache_get_size_limit:
 * @memory_cache: a #ShumateMemoryCache
 *
 * Gets the maximum number of tiles stored in the cache.
 *
 * Returns: maximum number of stored tiles
 */
guint
shumate_memory_cache_get_size_limit (ShumateMemoryCache *memory_cache)
{
  g_return_val_if_fail (SHUMATE_IS_MEMORY_CACHE (memory_cache), 0);

  return memory_cache->priv->size_limit;
}


/**
 * shumate_memory_cache_set_size_limit:
 * @memory_cache: a #ShumateMemoryCache
 * @size_limit: maximum number of tiles stored in the cache
 *
 * Sets the maximum number of tiles stored in the cache.
 */
void
shumate_memory_cache_set_size_limit (ShumateMemoryCache *memory_cache,
    guint size_limit)
{
  g_return_if_fail (SHUMATE_IS_MEMORY_CACHE (memory_cache));

  ShumateMemoryCachePrivate *priv = memory_cache->priv;

  priv->size_limit = size_limit;
  g_object_notify (G_OBJECT (memory_cache), "size-limit");
}


static gchar *
generate_queue_key (ShumateMemoryCache *memory_cache,
    ShumateTile *tile)
{
  g_return_val_if_fail (SHUMATE_IS_MEMORY_CACHE (memory_cache), NULL);
  g_return_val_if_fail (SHUMATE_IS_TILE (tile), NULL);

  ShumateMapSource *map_source = SHUMATE_MAP_SOURCE (memory_cache);
  gchar *key;

  key = g_strdup_printf ("%d/%d/%d/%s",
        shumate_tile_get_zoom_level (tile),
        shumate_tile_get_x (tile),
        shumate_tile_get_y (tile),
        shumate_map_source_get_id (map_source));
  return key;
}


static void
move_queue_member_to_head (GQueue *queue, GList *link)
{
  g_queue_unlink (queue, link);
  g_queue_push_head_link (queue, link);
}


static void
delete_queue_member (QueueMember *member, gpointer user_data)
{
  if (member)
    {
      g_free (member->key);
      g_free (member->data);
      g_slice_free (QueueMember, member);
    }
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
      if (SHUMATE_IS_TILE_CACHE (next_source))
        shumate_tile_cache_on_tile_filled (SHUMATE_TILE_CACHE (next_source), tile);

      shumate_tile_set_fade_in (tile, FALSE);
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
  g_return_if_fail (SHUMATE_IS_MEMORY_CACHE (map_source));
  g_return_if_fail (SHUMATE_IS_TILE (tile));

  ShumateMapSource *next_source = shumate_map_source_get_next_source (map_source);

  if (shumate_tile_get_state (tile) == SHUMATE_STATE_DONE)
    return;

  if (shumate_tile_get_state (tile) != SHUMATE_STATE_LOADED)
    {
      ShumateMemoryCache *memory_cache = SHUMATE_MEMORY_CACHE (map_source);
      ShumateMemoryCachePrivate *priv = memory_cache->priv;
      ShumateRenderer *renderer;
      GList *link;
      gchar *key;

      key = generate_queue_key (memory_cache, tile);
      link = g_hash_table_lookup (priv->hash_table, key);
      g_free (key);
      if (link)
        {
          QueueMember *member = link->data;

          move_queue_member_to_head (priv->queue, link);

          renderer = shumate_map_source_get_renderer (map_source);

          g_return_if_fail (SHUMATE_IS_RENDERER (renderer));

          g_object_ref (map_source);
          g_object_ref (tile);

          g_signal_connect (tile, "render-complete", G_CALLBACK (tile_rendered_cb), map_source);

          shumate_renderer_set_data (renderer, (guint8) member->data, member->size);
          shumate_renderer_render (renderer, tile);

          return;
        }
    }

  if (SHUMATE_IS_MAP_SOURCE (next_source))
    shumate_map_source_fill_tile (next_source, tile);
  else if (shumate_tile_get_state (tile) == SHUMATE_STATE_LOADED)
    {
      /* if we have some content, use the tile even if it wasn't validated */
      shumate_tile_set_state (tile, SHUMATE_STATE_DONE);
      shumate_tile_display_content (tile);
    }
}


static void
store_tile (ShumateTileCache *tile_cache,
    ShumateTile *tile,
    const gchar *contents,
    gsize size)
{
  g_return_if_fail (SHUMATE_IS_MEMORY_CACHE (tile_cache));

  ShumateMapSource *map_source = SHUMATE_MAP_SOURCE (tile_cache);
  ShumateMapSource *next_source = shumate_map_source_get_next_source (map_source);
  ShumateMemoryCache *memory_cache = SHUMATE_MEMORY_CACHE (tile_cache);
  ShumateMemoryCachePrivate *priv = memory_cache->priv;
  GList *link;
  gchar *key;

  key = generate_queue_key (memory_cache, tile);
  link = g_hash_table_lookup (priv->hash_table, key);
  if (link)
    {
      move_queue_member_to_head (priv->queue, link);
      g_free (key);
    }
  else
    {
      QueueMember *member;

      if (priv->queue->length >= priv->size_limit)
        {
          member = g_queue_pop_tail (priv->queue);
          g_hash_table_remove (priv->hash_table, member->key);
          delete_queue_member (member, NULL);
        }

      member = g_slice_new (QueueMember);
      member->key = key;
      member->data = g_memdup (contents, size);
      member->size = size;

      g_queue_push_head (priv->queue, member);
      g_hash_table_insert (priv->hash_table, g_strdup (key), g_queue_peek_head_link (priv->queue));
    }

  if (SHUMATE_IS_TILE_CACHE (next_source))
    shumate_tile_cache_store_tile (SHUMATE_TILE_CACHE (next_source), tile, contents, size);
}


static void
refresh_tile_time (ShumateTileCache *tile_cache,
    ShumateTile *tile)
{
  g_return_if_fail (SHUMATE_IS_MEMORY_CACHE (tile_cache));

  ShumateMapSource *map_source = SHUMATE_MAP_SOURCE (tile_cache);
  ShumateMapSource *next_source = shumate_map_source_get_next_source (map_source);

  if (SHUMATE_IS_TILE_CACHE (next_source))
    shumate_tile_cache_refresh_tile_time (SHUMATE_TILE_CACHE (next_source), tile);
}


/**
 * shumate_memory_cache_clean:
 * @memory_cache: a #ShumateMemoryCache
 *
 * Cleans the contents of the cache.
 */
void
shumate_memory_cache_clean (ShumateMemoryCache *memory_cache)
{
  ShumateMemoryCachePrivate *priv = memory_cache->priv;

  g_queue_foreach (priv->queue, (GFunc) delete_queue_member, NULL);
  g_queue_clear (priv->queue);
  g_hash_table_destroy (memory_cache->priv->hash_table);
  priv->hash_table = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
}


static void
on_tile_filled (ShumateTileCache *tile_cache,
    ShumateTile *tile)
{
  g_return_if_fail (SHUMATE_IS_MEMORY_CACHE (tile_cache));
  g_return_if_fail (SHUMATE_IS_TILE (tile));

  ShumateMapSource *map_source = SHUMATE_MAP_SOURCE (tile_cache);
  ShumateMapSource *next_source = shumate_map_source_get_next_source (map_source);
  ShumateMemoryCache *memory_cache = SHUMATE_MEMORY_CACHE (tile_cache);
  ShumateMemoryCachePrivate *priv = memory_cache->priv;
  GList *link;
  gchar *key;

  key = generate_queue_key (memory_cache, tile);
  link = g_hash_table_lookup (priv->hash_table, key);
  g_free (key);
  if (link)
    move_queue_member_to_head (priv->queue, link);

  if (SHUMATE_IS_TILE_CACHE (next_source))
    shumate_tile_cache_on_tile_filled (SHUMATE_TILE_CACHE (next_source), tile);
}
