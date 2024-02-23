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

/*
 * Map styles can be very large with many layers, often having many similar expressions. For example,
 * the GNOME Maps style has over 800 style layers for roads, and most of their filter expressions
 * are different permutations of the same few geometry and feature data expressions. Instead of calculating
 * every expression for every feature 800 times, we can build indexes for those expressions and use them
 * to quickly filter features.
 *
 * The indexes are bitsets: each feature's bit is set if it matches the expression. It is very fast to
 * combine these bitsets with bitwise AND, OR, and NOT operations.
 */

#include "shumate-vector-index-private.h"

typedef struct {
  /* Hash set of ShumateVectorValues that should have indexes */
  GHashTable *values;
  /* True if there should be an index of features that have any value for this field */
  guint8 has_index;
} ShumateVectorIndexDescriptionField;

typedef struct {
  /* Map of field name to ShumateVectorIndexDescriptionField */
  GHashTable *fields;
  /* True if there should be geometry type indexes for the broad geometry types, not distinguishing single vs. multi */
  guint8 broad_geometry_indexes;
  /* True if there should be geometry type indexes that distinguish single vs. multi types*/
  guint8 geometry_indexes;
} ShumateVectorIndexDescriptionLayer;

/* A description of the fields and values that a set of expressions needs indexes for. */
struct _ShumateVectorIndexDescription {
  /* Map of layer name to ShumateVectorIndexDescriptionLayer */
  GHashTable *layers;
};

typedef struct {
  /* Map of value to GHashTable of ShumateVectorIndexBitset */
  GHashTable *indexes;
  /* Index of features that have any value for this field */
  ShumateVectorIndexBitset *has_index;
} ShumateVectorIndexField;

typedef struct {
  /* Map of field name to ShumateVectorIndexField */
  GHashTable *fields;
  ShumateVectorIndexBitset *broad_geometry_type_indexes[3];
  ShumateVectorIndexBitset *geometry_type_indexes[6];
} ShumateVectorIndexLayer;

/* A set of indexes for a specific vector tile. */
struct _ShumateVectorIndex {
  /* Map of layer index to ShumateVectorIndexLayer */
  GHashTable *layers;
};

static inline int
n_units (int len)
{
  return (len + 31) / 32;
}

ShumateVectorIndexBitset *
shumate_vector_index_bitset_new (int len)
{
  ShumateVectorIndexBitset *bitset = g_new (ShumateVectorIndexBitset, 1);
  bitset->len = len;
  bitset->bits = g_new0 (guint32, n_units (len));
  return bitset;
}

ShumateVectorIndexBitset *
shumate_vector_index_bitset_copy (ShumateVectorIndexBitset *bitset)
{
  ShumateVectorIndexBitset *copy;

  if (bitset == NULL)
    return NULL;

  copy = g_new (ShumateVectorIndexBitset, 1);
  copy->len = bitset->len;
  copy->bits = g_memdup2 (bitset->bits, n_units (bitset->len) * sizeof (guint32));
  return copy;
}

void
shumate_vector_index_bitset_free (ShumateVectorIndexBitset *bitset)
{
  if (bitset == NULL)
    return;

  g_free (bitset->bits);
  g_free (bitset);
}

/* Sets the given bit of the bitset. */
void
shumate_vector_index_bitset_set (ShumateVectorIndexBitset *bitset, guint bit)
{
  gsize unit = bit / 32;
  guint32 mask = 1 << (bit % 32);
  bitset->bits[unit] |= mask;
}

/* Returns the value of the given bit of the bitset. */
gboolean
shumate_vector_index_bitset_get (ShumateVectorIndexBitset *bitset, guint bit)
{
  gsize unit = bit / 32;
  guint32 mask = 1 << (bit % 32);
  return (bitset->bits[unit] & mask) != 0;
}

/* Clears the given bit of the bitset. */
void
shumate_vector_index_bitset_clear (ShumateVectorIndexBitset *bitset, guint bit)
{
  gsize unit = bit / 32;
  guint32 mask = 1 << (bit % 32);
  bitset->bits[unit] &= ~mask;
}

/* Computes the bitwise AND of the two bitsets, storing the result in the first bitset. */
void
shumate_vector_index_bitset_and (ShumateVectorIndexBitset *bitset, ShumateVectorIndexBitset *other)
{
  g_assert (bitset != NULL);
  g_assert (other != NULL);
  g_assert (bitset->len == other->len);

  for (gsize i = 0; i < n_units (bitset->len); i++)
    bitset->bits[i] &= other->bits[i];
}

