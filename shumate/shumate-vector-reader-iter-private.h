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

#pragma once

#ifdef SHUMATE_HAS_VECTOR_RENDERER

#include <glib-object.h>
#include "shumate-vector-reader-iter.h"
#include "vector/vector_tile.pb-c.h"

G_BEGIN_DECLS

VectorTile__Tile__Layer *shumate_vector_reader_iter_get_layer_struct (ShumateVectorReaderIter *self);
VectorTile__Tile__Feature *shumate_vector_reader_iter_get_feature_struct (ShumateVectorReaderIter *self);

int shumate_vector_reader_iter_get_layer_index (ShumateVectorReaderIter *self);
int shumate_vector_reader_iter_get_feature_index (ShumateVectorReaderIter *self);

G_END_DECLS

#endif