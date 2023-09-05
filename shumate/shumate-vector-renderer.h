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

#include <shumate/shumate-data-source.h>
#include <shumate/shumate-map-source.h>
#include <shumate/shumate-vector-sprite-sheet.h>

G_BEGIN_DECLS


#define SHUMATE_TYPE_VECTOR_RENDERER (shumate_vector_renderer_get_type())
G_DECLARE_FINAL_TYPE (ShumateVectorRenderer, shumate_vector_renderer, SHUMATE, VECTOR_RENDERER, ShumateMapSource)


gboolean shumate_vector_renderer_is_supported (void);


ShumateVectorRenderer *shumate_vector_renderer_new (const char  *id,
                                                    const char  *style_json,
                                                    GError     **error);

gboolean shumate_vector_renderer_set_sprite_sheet_data (ShumateVectorRenderer  *self,
                                                        GdkPixbuf              *sprites_pixbuf,
                                                        const char             *sprites_json,
                                                        GError                **error);

ShumateVectorSpriteSheet *shumate_vector_renderer_get_sprite_sheet (ShumateVectorRenderer *self);
void shumate_vector_renderer_set_sprite_sheet (ShumateVectorRenderer    *self,
                                               ShumateVectorSpriteSheet *sprites);

void shumate_vector_renderer_set_data_source (ShumateVectorRenderer *self,
                                              const char            *name,
                                              ShumateDataSource     *data_source);

/**
 * SHUMATE_STYLE_ERROR:
 *
 * Error domain for errors that may occur when parsing a map style. Errors in
 * this domain will be from the [error@StyleError] enum.
 */
#define SHUMATE_STYLE_ERROR shumate_style_error_quark ()
GQuark shumate_style_error_quark (void);

/**
 * ShumateStyleError:
 * @SHUMATE_STYLE_ERROR_FAILED: An unspecified error occurred during the operation.
 * @SHUMATE_STYLE_ERROR_MALFORMED_STYLE: A JSON node in the style has the wrong type (e.g. an object where there should be an array).
 * @SHUMATE_STYLE_ERROR_UNSUPPORTED_LAYER: An unsupported layer type was encountered.
 * @SHUMATE_STYLE_ERROR_INVALID_EXPRESSION: An invalid or unrecognized expression was encountered.
 * @SHUMATE_STYLE_ERROR_SUPPORT_OMITTED: Libshumate was compiled without vector tile support.
 * @SHUMATE_STYLE_ERROR_UNSUPPORTED: An unsupported style spec feature was encountered.
 *
 * Error codes that occurs while parsing the style in [class@VectorRenderer].
 */
typedef enum {
  SHUMATE_STYLE_ERROR_FAILED,
  SHUMATE_STYLE_ERROR_MALFORMED_STYLE,
  SHUMATE_STYLE_ERROR_UNSUPPORTED_LAYER,
  SHUMATE_STYLE_ERROR_INVALID_EXPRESSION,
  SHUMATE_STYLE_ERROR_SUPPORT_OMITTED,
  SHUMATE_STYLE_ERROR_UNSUPPORTED,
} ShumateStyleError;

G_END_DECLS
