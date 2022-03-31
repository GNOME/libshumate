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
shumate_vector_symbol_info_free (ShumateVectorSymbolInfo *self)
{
  g_assert (self);
  g_assert_cmpint (self->ref_count, ==, 0);

  g_clear_pointer (&self->text, g_free);
  g_clear_pointer (&self->text_font, g_free);
  shumate_vector_line_string_clear (&self->line);

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


ShumateVectorSymbolInfo *
shumate_vector_symbol_info_new (const char    *text,
                                const GdkRGBA *text_color,
                                double         text_size,
                                const char    *text_font,
                                gboolean       line_placement,
                                double         x,
                                double         y)
{
  ShumateVectorSymbolInfo *self;

  self = g_new0 (ShumateVectorSymbolInfo, 1);

  *self = (ShumateVectorSymbolInfo) {
    .ref_count = 1,
    .text = g_strdup (text),
    .text_color = *text_color,
    .text_size = text_size,
    .text_font = g_strdup (text_font),
    .line_placement = line_placement,
    .x = x,
    .y = y,
  };

  return self;
}


void
shumate_vector_symbol_info_set_line_points (ShumateVectorSymbolInfo *self,
                                            ShumateVectorLineString *linestring)
{
  ShumateVectorPoint center;
  shumate_vector_line_string_clear (&self->line);
  self->line = *linestring;
  shumate_vector_line_string_bounds (&self->line, &self->line_size, &center);
  self->x = center.x;
  self->y = center.y;
  self->line_length = shumate_vector_line_string_length (&self->line);
}
