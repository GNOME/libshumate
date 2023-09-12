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
#include "shumate-vector-expression-private.h"


G_BEGIN_DECLS

#define SHUMATE_TYPE_VECTOR_EXPRESSION_INTERPOLATE (shumate_vector_expression_interpolate_get_type())
G_DECLARE_FINAL_TYPE (ShumateVectorExpressionInterpolate, shumate_vector_expression_interpolate, SHUMATE, VECTOR_EXPRESSION_INTERPOLATE, ShumateVectorExpression)

ShumateVectorExpression *shumate_vector_expression_interpolate_from_json_obj (JsonObject *object, GError **error);
ShumateVectorExpression *shumate_vector_expression_interpolate_from_json_array (JsonArray *array,
                                                                                ShumateVectorExpressionContext *ctx,
                                                                                GError **error);

ShumateVectorExpression *shumate_vector_expression_step_from_json_array (JsonArray                       *array,
                                                                         ShumateVectorExpressionContext  *ctx,
                                                                         GError                         **error);
G_END_DECLS
