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

#include "shumate-vector-expression-private.h"

G_BEGIN_DECLS

#define SHUMATE_TYPE_VECTOR_EXPRESSION_FORMAT (shumate_vector_expression_format_get_type())
G_DECLARE_FINAL_TYPE (ShumateVectorExpressionFormat, shumate_vector_expression_format, SHUMATE, VECTOR_EXPRESSION_FORMAT, ShumateVectorExpression)

ShumateVectorExpression *shumate_vector_expression_format_new (const char *format, GError **error);

G_END_DECLS
