/*
 * Copyright (C) 2023 James Westman <james@jwestman.net>
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

#include "shumate-vector-reader-private.h"
#ifdef SHUMATE_HAS_VECTOR_RENDERER
#include "vector/vector_tile.pb-c.h"
#endif

/**
 * ShumateVectorReader:
 *
 * Provides low-level access to the contents of a vector tile.
 *
 * To create a new [class@VectorReader], use [ctor@VectorReader.new] and pass
 * the bytes of a vector tile, which you might get from a [class@DataSource].
 * Then, use [method@VectorReader.iterate] to get a [class@VectorReaderIter]
 * and iterate over the features in the tile. You can create multiple
 * [class@VectorReaderIter]s from the same [class@VectorReader].
 *
 * Since: 1.2
 */

G_DEFINE_TYPE (ShumateVectorReader, shumate_vector_reader, G_TYPE_OBJECT)

static void
shumate_vector_reader_finalize (GObject *object)
{
  ShumateVectorReader *self = SHUMATE_VECTOR_READER (object);

#ifdef SHUMATE_HAS_VECTOR_RENDERER
  vector_tile__tile__free_unpacked (self->tile, NULL);
  self->tile = NULL;
#endif

  G_OBJECT_CLASS (shumate_vector_reader_parent_class)->finalize (object);
}

static void
shumate_vector_reader_class_init (ShumateVectorReaderClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = shumate_vector_reader_finalize;
}

static void
shumate_vector_reader_init (ShumateVectorReader *self)
{
}

/**
 * shumate_vector_reader_new:
 * @bytes: (transfer none): A tile in Mapbox Vector Tile format
 *
 * Creates a new [class@VectorReader] from @bytes.
 *
 * Returns: (transfer full): A new [class@VectorReader]
 *
 * Since: 1.2
 */
ShumateVectorReader *
shumate_vector_reader_new (GBytes *bytes)
{
#ifdef SHUMATE_HAS_VECTOR_RENDERER
  g_autoptr(ShumateVectorReader) self = g_object_new(SHUMATE_TYPE_VECTOR_READER, NULL);
  gconstpointer data;
  gsize len;

  data = g_bytes_get_data (bytes, &len);
  self->tile = vector_tile__tile__unpack (NULL, len, data);

  if (self->tile == NULL)
    return NULL;

  return g_steal_pointer (&self);
#else
  g_warning ("Vector tile support is not enabled");
  return NULL;
#endif
}

/**
 * shumate_vector_reader_iterate:
 * @self: A [class@VectorReader]
 *
 * Creates a new [class@VectorReaderIter] for @self.
 *
 * Returns: (transfer full): A new [class@VectorReaderIter]
 *
 * Since: 1.2
 */
ShumateVectorReaderIter *
shumate_vector_reader_iterate (ShumateVectorReader *self)
{
#ifdef SHUMATE_HAS_VECTOR_RENDERER
  return shumate_vector_reader_iter_new (self);
#else
  g_warning ("Vector tile support is not enabled");
  return NULL;
#endif
}
