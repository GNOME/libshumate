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

#include <gtk/gtk.h>
#include "shumate-vector-sprite.h"
#include "shumate-vector-value.h"

#ifdef SHUMATE_HAS_VECTOR_RENDERER
#include <json-glib/json-glib.h>
#endif

#define SHUMATE_VECTOR_COLOR_BLACK ((GdkRGBA) {.red=0, .green=0, .blue=0, .alpha=1})


typedef struct {
  GdkRGBA text_color;
  char *string;
  ShumateVectorSprite *sprite;
  double font_scale;
  gboolean has_text_color : 1;
  gboolean has_font_scale : 1;
} ShumateVectorFormatPart;

typedef struct {
  guint8 case_sensitive : 1;
} ShumateVectorCollator;

struct _ShumateVectorValue {
  ShumateVectorValueType type;

  union {
    double number;
    gboolean boolean;
    struct {
      char *string;
      GdkRGBA color;
      int color_state;
    };
    GPtrArray *array;
    struct {
      ShumateVectorSprite *image;
      char *image_name;
    };
    GPtrArray *formatted_string;
    ShumateVectorCollator collator;
  };
};

#define SHUMATE_VECTOR_VALUE_INIT ((ShumateVectorValue) {.type = 0})

gboolean shumate_vector_value_set_from_g_value (ShumateVectorValue *self, const GValue *value);
#ifdef SHUMATE_HAS_VECTOR_RENDERER
gboolean shumate_vector_value_set_from_json_literal (ShumateVectorValue *self, JsonNode *node, GError **error);
#endif

void shumate_vector_value_free (ShumateVectorValue *self);
void shumate_vector_value_copy (ShumateVectorValue *self, ShumateVectorValue *out);
void shumate_vector_value_steal (ShumateVectorValue *self, ShumateVectorValue *out);

G_DEFINE_AUTO_CLEANUP_CLEAR_FUNC (ShumateVectorValue, shumate_vector_value_unset)

GPtrArray *shumate_vector_value_get_array (ShumateVectorValue *self);

void shumate_vector_value_set_image (ShumateVectorValue *self, ShumateVectorSprite *image, const char *image_name);
gboolean shumate_vector_value_get_image (ShumateVectorValue *self, ShumateVectorSprite **image);

void shumate_vector_value_set_formatted (ShumateVectorValue *self, GPtrArray *format_parts);
gboolean shumate_vector_value_get_formatted (ShumateVectorValue *self, GPtrArray **format_parts);
void shumate_vector_format_part_free (ShumateVectorFormatPart *self);
G_DEFINE_AUTOPTR_CLEANUP_FUNC (ShumateVectorFormatPart, shumate_vector_format_part_free)

void shumate_vector_value_set_collator (ShumateVectorValue *self, ShumateVectorCollator *collator);
gboolean shumate_vector_value_get_collator (ShumateVectorValue *self, ShumateVectorCollator *collator);

char *shumate_vector_value_as_string (ShumateVectorValue *self);