/* Computes the bitwise OR of the two bitsets, storing the result in the first bitset. */
void
shumate_vector_index_bitset_or (ShumateVectorIndexBitset *bitset, ShumateVectorIndexBitset *other)
{
  g_assert (bitset != NULL);
  g_assert (other != NULL);
  g_assert (bitset->len == other->len);

  for (gsize i = 0; i < n_units (bitset->len); i++)
    bitset->bits[i] |= other->bits[i];
}

/* Computes the bitwise inverse of the bitset in place. */
void
shumate_vector_index_bitset_not (ShumateVectorIndexBitset *bitset)
{
  g_assert (bitset != NULL);

  for (gsize i = 0; i < n_units (bitset->len); i++)
    bitset->bits[i] = ~bitset->bits[i];
}

/* Returns the next set bit after the given bit, or -1 if no bit is set after it. */
int
shumate_vector_index_bitset_next (ShumateVectorIndexBitset *bitset, int start)
{
  int unit = start / 32;
  start = start % 32;

  g_assert (start >= -1 && start < bitset->len);

  for (gsize i = unit; i < n_units (bitset->len); i++)
    {
      int nth = g_bit_nth_lsf (bitset->bits[i], start);
      if (nth != -1)
        {
          int result = i * 32 + nth;
          if (result < bitset->len)
            return result;
          else
            return -1;
        }
      start = -1;
    }

  return -1;
}

static ShumateVectorIndexField *
shumate_vector_index_field_new (void)
{
  ShumateVectorIndexField *field = g_new0 (ShumateVectorIndexField, 1);
  field->indexes = g_hash_table_new_full (
    (GHashFunc)shumate_vector_value_hash,
    (GEqualFunc)shumate_vector_value_equal,
    (GDestroyNotify)shumate_vector_value_free,
    (GDestroyNotify)shumate_vector_index_bitset_free
  );
  return field;
}

static void
shumate_vector_index_field_free (ShumateVectorIndexField *field)
{
  g_hash_table_destroy (field->indexes);
  g_clear_pointer (&field->has_index, shumate_vector_index_bitset_free);
  g_free (field);
}

static ShumateVectorIndexLayer *
shumate_vector_index_layer_new (void)
{
  ShumateVectorIndexLayer *layer = g_new0 (ShumateVectorIndexLayer, 1);
  layer->fields = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, (GDestroyNotify) shumate_vector_index_field_free);
  return layer;
}

static void
shumate_vector_index_layer_free (ShumateVectorIndexLayer *layer)
{
  g_hash_table_destroy (layer->fields);
  for (int i = 0; i < 3; i++)
    g_clear_pointer (&layer->broad_geometry_type_indexes[i], shumate_vector_index_bitset_free);
  for (int i = 0; i < 6; i++)
    g_clear_pointer (&layer->geometry_type_indexes[i], shumate_vector_index_bitset_free);
  g_free (layer);
}

ShumateVectorIndex *
shumate_vector_index_new (void)
{
  ShumateVectorIndex *index = g_new0 (ShumateVectorIndex, 1);
  index->layers = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, (GDestroyNotify) shumate_vector_index_layer_free);
  return index;
}

void
shumate_vector_index_free (ShumateVectorIndex *index)
{
  g_hash_table_destroy (index->layers);
  g_free (index);
}

gboolean
shumate_vector_index_has_layer (ShumateVectorIndex *self,
                                int                 layer_idx)
{
  return g_hash_table_contains (self->layers, GINT_TO_POINTER (layer_idx));
}

static ShumateVectorIndexLayer *
get_or_create_layer (ShumateVectorIndex *self,
                     int                 layer_idx)
{
  ShumateVectorIndexLayer *layer;

  layer = g_hash_table_lookup (self->layers, GINT_TO_POINTER (layer_idx));
  if (layer == NULL)
    {
      layer = shumate_vector_index_layer_new ();
      g_hash_table_insert (self->layers, GINT_TO_POINTER (layer_idx), layer);
    }

  return layer;
}

static ShumateVectorIndexField *
get_or_create_field (ShumateVectorIndex *self,
                     int                 layer_idx,
                     const char         *field_name)
{
  ShumateVectorIndexLayer *layer = get_or_create_layer (self, layer_idx);
  ShumateVectorIndexField *field;

  field = g_hash_table_lookup (layer->fields, field_name);
  if (field == NULL)
    {
      field = shumate_vector_index_field_new ();
      g_hash_table_insert (layer->fields, g_strdup (field_name), field);
    }

  return field;
}

