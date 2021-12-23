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

#pragma once

#include <json-glib/json-glib.h>
#include <gtk/gtk.h>
#include "vector_tile.pb-c.h"

#define SHUMATE_VECTOR_COLOR_BLACK ((GdkRGBA) {.red=0, .green=0, .blue=0, .alpha=1})

typedef struct {
  int type;

  union {
    double number;
    gboolean boolean;
    struct {
      char *string;
      GdkRGBA color;
      int color_state;
    };
  };
} ShumateVectorValue;

#define SHUMATE_VECTOR_VALUE_INIT ((ShumateVectorValue) {.type = 0})

gboolean shumate_vector_value_set_from_g_value (ShumateVectorValue *self, const GValue *value);
void shumate_vector_value_set_from_feature_value (ShumateVectorValue *self, VectorTile__Tile__Value *value);

void shumate_vector_value_unset (ShumateVectorValue *self);
gboolean shumate_vector_value_is_null (ShumateVectorValue *self);
void shumate_vector_value_copy (ShumateVectorValue *self, ShumateVectorValue *out);

G_DEFINE_AUTO_CLEANUP_CLEAR_FUNC (ShumateVectorValue, shumate_vector_value_unset)

void shumate_vector_value_set_number (ShumateVectorValue *self, double number);
gboolean shumate_vector_value_get_number (ShumateVectorValue *self, double *number);

void shumate_vector_value_set_string (ShumateVectorValue *self, const char *string);
gboolean shumate_vector_value_get_string (ShumateVectorValue *self, const char **string);

void shumate_vector_value_set_boolean (ShumateVectorValue *self, gboolean boolean);
gboolean shumate_vector_value_get_boolean (ShumateVectorValue *self, gboolean *boolean);

void shumate_vector_value_set_color (ShumateVectorValue *self, GdkRGBA *color);
gboolean shumate_vector_value_get_color (ShumateVectorValue *self, GdkRGBA *color);

gboolean shumate_vector_value_equal (ShumateVectorValue *a, ShumateVectorValue *b);

char *shumate_vector_value_as_string (ShumateVectorValue *self);
