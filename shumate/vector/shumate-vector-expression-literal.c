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


#include "shumate-vector-expression-literal-private.h"

struct _ShumateVectorExpressionLiteral
{
  ShumateVectorExpression parent_instance;

  ShumateVectorValue value;
};

G_DEFINE_TYPE (ShumateVectorExpressionLiteral, shumate_vector_expression_literal, SHUMATE_TYPE_VECTOR_EXPRESSION)

ShumateVectorExpression *
shumate_vector_expression_literal_new (ShumateVectorValue *value)
{
  ShumateVectorExpressionLiteral *self = g_object_new (SHUMATE_TYPE_VECTOR_EXPRESSION_LITERAL, NULL);
  shumate_vector_value_copy (value, &self->value);
  return (ShumateVectorExpression *)self;
}

static void
shumate_vector_expression_literal_finalize (GObject *object)
{
  ShumateVectorExpressionLiteral *self = (ShumateVectorExpressionLiteral *)object;

  shumate_vector_value_unset (&self->value);

  G_OBJECT_CLASS (shumate_vector_expression_literal_parent_class)->finalize (object);
}

static gboolean
shumate_vector_expression_literal_eval (ShumateVectorExpression  *expr,
                                        ShumateVectorRenderScope *scope,
                                        ShumateVectorValue       *out)
{
  ShumateVectorExpressionLiteral *self = (ShumateVectorExpressionLiteral *)expr;
  shumate_vector_value_copy (&self->value, out);
  return TRUE;
}

static void
shumate_vector_expression_literal_class_init (ShumateVectorExpressionLiteralClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ShumateVectorExpressionClass *expr_class = SHUMATE_VECTOR_EXPRESSION_CLASS (klass);

  object_class->finalize = shumate_vector_expression_literal_finalize;
  expr_class->eval = shumate_vector_expression_literal_eval;
}

static void
shumate_vector_expression_literal_init (ShumateVectorExpressionLiteral *self)
{
}