void
shumate_vector_index_add_bitset (ShumateVectorIndex       *self,
                                 int                       layer_idx,
                                 const char               *field_name,
                                 ShumateVectorValue       *value,
                                 ShumateVectorIndexBitset *bitset)
{
  ShumateVectorIndexBitset *existing;
  ShumateVectorIndexField *field = get_or_create_field (self, layer_idx, field_name);

  existing = g_hash_table_lookup (field->indexes, value);
  if (existing != NULL)
    {
      shumate_vector_index_bitset_or (existing, bitset);
      shumate_vector_index_bitset_free (bitset);
    }
  else
    {
      ShumateVectorValue *value_copy = g_new0 (ShumateVectorValue, 1);
      shumate_vector_value_copy (value, value_copy);
      g_hash_table_insert (field->indexes, value_copy, bitset);
    }
}

void
shumate_vector_index_add_bitset_has (ShumateVectorIndex       *self,
                                     int                       layer_idx,
                                     const char               *field_name,
                                     ShumateVectorIndexBitset *bitset)
{
  ShumateVectorIndexField *field = get_or_create_field (self, layer_idx, field_name);

  if (field->has_index != NULL)
    {
      shumate_vector_index_bitset_or (field->has_index, bitset);
      shumate_vector_index_bitset_free (bitset);
    }
  else
    field->has_index = bitset;
}

void
shumate_vector_index_add_bitset_broad_geometry_type (ShumateVectorIndex       *self,
                                                     int                       layer_idx,
                                                     ShumateGeometryType       type,
                                                     ShumateVectorIndexBitset *bitset)
{
  ShumateVectorIndexLayer *layer = get_or_create_layer (self, layer_idx);
  ShumateVectorIndexBitset *existing;
  int i;

  switch (type)
    {
    case SHUMATE_GEOMETRY_TYPE_POINT:
      i = 0;
      break;
    case SHUMATE_GEOMETRY_TYPE_LINESTRING:
      i = 1;
      break;
    case SHUMATE_GEOMETRY_TYPE_POLYGON:
      i = 2;
      break;
    default:
      g_assert_not_reached ();
    }

  existing = layer->broad_geometry_type_indexes[i];
  if (existing != NULL)
    {
      shumate_vector_index_bitset_or (existing, bitset);
      shumate_vector_index_bitset_free (bitset);
    }
  else
    layer->broad_geometry_type_indexes[i] = bitset;
}

void
shumate_vector_index_add_bitset_geometry_type (ShumateVectorIndex       *self,
                                               int                       layer_idx,
                                               ShumateGeometryType       type,
                                               ShumateVectorIndexBitset *bitset)
{
  ShumateVectorIndexLayer *layer = get_or_create_layer (self, layer_idx);
  ShumateVectorIndexBitset *existing;

  existing = layer->geometry_type_indexes[type - 1];
  if (existing != NULL)
    {
      shumate_vector_index_bitset_or (existing, bitset);
      shumate_vector_index_bitset_free (bitset);
    }
  else
    layer->geometry_type_indexes[type - 1] = bitset;
}

ShumateVectorIndexBitset *
shumate_vector_index_get_bitset (ShumateVectorIndex *self,
                                 int                 layer_idx,
                                 const char         *field_name,
                                 ShumateVectorValue *value)
{
  ShumateVectorIndexLayer *layer;
  ShumateVectorIndexField *field;

  if (self == NULL)
    return NULL;

  layer = g_hash_table_lookup (self->layers, GINT_TO_POINTER (layer_idx));
  if (layer == NULL)
    return NULL;

  field = g_hash_table_lookup (layer->fields, field_name);
  if (field == NULL)
    return NULL;

  return g_hash_table_lookup (field->indexes, value);
}

ShumateVectorIndexBitset *
shumate_vector_index_get_bitset_has (ShumateVectorIndex *self,
                                     int                 layer_idx,
                                     const char         *field_name)
{
  ShumateVectorIndexLayer *layer;
  ShumateVectorIndexField *field;

  if (self == NULL)
    return NULL;

  layer = g_hash_table_lookup (self->layers, GINT_TO_POINTER (layer_idx));
  if (layer == NULL)
    return NULL;

  field = g_hash_table_lookup (layer->fields, field_name);
  if (field == NULL)
    return NULL;

  return field->has_index;
}

