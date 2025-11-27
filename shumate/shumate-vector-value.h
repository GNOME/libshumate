/*
 * Copyright (C) 2025 James Westman <james@jwestman.net>
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

G_BEGIN_DECLS


/**
 * ShumateVectorValueType:
 * @SHUMATE_VECTOR_VALUE_TYPE_NULL: Null value
 * @SHUMATE_VECTOR_VALUE_TYPE_NUMBER: Number value
 * @SHUMATE_VECTOR_VALUE_TYPE_BOOLEAN: Boolean value
 * @SHUMATE_VECTOR_VALUE_TYPE_STRING: String value
 * @SHUMATE_VECTOR_VALUE_TYPE_COLOR: Color value
 * @SHUMATE_VECTOR_VALUE_TYPE_ARRAY: Array value
 * @SHUMATE_VECTOR_VALUE_TYPE_RESOLVED_IMAGE: Resolved image value
 * @SHUMATE_VECTOR_VALUE_TYPE_FORMATTED_STRING: Formatted string value
 * @SHUMATE_VECTOR_VALUE_TYPE_COLLATOR: Collator value
 *
 * Type of a [struct@VectorValue].
 */
typedef enum {
  SHUMATE_VECTOR_VALUE_TYPE_NULL,
  SHUMATE_VECTOR_VALUE_TYPE_NUMBER,
  SHUMATE_VECTOR_VALUE_TYPE_BOOLEAN,
  SHUMATE_VECTOR_VALUE_TYPE_STRING,
  SHUMATE_VECTOR_VALUE_TYPE_COLOR,
  SHUMATE_VECTOR_VALUE_TYPE_ARRAY,
  SHUMATE_VECTOR_VALUE_TYPE_RESOLVED_IMAGE,
  SHUMATE_VECTOR_VALUE_TYPE_FORMATTED_STRING,
  SHUMATE_VECTOR_VALUE_TYPE_COLLATOR,
} ShumateVectorValueType;

typedef struct _ShumateVectorValue ShumateVectorValue;

#define SHUMATE_TYPE_VECTOR_VALUE shumate_vector_value_get_type()

ShumateVectorValue *shumate_vector_value_new (void);
ShumateVectorValue *shumate_vector_value_new_from_value (GValue *value);
ShumateVectorValue *shumate_vector_value_new_string (const char *string);
ShumateVectorValue *shumate_vector_value_new_number (double number);
ShumateVectorValue *shumate_vector_value_new_boolean (gboolean boolean);
ShumateVectorValue *shumate_vector_value_new_color (GdkRGBA *color);

void shumate_vector_value_free (ShumateVectorValue *self);
G_DEFINE_AUTOPTR_CLEANUP_FUNC (ShumateVectorValue, shumate_vector_value_free)

ShumateVectorValue *shumate_vector_value_dup (ShumateVectorValue *self);

ShumateVectorValueType shumate_vector_value_get_value_type (ShumateVectorValue *self);

gboolean shumate_vector_value_is_null (ShumateVectorValue *self);
void shumate_vector_value_unset (ShumateVectorValue *self);

void shumate_vector_value_set_number (ShumateVectorValue *self, double number);
gboolean shumate_vector_value_get_number (ShumateVectorValue *self, double *number);

void shumate_vector_value_set_string (ShumateVectorValue *self, const char *string);
gboolean shumate_vector_value_get_string (ShumateVectorValue *self, const char **string);

void shumate_vector_value_set_boolean (ShumateVectorValue *self, gboolean boolean);
gboolean shumate_vector_value_get_boolean (ShumateVectorValue *self, gboolean *boolean);

void shumate_vector_value_set_color (ShumateVectorValue *self, GdkRGBA *color);
gboolean shumate_vector_value_get_color (ShumateVectorValue *self, GdkRGBA *color);

void shumate_vector_value_start_array (ShumateVectorValue *self);
void shumate_vector_value_array_append (ShumateVectorValue *self, ShumateVectorValue *element);

gboolean shumate_vector_value_equal (ShumateVectorValue *a, ShumateVectorValue *b);
gint shumate_vector_value_hash (ShumateVectorValue *self);

G_END_DECLS
