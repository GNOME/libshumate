/*
 * Copyright (C) 2021 James Westman <james@jwestman.net>
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


#include "shumate-vector-symbol-info-private.h"


G_DEFINE_BOXED_TYPE (ShumateVectorSymbolInfo, shumate_vector_symbol_info, shumate_vector_symbol_info_ref, shumate_vector_symbol_info_unref)


static void
shumate_vector_symbol_details_free (ShumateVectorSymbolDetails *details)
{
  g_assert (details != NULL);
  g_assert_cmpint (details->ref_count, ==, 0);

  g_clear_pointer (&details->layer, g_free);
  g_clear_pointer (&details->feature_id, g_free);
  g_clear_pointer (&details->source_layer, g_free);

  g_clear_object (&details->icon_image);

  g_clear_pointer (&details->formatted_text, g_ptr_array_unref);
  g_clear_pointer (&details->text_font, g_free);

  g_clear_pointer (&details->cursor, g_free);

  g_clear_pointer (&details->tags, g_hash_table_unref);

  g_free (details);
}

ShumateVectorSymbolDetails *
shumate_vector_symbol_details_ref (ShumateVectorSymbolDetails *details)
{
  g_return_val_if_fail (details, NULL);
  g_return_val_if_fail (details->ref_count, NULL);

  g_atomic_int_inc (&details->ref_count);

  return details;
}

void
shumate_vector_symbol_details_unref (ShumateVectorSymbolDetails *details)
{
  g_return_if_fail (details);
  g_return_if_fail (details->ref_count);

  if (g_atomic_int_dec_and_test (&details->ref_count))
    shumate_vector_symbol_details_free (details);
}

static void
shumate_vector_symbol_info_free (ShumateVectorSymbolInfo *self)
{
  g_assert (self != NULL);
  g_assert_cmpint (self->ref_count, ==, 0);

  g_clear_pointer (&self->details, shumate_vector_symbol_details_unref);

  g_clear_pointer (&self->line, shumate_vector_line_string_free);

  g_free (self);
}

ShumateVectorSymbolInfo *
shumate_vector_symbol_info_ref (ShumateVectorSymbolInfo *self)
{
  g_return_val_if_fail (self, NULL);
  g_return_val_if_fail (self->ref_count, NULL);

  g_atomic_int_inc (&self->ref_count);

  return self;
}

void
shumate_vector_symbol_info_unref (ShumateVectorSymbolInfo *self)
{
  g_return_if_fail (self);
  g_return_if_fail (self->ref_count);

  if (g_atomic_int_dec_and_test (&self->ref_count))
    shumate_vector_symbol_info_free (self);
}


void
shumate_vector_symbol_info_set_line_points (ShumateVectorSymbolInfo *self,
                                            ShumateVectorLineString *linestring,
                                            float                    position)
{
  ShumateVectorPoint center;
  g_clear_pointer (&self->line, shumate_vector_line_string_free);
  self->line = linestring;

  shumate_vector_line_string_bounds (self->line, &self->line_size, &center);
  self->x = center.x;
  self->y = center.y;
  self->line_length = shumate_vector_line_string_length (self->line);
  self->line_position = position;
}

int
shumate_vector_symbol_info_compare (ShumateVectorSymbolInfo *a,
                                    ShumateVectorSymbolInfo *b)
{
  if (a->details->layer_idx < b->details->layer_idx)
    return -1;
  else if (a->details->layer_idx > b->details->layer_idx)
    return 1;
  else if (a->details->symbol_sort_key < b->details->symbol_sort_key)
    return -1;
  else if (a->details->symbol_sort_key > b->details->symbol_sort_key)
    return 1;
  else
    return 0;
}
