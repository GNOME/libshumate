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
 * ShumateMemoryCache:
 *
 * A cache that stores and retrieves tiles from the memory. The cache contents
 * is not preserved between application restarts so this cache serves mostly as
 * a quick access temporary cache to the most recently used tiles.
 */

#include "shumate-memory-cache-private.h"
#include "shumate-tile-private.h"

#include <glib.h>
#include <string.h>

enum
{
  PROP_0,
  PROP_SIZE_LIMIT,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];

struct _ShumateMemoryCache
{
  GObject parent_instance;

  guint size_limit;
  GQueue *queue;
  GHashTable *hash_table;
};

G_DEFINE_TYPE (ShumateMemoryCache, shumate_memory_cache, G_TYPE_OBJECT);

typedef struct
{
  char *key;
  GdkPaintable *paintable;
  GPtrArray *symbols;
} QueueMember;


static void
shumate_memory_cache_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
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
shumate_memory_cache_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
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
shumate_memory_cache_finalize (GObject *object)
{
  ShumateMemoryCache *self = SHUMATE_MEMORY_CACHE (object);

  shumate_memory_cache_clean (self);
  g_clear_pointer (&self->queue, g_queue_free);
  g_clear_pointer (&self->hash_table, g_hash_table_unref);

  G_OBJECT_CLASS (shumate_memory_cache_parent_class)->finalize (object);
}


static void
shumate_memory_cache_class_init (ShumateMemoryCacheClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = shumate_memory_cache_finalize;
  object_class->get_property = shumate_memory_cache_get_property;
  object_class->set_property = shumate_memory_cache_set_property;

  properties[PROP_SIZE_LIMIT] =
    g_param_spec_uint ("size-limit",
                       "Size Limit",
                       "Maximal number of stored tiles",
                       1,
                       G_MAXINT,
                       100,
                       G_PARAM_CONSTRUCT | G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, N_PROPS, properties);
}


/**
 * shumate_memory_cache_new_full:
 * @size_limit: maximum number of tiles stored in the cache
 *
 * Constructor of #ShumateMemoryCache.
 *
 * Returns: a constructed #ShumateMemoryCache
 */
ShumateMemoryCache *
shumate_memory_cache_new_full (guint size_limit)
{
  ShumateMemoryCache *cache;

  cache = g_object_new (SHUMATE_TYPE_MEMORY_CACHE,
                        "size-limit", size_limit,
                        NULL);

  return cache;
}


static void
shumate_memory_cache_init (ShumateMemoryCache *self)
{
  self->queue = g_queue_new ();
  self->hash_table = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
}


guint
shumate_memory_cache_get_size_limit (ShumateMemoryCache *self)
{
  g_return_val_if_fail (SHUMATE_IS_MEMORY_CACHE (self), 0);

  return self->size_limit;
}


void
shumate_memory_cache_set_size_limit (ShumateMemoryCache *self,
                                     guint               size_limit)
{
  g_return_if_fail (SHUMATE_IS_MEMORY_CACHE (self));

  self->size_limit = size_limit;
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SIZE_LIMIT]);
}


static char *
generate_queue_key (ShumateMemoryCache *self,
                    ShumateTile        *tile,
                    const char         *source_id)
{
  g_return_val_if_fail (SHUMATE_IS_MEMORY_CACHE (self), NULL);
  g_return_val_if_fail (SHUMATE_IS_TILE (tile), NULL);

  char *key;

  key = g_strdup_printf ("%d/%d/%d/%s",
        shumate_tile_get_zoom_level (tile),
        shumate_tile_get_x (tile),
        shumate_tile_get_y (tile),
        source_id);
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
      g_clear_object (&member->paintable);
      g_clear_pointer (&member->symbols, g_ptr_array_unref);
      g_clear_pointer (&member->key, g_free);
      g_free (member);
    }
}


void
shumate_memory_cache_clean (ShumateMemoryCache *self)
{
  g_queue_foreach (self->queue, (GFunc) delete_queue_member, NULL);
  g_queue_clear (self->queue);
  g_hash_table_unref (self->hash_table);
  self->hash_table = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
}


gboolean
shumate_memory_cache_try_fill_tile (ShumateMemoryCache *self,
                                    ShumateTile        *tile,
                                    const char         *source_id)
{
  ShumateMemoryCache *memory_cache = (ShumateMemoryCache *) self;
  GList *link;
  QueueMember *member;
  g_autofree char *key = NULL;

  g_return_val_if_fail (SHUMATE_IS_MEMORY_CACHE (self), FALSE);
  g_return_val_if_fail (SHUMATE_IS_TILE (tile), FALSE);

  key = generate_queue_key (memory_cache, tile, source_id);

  link = g_hash_table_lookup (self->hash_table, key);
  if (link == NULL)
    return FALSE;

  member = link->data;

  move_queue_member_to_head (self->queue, link);

  shumate_tile_set_paintable (tile, member->paintable);
  shumate_tile_set_symbols (tile, member->symbols);
  shumate_tile_set_fade_in (tile, FALSE);
  shumate_tile_set_state (tile, SHUMATE_STATE_DONE);
  return TRUE;
}

void
shumate_memory_cache_store_tile (ShumateMemoryCache *self,
                                 ShumateTile        *tile,
                                 const char         *source_id)
{
  GList *link;
  char *key;

  g_return_if_fail (SHUMATE_IS_MEMORY_CACHE (self));
  g_return_if_fail (SHUMATE_IS_TILE (tile));

  key = generate_queue_key (self, tile, source_id);
  link = g_hash_table_lookup (self->hash_table, key);
  if (link)
    {
      move_queue_member_to_head (self->queue, link);
      g_free (key);
    }
  else
    {
      QueueMember *member;
      GdkPaintable *paintable;
      GPtrArray *symbols;

      if (self->queue->length >= self->size_limit)
        {
          member = g_queue_pop_tail (self->queue);
          g_hash_table_remove (self->hash_table, member->key);
          delete_queue_member (member, NULL);
        }

      member = g_new0 (QueueMember, 1);
      member->key = key;
      if ((paintable = shumate_tile_get_paintable (tile)))
        member->paintable = g_object_ref (paintable);
      if ((symbols = shumate_tile_get_symbols (tile)))
        member->symbols = g_ptr_array_ref (symbols);

      g_queue_push_head (self->queue, member);
      g_hash_table_insert (self->hash_table, g_strdup (key), g_queue_peek_head_link (self->queue));
    }
}