ShumateVectorIndexBitset *
shumate_vector_index_get_bitset_broad_geometry_type (ShumateVectorIndex *self,
                                                     int                  layer_idx,
                                                     ShumateGeometryType  type)
{
  ShumateVectorIndexLayer *layer;

  if (self == NULL)
    return NULL;

  layer = g_hash_table_lookup (self->layers, GINT_TO_POINTER (layer_idx));
  if (layer == NULL)
    return NULL;

  switch (type)
    {
    case SHUMATE_GEOMETRY_TYPE_POINT:
      return layer->broad_geometry_type_indexes[0];
    case SHUMATE_GEOMETRY_TYPE_LINESTRING:
      return layer->broad_geometry_type_indexes[1];
    case SHUMATE_GEOMETRY_TYPE_POLYGON:
      return layer->broad_geometry_type_indexes[2];
    default:
      return NULL;
    }
}

ShumateVectorIndexBitset *
shumate_vector_index_get_bitset_geometry_type (ShumateVectorIndex *self,
                                               int                  layer_idx,
                                               ShumateGeometryType  type)
{
  ShumateVectorIndexLayer *layer;

  g_assert (type >= SHUMATE_GEOMETRY_TYPE_POINT && type <= SHUMATE_GEOMETRY_TYPE_MULTIPOLYGON);

  if (self == NULL)
    return NULL;

  layer = g_hash_table_lookup (self->layers, GINT_TO_POINTER (layer_idx));
  if (layer == NULL)
    return NULL;

  return layer->geometry_type_indexes[type - 1];
}

static ShumateVectorIndexDescriptionField *
shumate_vector_index_description_field_new (void)
{
  ShumateVectorIndexDescriptionField *field = g_new0 (ShumateVectorIndexDescriptionField, 1);
  field->values = g_hash_table_new_full ((GHashFunc)shumate_vector_value_hash, (GEqualFunc)shumate_vector_value_equal, (GDestroyNotify)shumate_vector_value_free, NULL);
  return field;
}

static void
shumate_vector_index_description_field_free (ShumateVectorIndexDescriptionField *field)
{
  g_hash_table_destroy (field->values);
  g_free (field);
}

static ShumateVectorIndexDescriptionLayer *
shumate_vector_index_description_layer_new (void)
{
  ShumateVectorIndexDescriptionLayer *layer = g_new0 (ShumateVectorIndexDescriptionLayer, 1);
  layer->fields = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, (GDestroyNotify) shumate_vector_index_description_field_free);
  return layer;
}

static void
shumate_vector_index_description_layer_free (ShumateVectorIndexDescriptionLayer *layer)
{
  g_hash_table_destroy (layer->fields);
  g_free (layer);
}

/* Creates a new index description, which describes the indexes that a set of expressions will use. */
ShumateVectorIndexDescription *
shumate_vector_index_description_new (void)
{
  ShumateVectorIndexDescription *desc = g_new0 (ShumateVectorIndexDescription, 1);
  desc->layers = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, (GDestroyNotify) shumate_vector_index_description_layer_free);
  return desc;
}

void
shumate_vector_index_description_free (ShumateVectorIndexDescription *description)
{
  g_hash_table_destroy (description->layers);
  g_free (description);
}

/* Returns whether the index description has any indexes for the given layer. */
gboolean
shumate_vector_index_description_has_layer (ShumateVectorIndexDescription *description,
                                            const char                    *layer_name)
{
  return g_hash_table_contains (description->layers, layer_name);
}

/* Returns whether the index description has any indexes for the given field. */
gboolean
shumate_vector_index_description_has_field (ShumateVectorIndexDescription *description,
                                            const char                    *layer_name,
                                            const char                    *field_name)
{
  ShumateVectorIndexDescriptionLayer *layer = g_hash_table_lookup (description->layers, layer_name);
  if (layer == NULL)
    return FALSE;

  return g_hash_table_contains (layer->fields, field_name);
}

/* Returns whether the index description has an index for the given key=value pair. */
gboolean
shumate_vector_index_description_has_value (ShumateVectorIndexDescription *description,
                                            const char                    *layer_name,
                                            const char                    *field_name,
                                            ShumateVectorValue            *value)
{
  ShumateVectorIndexDescriptionLayer *layer;
  ShumateVectorIndexDescriptionField *field;

  layer = g_hash_table_lookup (description->layers, layer_name);
  if (layer == NULL)
    return FALSE;

  field = g_hash_table_lookup (layer->fields, field_name);
  if (field == NULL)
    return FALSE;

  return g_hash_table_contains (field->values, value);
}

