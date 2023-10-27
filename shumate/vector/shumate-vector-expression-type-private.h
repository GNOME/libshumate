/*
 * Copyright (C) 2023 James Westman <james@jwestman.net>
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

#include <glib.h>

G_BEGIN_DECLS

typedef enum {
  EXPR_INVALID,

  EXPR_LITERAL,
  EXPR_TO_BOOLEAN,
  EXPR_TO_COLOR,
  EXPR_TO_NUMBER,
  EXPR_TO_STRING,
  EXPR_TYPEOF,

  EXPR_COLLATOR,
  EXPR_FORMAT,
  EXPR_IMAGE,
  EXPR_TYPE_LITERAL,

  EXPR_ID,
  EXPR_GEOMETRY_TYPE,

  EXPR_AT,
  EXPR_GET,
  EXPR_HAS,
  EXPR_NOT_HAS,
  EXPR_IN,
  EXPR_NOT_IN,
  EXPR_INDEX_OF,
  EXPR_LENGTH,
  EXPR_SLICE,

  EXPR_NOT,
  EXPR_NE,
  EXPR_LT,
  EXPR_LE,
  EXPR_EQ,
  EXPR_GT,
  EXPR_GE,
  EXPR_ALL,
  EXPR_ANY,
  EXPR_NONE,
  EXPR_CASE,
  EXPR_COALESCE,
  EXPR_MATCH,

  EXPR_INTERPOLATE,
  EXPR_STEP,

  EXPR_LET,
  EXPR_VAR,

  EXPR_CONCAT,
  EXPR_DOWNCASE,
  EXPR_RESOLVED_LOCALE,
  EXPR_UPCASE,

  EXPR_SUB,
  EXPR_MUL,
  EXPR_DIV,
  EXPR_REM,
  EXPR_POW,
  EXPR_ADD,
  EXPR_ABS,
  EXPR_ACOS,
  EXPR_ASIN,
  EXPR_ATAN,
  EXPR_CEIL,
  EXPR_COS,
  EXPR_E,
  EXPR_FLOOR,
  EXPR_LN,
  EXPR_LN2,
  EXPR_LOG10,
  EXPR_LOG2,
  EXPR_MAX,
  EXPR_MIN,
  EXPR_PI,
  EXPR_ROUND,
  EXPR_SIN,
  EXPR_SQRT,
  EXPR_TAN,

  EXPR_ZOOM,

  EXPR_FAST_GET,
  EXPR_FAST_HAS,
  EXPR_FAST_NOT_HAS,
  EXPR_FAST_IN,
  EXPR_FAST_NOT_IN,
  EXPR_FAST_EQ,
  EXPR_FAST_NE,
  EXPR_FAST_GEOMETRY_TYPE,
  EXPR_FAST_NOT_GEOMETRY_TYPE,
} ExpressionType;

typedef struct _ExprInfo
{
  /* Keep the struct definition in shumate-vector-expression-type.gperf
     in sync! */

  const char *name;
  ExpressionType type;

  int expect_exprs;
  int expect_ge_exprs;
  int expect_le_exprs;

  gboolean expect_exprs_set : 1;
  gboolean expect_ge_exprs_set : 1;
  gboolean expect_le_exprs_set : 1;
  gboolean lookup_first_arg : 1;
} ExprInfo;

G_END_DECLS