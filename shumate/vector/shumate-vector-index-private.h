/*
 * Copyright (C) 2024 James Westman <james@jwestman.net>
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

#include <glib-object.h>
#include "shumate-vector-value-private.h"
#include "../shumate-vector-reader.h"

G_BEGIN_DECLS

typedef struct _ShumateVectorIndexDescription ShumateVectorIndexDescription;
typedef struct _ShumateVectorIndex ShumateVectorIndex;

typedef struct {
  int len;
  guint32 *bits;
} ShumateVectorIndexBitset;

ShumateVectorIndexBitset *shumate_vector_index_bitset_new (int len);
ShumateVectorIndexBitset *shumate_vector_index_bitset_copy (ShumateVectorIndexBitset *bitset);
void shumate_vector_index_bitset_free (ShumateVectorIndexBitset *bitset);
void shumate_vector_index_bitset_set (ShumateVectorIndexBitset *bitset, guint bit);
gboolean shumate_vector_index_bitset_get (ShumateVectorIndexBitset *bitset, guint bit);
void shumate_vector_index_bitset_clear (ShumateVectorIndexBitset *bitset, guint bit);
void shumate_vector_index_bitset_and (ShumateVectorIndexBitset *bitset, ShumateVectorIndexBitset *other);
void shumate_vector_index_bitset_or (ShumateVectorIndexBitset *bitset, ShumateVectorIndexBitset *other);
void shumate_vector_index_bitset_not (ShumateVectorIndexBitset *bitset);
int shumate_vector_index_bitset_next (ShumateVectorIndexBitset *bitset, int start);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (ShumateVectorIndexBitset, shumate_vector_index_bitset_free)

ShumateVectorIndex *shumate_vector_index_new (void);
void shumate_vector_index_free (ShumateVectorIndex *index);
gboolean shumate_vector_index_has_layer (ShumateVectorIndex *self, int layer_idx);
void shumate_vector_index_add_bitset (ShumateVectorIndex       *self,
                                      int                       layer_idx,
                                      const char               *field_name,
                                      ShumateVectorValue       *value,
                                      ShumateVectorIndexBitset *bitset);
void shumate_vector_index_add_bitset_has (ShumateVectorIndex       *self,
                                          int                       layer_idx,
                                          const char               *field_name,
                                          ShumateVectorIndexBitset *bitset);
void shumate_vector_index_add_bitset_broad_geometry_type (ShumateVectorIndex       *self,
                                                          int                       layer_idx,
                                                          ShumateGeometryType       type,
                                                          ShumateVectorIndexBitset *bitset);
void shumate_vector_index_add_bitset_geometry_type (ShumateVectorIndex       *self,
                                                    int                       layer_idx,
                                                    ShumateGeometryType       type,
                                                    ShumateVectorIndexBitset *bitset);
ShumateVectorIndexBitset *shumate_vector_index_get_bitset (ShumateVectorIndex *self,
                                                           int                 layer_idx,
                                                           const char         *field_name,
                                                           ShumateVectorValue *value);
ShumateVectorIndexBitset *shumate_vector_index_get_bitset_has (ShumateVectorIndex *self,
                                                               int                 layer_idx,
                                                               const char         *field_name);
ShumateVectorIndexBitset *shumate_vector_index_get_bitset_broad_geometry_type (ShumateVectorIndex *self,
                                                                               int                  layer_idx,
                                                                               ShumateGeometryType  type);
ShumateVectorIndexBitset *shumate_vector_index_get_bitset_geometry_type (ShumateVectorIndex *self,
                                                                         int                  layer_idx,
                                                                         ShumateGeometryType  type);

ShumateVectorIndexDescription *shumate_vector_index_description_new (void);
void shumate_vector_index_description_free (ShumateVectorIndexDescription *description);
gboolean shumate_vector_index_description_has_layer (ShumateVectorIndexDescription *description,
                                                     const char                    *layer_name);
gboolean shumate_vector_index_description_has_field (ShumateVectorIndexDescription *description,
                                                     const char                    *layer_name,
                                                     const char                    *field_name);
gboolean shumate_vector_index_description_has_value (ShumateVectorIndexDescription *description,
                                                     const char                    *layer_name,
                                                     const char                    *field_name,
                                                     ShumateVectorValue            *value);
gboolean shumate_vector_index_description_has_field_has_index (ShumateVectorIndexDescription *description,
                                                               const char                    *layer_name,
                                                               const char                    *field_name);
gboolean shumate_vector_index_description_has_broad_geometry_type (ShumateVectorIndexDescription *description,
                                                                   const char                    *layer_name);
gboolean shumate_vector_index_description_has_geometry_type (ShumateVectorIndexDescription *description,
                                                             const char                    *layer_name);
void shumate_vector_index_description_add (ShumateVectorIndexDescription *desc,
                                           const char                    *layer,
                                           const char                    *field,
                                           ShumateVectorValue            *value);
void shumate_vector_index_description_add_has_index (ShumateVectorIndexDescription *desc,
                                                     const char                    *layer,
                                                     const char                    *field);
void shumate_vector_index_description_add_broad_geometry_type (ShumateVectorIndexDescription *desc,
                                                               const char                    *layer);
void shumate_vector_index_description_add_geometry_type (ShumateVectorIndexDescription *desc,
                                                         const char                    *layer);
G_END_DECLS