/* Returns whether the index description has an index for features with any value for the
   given field. */
gboolean
shumate_vector_index_description_has_field_has_index (ShumateVectorIndexDescription *description,
                                                      const char                    *layer_name,
                                                      const char                    *field_name)
{
  ShumateVectorIndexDescriptionLayer *layer;
  ShumateVectorIndexDescriptionField *field;

  layer = g_hash_table_lookup (description->layers, layer_name);
  if (layer == NULL)
    return FALSE;

  field = g_hash_table_lookup (layer->fields, field_name);
  if (field == NULL)
    return FALSE;

  return field->has_index;
}

/* Returns whether the index description has broad geometry type indexes. */
gboolean
shumate_vector_index_description_has_broad_geometry_type (ShumateVectorIndexDescription *description,
                                                          const char                    *layer_name)
{
  ShumateVectorIndexDescriptionLayer *layer;

  layer = g_hash_table_lookup (description->layers, layer_name);
  if (layer == NULL)
    return FALSE;

  return layer->broad_geometry_indexes;
}

/* Returns whether the index description has geometry type indexes that distinguish single vs. multi geometries. */
gboolean
shumate_vector_index_description_has_geometry_type (ShumateVectorIndexDescription *description,
                                                    const char                    *layer_name)
{
  ShumateVectorIndexDescriptionLayer *layer;

  layer = g_hash_table_lookup (description->layers, layer_name);
  if (layer == NULL)
    return FALSE;

  return layer->geometry_indexes;
}

static ShumateVectorIndexDescriptionLayer *
get_or_create_desc_layer (ShumateVectorIndexDescription *desc,
                          const char                    *layer)
{
  ShumateVectorIndexDescriptionLayer *layer_desc;

  layer_desc = g_hash_table_lookup (desc->layers, layer);
  if (layer_desc == NULL)
    {
      layer_desc = shumate_vector_index_description_layer_new ();
      g_hash_table_insert (desc->layers, g_strdup (layer), layer_desc);
    }

  return layer_desc;
}

static ShumateVectorIndexDescriptionField *
get_or_create_desc_field (ShumateVectorIndexDescription *desc,
                          const char                    *layer,
                          const char                    *field)
{
  ShumateVectorIndexDescriptionLayer *layer_desc = get_or_create_desc_layer (desc, layer);
  ShumateVectorIndexDescriptionField *field_desc;

  field_desc = g_hash_table_lookup (layer_desc->fields, field);
  if (field_desc == NULL)
    {
      field_desc = shumate_vector_index_description_field_new ();
      g_hash_table_insert (layer_desc->fields, g_strdup (field), field_desc);
    }

  return field_desc;
}

/* Add an index for the given key=value pair. */
void
shumate_vector_index_description_add (ShumateVectorIndexDescription *desc,
                                      const char                    *layer,
                                      const char                    *field,
                                      ShumateVectorValue            *value)
{
  ShumateVectorIndexDescriptionField *field_desc = get_or_create_desc_field (desc, layer, field);
  ShumateVectorValue *value_copy;

  value_copy = g_new0 (ShumateVectorValue, 1);
  shumate_vector_value_copy (value, value_copy);
  g_hash_table_insert (field_desc->values, value_copy, value_copy);
}

/* Add an index for features that have any value for the given field. */
void
shumate_vector_index_description_add_has_index (ShumateVectorIndexDescription *desc,
                                                const char                    *layer,
                                                const char                    *field)
{
  ShumateVectorIndexDescriptionField *field_desc = get_or_create_desc_field (desc, layer, field);
  field_desc->has_index = TRUE;
}

/* Add geometry indexes to the index description. "Broad" indexes only index point/line/polygon,
   not whether the geometry is a single or multi geometry. This is very common and is faster
   to calculate. */
void
shumate_vector_index_description_add_broad_geometry_type (ShumateVectorIndexDescription *desc,
                                                          const char                    *layer)
{
  ShumateVectorIndexDescriptionLayer *layer_desc = get_or_create_desc_layer (desc, layer);
  layer_desc->broad_geometry_indexes = TRUE;
}

/* Add geometry indexes to the index description. These indexes will distinguish single vs. multi geometries. */
void
shumate_vector_index_description_add_geometry_type (ShumateVectorIndexDescription *desc,
                                                    const char                    *layer)
{
  ShumateVectorIndexDescriptionLayer *layer_desc = get_or_create_desc_layer (desc, layer);
  layer_desc->geometry_indexes = TRUE;
}
