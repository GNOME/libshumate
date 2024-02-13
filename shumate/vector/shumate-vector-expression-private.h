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
#include "shumate-vector-render-scope-private.h"
#include "shumate-vector-symbol-info-private.h"
#include "shumate-vector-value-private.h"
#include "shumate-vector-index-private.h"

G_BEGIN_DECLS

typedef struct _ShumateVectorExpressionContext ShumateVectorExpressionContext;
struct _ShumateVectorExpressionContext {
  ShumateVectorExpressionContext *parent;
  GHashTable *variables;
};

void shumate_vector_expression_context_clear (ShumateVectorExpressionContext *ctx);

G_DEFINE_AUTO_CLEANUP_CLEAR_FUNC (ShumateVectorExpressionContext, shumate_vector_expression_context_clear)

#define SHUMATE_TYPE_VECTOR_EXPRESSION (shumate_vector_expression_get_type())
G_DECLARE_DERIVABLE_TYPE (ShumateVectorExpression, shumate_vector_expression, SHUMATE, VECTOR_EXPRESSION, GObject)

struct _ShumateVectorExpressionClass
{
  GObjectClass parent_class;

  gboolean (*eval) (ShumateVectorExpression  *self,
                    ShumateVectorRenderScope *scope,
                    ShumateVectorValue       *out);

  ShumateVectorIndexBitset *(*eval_bitset) (ShumateVectorExpression  *self,
                                            ShumateVectorRenderScope *scope,
                                            ShumateVectorIndexBitset *mask);

  void (*collect_indexes) (ShumateVectorExpression       *self,
                           const char                    *layer_name,
                           ShumateVectorIndexDescription *index_description);
};

ShumateVectorExpression *shumate_vector_expression_from_json (JsonNode  *json,
                                                              GError   **error);
ShumateVectorExpression *shumate_vector_expression_filter_from_format (const char *format,
                                                                       GError **error);

gboolean shumate_vector_expression_eval (ShumateVectorExpression  *self,
                                         ShumateVectorRenderScope *scope,
                                         ShumateVectorValue       *out);

ShumateVectorIndexBitset *shumate_vector_expression_eval_bitset (ShumateVectorExpression  *self,
                                                                 ShumateVectorRenderScope *scope,
                                                                 ShumateVectorIndexBitset *mask);

void shumate_vector_expression_collect_indexes (ShumateVectorExpression       *self,
                                                const char                    *layer_name,
                                                ShumateVectorIndexDescription *index_description);

double shumate_vector_expression_eval_number (ShumateVectorExpression  *self,
                                              ShumateVectorRenderScope *scope,
                                              double                    default_val);

gboolean shumate_vector_expression_eval_boolean (ShumateVectorExpression  *self,
                                                 ShumateVectorRenderScope *scope,
                                                 gboolean                  default_val);


char *shumate_vector_expression_eval_string (ShumateVectorExpression  *self,
                                             ShumateVectorRenderScope *scope,
                                             const char               *default_val);


void shumate_vector_expression_eval_color (ShumateVectorExpression  *self,
                                           ShumateVectorRenderScope *scope,
                                           GdkRGBA                  *color);

ShumateVectorSprite *shumate_vector_expression_eval_image (ShumateVectorExpression  *self,
                                                           ShumateVectorRenderScope *scope);

ShumateVectorAlignment shumate_vector_expression_eval_alignment (ShumateVectorExpression  *self,
                                                                 ShumateVectorRenderScope *scope);
ShumateVectorPlacement shumate_vector_expression_eval_placement (ShumateVectorExpression  *self,
                                                                 ShumateVectorRenderScope *scope);

ShumateVectorAnchor shumate_vector_expression_eval_anchor (ShumateVectorExpression  *self,
                                                           ShumateVectorRenderScope *scope);

ShumateVectorOverlap shumate_vector_expression_eval_overlap (ShumateVectorExpression  *self,
                                                             ShumateVectorExpression  *allow_overlap,
                                                             ShumateVectorRenderScope *scope);

G_END_DECLS
