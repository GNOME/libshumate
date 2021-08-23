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

G_BEGIN_DECLS


/**
 * SHUMATE_STYLE_ERROR:
 *
 * Error domain for errors that may occur when parsing a map style. Errors in
 * this domain will be from the [enum@StyleError] enum.
 */
#define SHUMATE_STYLE_ERROR shumate_style_error_quark ()
GQuark shumate_style_error_quark (void);

/**
 * ShumateStyleError:
 * @SHUMATE_STYLE_ERROR_FAILED: An unspecified error occurred during the operation.
 * @SHUMATE_STYLE_ERROR_MALFORMED_STYLE: A JSON node in the style has the wrong type (e.g. an object where there should be an array).
 * @SHUMATE_STYLE_ERROR_UNSUPPORTED_LAYER: An unsupported layer type was encountered.
 * @SHUMATE_STYLE_ERROR_INVALID_EXPRESSION: An invalid or unrecognized expression was encountered.
 *
 * Error codes in the [error@StyleError] domain.
 */
typedef enum {
  SHUMATE_STYLE_ERROR_FAILED,
  SHUMATE_STYLE_ERROR_MALFORMED_STYLE,
  SHUMATE_STYLE_ERROR_UNSUPPORTED_LAYER,
  SHUMATE_STYLE_ERROR_INVALID_EXPRESSION,
} ShumateStyleError;


#define SHUMATE_TYPE_VECTOR_STYLE (shumate_vector_style_get_type())

G_DECLARE_FINAL_TYPE (ShumateVectorStyle, shumate_vector_style, SHUMATE, VECTOR_STYLE, GObject)

ShumateVectorStyle *shumate_vector_style_create (const char *style_json, GError **error);

const char *shumate_vector_style_get_style_json (ShumateVectorStyle *self);

GdkTexture *shumate_vector_style_render (ShumateVectorStyle *self, int size);

G_END_DECLS
