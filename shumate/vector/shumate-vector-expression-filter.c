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


#include "shumate-vector-renderer.h"
#include "shumate-vector-expression-filter-private.h"
#include "shumate-vector-expression-literal-private.h"

typedef enum {
  EXPR_NOT,
  EXPR_NONE,
  EXPR_ANY,
  EXPR_ALL,
  EXPR_HAS,
  EXPR_NOT_HAS,
  EXPR_GET,
  EXPR_IN,
  EXPR_NOT_IN,
  EXPR_EQ,
  EXPR_NE,
  EXPR_GT,
  EXPR_LT,
  EXPR_GE,
  EXPR_LE,
  EXPR_CASE,
  EXPR_ADD,
  EXPR_SUB,
  EXPR_MUL,
  EXPR_DIV,
  EXPR_COALESCE,
} ExpressionType;

struct _ShumateVectorExpressionFilter
{
  ShumateVectorExpression parent_instance;

  ExpressionType type;
  GPtrArray *expressions;
};

G_DEFINE_TYPE (ShumateVectorExpressionFilter, shumate_vector_expression_filter, SHUMATE_TYPE_VECTOR_EXPRESSION)


ShumateVectorExpression *
shumate_vector_expression_filter_from_json_array (JsonArray                       *array,
                                                  ShumateVectorExpressionContext  *ctx,
                                                  GError                         **error)
{
  g_autoptr(ShumateVectorExpressionFilter) self = NULL;
  JsonNode *op_node;
  const char *op;
  gboolean lookup_first_arg = TRUE;
  int expect_exprs = -1;
  int expect_ge_exprs = -1;

  if (json_array_get_length (array) == 0)
    {
      g_set_error (error,
                   SHUMATE_STYLE_ERROR,
                   SHUMATE_STYLE_ERROR_INVALID_EXPRESSION,
                   "Expected first element of filter array to be a string");
      return NULL;
    }

  op_node = json_array_get_element (array, 0);
  if (!JSON_NODE_HOLDS_VALUE (op_node) || json_node_get_value_type (op_node) != G_TYPE_STRING)
    {
      g_set_error (error,
                   SHUMATE_STYLE_ERROR,
                   SHUMATE_STYLE_ERROR_INVALID_EXPRESSION,
                   "Expected first element of filter array to be a string");
      return NULL;
    }

  op = json_node_get_string (op_node);

  if (g_strcmp0 ("literal", op) == 0)
    {
      g_auto(ShumateVectorValue) value = SHUMATE_VECTOR_VALUE_INIT;
      JsonNode *arg;

      if (json_array_get_length (array) != 2)
        {
          g_set_error (error,
                      SHUMATE_STYLE_ERROR,
                      SHUMATE_STYLE_ERROR_INVALID_EXPRESSION,
                      "Operator `literal` expected exactly 1 argument, got %d",
                      json_array_get_length (array) - 1);
          return NULL;
        }

      arg = json_array_get_element (array, 1);

      if (!shumate_vector_value_set_from_json_literal (&value, arg, error))
        return NULL;

      return shumate_vector_expression_literal_new (&value);
    }
  else if (g_strcmp0 ("let", op) == 0)
    {
      g_auto(ShumateVectorExpressionContext) child_ctx = {0};
      ShumateVectorExpression *child_expr = NULL;

      if (json_array_get_length (array) % 2 != 0)
        {
          g_set_error (error,
                      SHUMATE_STYLE_ERROR,
                      SHUMATE_STYLE_ERROR_INVALID_EXPRESSION,
                      "Operator `let` expected an odd number of arguments, got %d",
                      json_array_get_length (array) - 1);
          return NULL;
        }

      child_ctx.parent = ctx;
      child_ctx.variables = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, g_object_unref);

      for (int i = 1, n = json_array_get_length (array); i < n - 1; i += 2)
        {
          JsonNode *name_node;
          ShumateVectorExpression *expr = NULL;

          name_node = json_array_get_element (array, i);
          if (!JSON_NODE_HOLDS_VALUE (name_node) || json_node_get_value_type (name_node) != G_TYPE_STRING)
            {
              g_set_error (error,
                           SHUMATE_STYLE_ERROR,
                           SHUMATE_STYLE_ERROR_INVALID_EXPRESSION,
                           "Expected variable name to be a string (at index %d)",
                           i);
              return NULL;
            }

          expr = shumate_vector_expression_from_json (
            json_array_get_element (array, i + 1),
            &child_ctx,
            error
          );
          if (expr == NULL)
            return NULL;

          g_hash_table_insert (child_ctx.variables,
                               (char *)json_node_get_string (name_node),
                               expr);
        }

      child_expr = shumate_vector_expression_from_json (
        json_array_get_element (array, json_array_get_length (array) - 1),
        &child_ctx,
        error
      );
      return child_expr;
    }
  else if (g_strcmp0 ("var", op) == 0)
    {
      JsonNode *arg_node;
      const char *variable = NULL;

      if (json_array_get_length (array) != 2)
        {
          g_set_error (error,
                       SHUMATE_STYLE_ERROR,
                       SHUMATE_STYLE_ERROR_INVALID_EXPRESSION,
                       "Operator `var` expected exactly one argument, got %d",
                       json_array_get_length (array) - 1);
          return NULL;
        }

      arg_node = json_array_get_element (array, 1);
      if (!JSON_NODE_HOLDS_VALUE (arg_node) || json_node_get_value_type (arg_node) != G_TYPE_STRING)
        {
          g_set_error (error,
                       SHUMATE_STYLE_ERROR,
                       SHUMATE_STYLE_ERROR_INVALID_EXPRESSION,
                       "Operator `var` expected a string literal");
          return NULL;
        }

      variable = json_node_get_string (arg_node);

      for (ShumateVectorExpressionContext *c = ctx; c != NULL; c = c->parent)
        {
          ShumateVectorExpression *expr = g_hash_table_lookup (c->variables, variable);
          if (expr != NULL)
            return g_object_ref (expr);
        }

      g_set_error (error,
                   SHUMATE_STYLE_ERROR,
                   SHUMATE_STYLE_ERROR_INVALID_EXPRESSION,
                   "Variable `%s` not found",
                   variable);
      return NULL;
    }

  self = g_object_new (SHUMATE_TYPE_VECTOR_EXPRESSION_FILTER, NULL);

  if (g_strcmp0 ("!", op) == 0)
    {
      self->type = EXPR_NOT;
      expect_exprs = 1;
    }
  else if (g_strcmp0 ("none", op) == 0)
    self->type = EXPR_NONE;
  else if (g_strcmp0 ("any", op) == 0)
    self->type = EXPR_ANY;
  else if (g_strcmp0 ("all", op) == 0)
    self->type = EXPR_ALL;
  else if (g_strcmp0 ("has", op) == 0)
    {
      self->type = EXPR_HAS;
      expect_exprs = 1;
      lookup_first_arg = FALSE;
    }
  else if (g_strcmp0 ("!has", op) == 0)
    {
      self->type = EXPR_NOT_HAS;
      expect_exprs = 1;
      lookup_first_arg = FALSE;
    }
  else if (g_strcmp0 ("get", op) == 0)
    {
      self->type = EXPR_GET;
      expect_exprs = 1;
      lookup_first_arg = FALSE;
    }
  else if (g_strcmp0 ("in", op) == 0)
    {
      self->type = EXPR_IN;
      expect_ge_exprs = 1;
    }
  else if (g_strcmp0 ("!in", op) == 0)
    {
      self->type = EXPR_NOT_IN;
      expect_ge_exprs = 1;
    }
  else if (g_strcmp0 ("==", op) == 0)
    {
      self->type = EXPR_EQ;
      expect_exprs = 2;
    }
  else if (g_strcmp0 ("!=", op) == 0)
    {
      self->type = EXPR_NE;
      expect_exprs = 2;
    }
  else if (g_strcmp0 (">", op) == 0)
    {
      self->type = EXPR_GT;
      expect_exprs = 2;
    }
  else if (g_strcmp0 ("<", op) == 0)
    {
      self->type = EXPR_LT;
      expect_exprs = 2;
    }
  else if (g_strcmp0 (">=", op) == 0)
    {
      self->type = EXPR_GE;
      expect_exprs = 2;
    }
  else if (g_strcmp0 ("<=", op) == 0)
    {
      self->type = EXPR_LE;
      expect_exprs = 2;
    }
  else if (g_strcmp0 ("+", op) == 0)
    {
      self->type = EXPR_ADD;
      expect_ge_exprs = 1;
    }
  else if (g_strcmp0 ("-", op) == 0)
    {
      self->type = EXPR_SUB;
      expect_ge_exprs = 1;
    }
  else if (g_strcmp0 ("*", op) == 0)
    {
      self->type = EXPR_MUL;
      expect_ge_exprs = 1;
    }
  else if (g_strcmp0 ("/", op) == 0)
    {
      self->type = EXPR_DIV;
      expect_exprs = 2;
    }
  else if (g_strcmp0 ("zoom", op) == 0)
    {
      g_auto(ShumateVectorValue) value = SHUMATE_VECTOR_VALUE_INIT;

      self->type = EXPR_GET;
      expect_exprs = 0;

      shumate_vector_value_set_string (&value, "zoom");
      g_ptr_array_add (self->expressions, shumate_vector_expression_literal_new (&value));
    }
  else if (g_strcmp0 ("geometry-type", op) == 0)
    {
      g_auto(ShumateVectorValue) value = SHUMATE_VECTOR_VALUE_INIT;

      self->type = EXPR_GET;
      expect_exprs = 0;

      shumate_vector_value_set_string (&value, "$type");
      g_ptr_array_add (self->expressions, shumate_vector_expression_literal_new (&value));
    }
  else if (g_strcmp0 ("case", op) == 0)
    self->type = EXPR_CASE;
  else if (g_strcmp0 ("coalesce", op) == 0)
    self->type = EXPR_COALESCE;
  else
    {
      g_set_error (error,
                   SHUMATE_STYLE_ERROR,
                   SHUMATE_STYLE_ERROR_INVALID_EXPRESSION,
                   "Unrecognized operator %s", op);
      return FALSE;
    }

  if (expect_exprs > -1 && json_array_get_length (array) - 1 != expect_exprs)
    {
      g_set_error (error,
                   SHUMATE_STYLE_ERROR,
                   SHUMATE_STYLE_ERROR_INVALID_EXPRESSION,
                   "Operator `%s` expected exactly %d arguments, got %d",
                   op, expect_exprs, json_array_get_length (array) - 1);
      return FALSE;
    }

  if (expect_ge_exprs > 0 && json_array_get_length (array) - 1 < expect_ge_exprs)
    {
      g_set_error (error,
                   SHUMATE_STYLE_ERROR,
                   SHUMATE_STYLE_ERROR_INVALID_EXPRESSION,
                   "Operator `%s` expected at least %d arguments, got %d",
                   op, expect_ge_exprs, json_array_get_length (array) - 1);
      return FALSE;
    }

  for (int i = 1; i < json_array_get_length (array); i ++)
    {
      JsonNode *arg = json_array_get_element (array, i);
      g_autoptr(ShumateVectorExpression) expr = NULL;

      if (i == 1
          && lookup_first_arg
          && JSON_NODE_HOLDS_VALUE (arg)
          && json_node_get_value_type (arg) == G_TYPE_STRING)
        {
          /* If the first argument of a function is a string,
           * convert it to a GET expression so we can do things like
           * ["==", "admin_level", 2] */
          g_auto(ShumateVectorValue) value = SHUMATE_VECTOR_VALUE_INIT;

          expr = g_object_new (SHUMATE_TYPE_VECTOR_EXPRESSION_FILTER, NULL);
          ((ShumateVectorExpressionFilter *)expr)->type = EXPR_GET;

          shumate_vector_value_set_string (&value, json_node_get_string (arg));
          g_ptr_array_add (((ShumateVectorExpressionFilter *)expr)->expressions, shumate_vector_expression_literal_new (&value));
        }
      else if (!(expr = shumate_vector_expression_from_json (arg, ctx, error)))
        return NULL;

      g_ptr_array_add (self->expressions, g_steal_pointer (&expr));
    }

  return (ShumateVectorExpression *)g_steal_pointer (&self);
}


static void
shumate_vector_expression_filter_finalize (GObject *object)
{
  ShumateVectorExpressionFilter *self = (ShumateVectorExpressionFilter *)object;

  g_clear_pointer (&self->expressions, g_ptr_array_unref);

  G_OBJECT_CLASS (shumate_vector_expression_filter_parent_class)->finalize (object);
}


static gboolean
shumate_vector_expression_filter_eval (ShumateVectorExpression  *expr,
                                       ShumateVectorRenderScope *scope,
                                       ShumateVectorValue       *out)
{
  ShumateVectorExpressionFilter *self = (ShumateVectorExpressionFilter *)expr;
  g_auto(ShumateVectorValue) value = SHUMATE_VECTOR_VALUE_INIT;
  g_auto(ShumateVectorValue) value2 = SHUMATE_VECTOR_VALUE_INIT;
  gboolean inverted = FALSE;
  gboolean boolean;
  double number, number2;
  g_autofree char *string = NULL;
  ShumateVectorExpression **expressions = (ShumateVectorExpression **)self->expressions->pdata;
  guint n_expressions = self->expressions->len;

  switch (self->type)
    {
    case EXPR_NOT:
      g_assert (n_expressions == 1);

      if (!shumate_vector_expression_eval (expressions[0], scope, &value))
        return FALSE;
      if (!shumate_vector_value_get_boolean (&value, &boolean))
        return FALSE;

      shumate_vector_value_set_boolean (out, !boolean);
      return TRUE;

    case EXPR_NONE:
      inverted = TRUE;
      G_GNUC_FALLTHROUGH;
    case EXPR_ANY:
      for (int i = 0; i < n_expressions; i ++)
        {
          if (!shumate_vector_expression_eval (expressions[i], scope, &value))
            return FALSE;
          if (!shumate_vector_value_get_boolean (&value, &boolean))
            return FALSE;

          if (boolean)
            {
              shumate_vector_value_set_boolean (out, TRUE ^ inverted);
              return TRUE;
            }
        }

      shumate_vector_value_set_boolean (out, FALSE ^ inverted);
      return TRUE;

    case EXPR_ALL:
      for (int i = 0; i < n_expressions; i ++)
        {
          if (!shumate_vector_expression_eval (expressions[i], scope, &value))
            return FALSE;
          if (!shumate_vector_value_get_boolean (&value, &boolean))
            return FALSE;

          if (!boolean)
            {
              shumate_vector_value_set_boolean (out, FALSE);
              return TRUE;
            }
        }

      shumate_vector_value_set_boolean (out, TRUE);
      return TRUE;

    case EXPR_NOT_HAS:
      inverted = TRUE;
      G_GNUC_FALLTHROUGH;
    case EXPR_HAS:
      g_assert (n_expressions == 1);

      string = shumate_vector_expression_eval_string (expressions[0], scope, NULL);
      shumate_vector_render_scope_get_variable (scope, string, &value);
      shumate_vector_value_set_boolean (out, !shumate_vector_value_is_null (&value) ^ inverted);
      return TRUE;

    case EXPR_GET:
      g_assert (n_expressions == 1);

      string = shumate_vector_expression_eval_string (expressions[0], scope, NULL);
      shumate_vector_render_scope_get_variable (scope, string, &value);
      shumate_vector_value_copy (&value, out);
      return TRUE;

    case EXPR_NOT_IN:
      inverted = TRUE;
      G_GNUC_FALLTHROUGH;
    case EXPR_IN:
      g_assert (n_expressions >= 1);

      shumate_vector_expression_eval (expressions[0], scope, &value);

      for (int i = 1; i < n_expressions; i ++)
        {
          shumate_vector_expression_eval (expressions[i], scope, &value2);
          if (
              shumate_vector_value_equal (&value, &value2)
              || shumate_vector_value_array_contains (&value2, &value))
            {
              shumate_vector_value_set_boolean (out, TRUE ^ inverted);
              return TRUE;
            }
        }

      shumate_vector_value_set_boolean (out, FALSE ^ inverted);
      return TRUE;

    case EXPR_NE:
      inverted = TRUE;
      G_GNUC_FALLTHROUGH;
    case EXPR_EQ:
      g_assert (n_expressions == 2);

      if (!shumate_vector_expression_eval (expressions[0], scope, &value))
        return FALSE;
      if (!shumate_vector_expression_eval (expressions[1], scope, &value2))
        return FALSE;

      shumate_vector_value_set_boolean (out, shumate_vector_value_equal (&value, &value2) ^ inverted);
      return TRUE;

    case EXPR_LE:
      inverted = TRUE;
      G_GNUC_FALLTHROUGH;
    case EXPR_GT:
      g_assert (n_expressions == 2);

      if (!shumate_vector_expression_eval (expressions[0], scope, &value))
        return FALSE;
      if (!shumate_vector_expression_eval (expressions[1], scope, &value2))
        return FALSE;

      if (!shumate_vector_value_get_number (&value, &number))
        return FALSE;
      if (!shumate_vector_value_get_number (&value2, &number2))
        return FALSE;

      shumate_vector_value_set_boolean (out, (number > number2) ^ inverted);
      return TRUE;

    case EXPR_GE:
      inverted = TRUE;
      G_GNUC_FALLTHROUGH;
    case EXPR_LT:
      g_assert (n_expressions == 2);

      if (!shumate_vector_expression_eval (expressions[0], scope, &value))
        return FALSE;
      if (!shumate_vector_expression_eval (expressions[1], scope, &value2))
        return FALSE;

      if (!shumate_vector_value_get_number (&value, &number))
        return FALSE;
      if (!shumate_vector_value_get_number (&value2, &number2))
        return FALSE;

      shumate_vector_value_set_boolean (out, (number < number2) ^ inverted);
      return TRUE;

    case EXPR_ADD:
      number = 0;

      for (int i = 0; i < n_expressions; i ++)
        {
          if (!shumate_vector_expression_eval (expressions[i], scope, &value))
            return FALSE;
          if (!shumate_vector_value_get_number (&value, &number2))
            return FALSE;

          number += number2;
        }

      shumate_vector_value_set_number (out, number);
      return TRUE;

    case EXPR_SUB:
      g_assert (n_expressions >= 1);

      if (!shumate_vector_expression_eval (expressions[0], scope, &value))
        return FALSE;
      if (!shumate_vector_value_get_number (&value, &number))
        return FALSE;

      if (n_expressions == 1)
        shumate_vector_value_set_number (out, 0 - number);
      else
        {
          if (!shumate_vector_expression_eval (expressions[1], scope, &value2))
            return FALSE;
          if (!shumate_vector_value_get_number (&value2, &number2))
            return FALSE;

          shumate_vector_value_set_number (out, number - number2);
        }
      return TRUE;

    case EXPR_MUL:
      g_assert (n_expressions >= 1);

      if (!shumate_vector_expression_eval (expressions[0], scope, &value))
        return FALSE;
      if (!shumate_vector_value_get_number (&value, &number))
        return FALSE;

      for (int i = 1; i < n_expressions; i ++)
        {
          if (!shumate_vector_expression_eval (expressions[i], scope, &value))
            return FALSE;
          if (!shumate_vector_value_get_number (&value, &number2))
            return FALSE;

          number *= number2;
        }

      shumate_vector_value_set_number (out, number);
      return TRUE;

    case EXPR_DIV:
      if (!shumate_vector_expression_eval (expressions[0], scope, &value))
        return FALSE;
      if (!shumate_vector_expression_eval (expressions[1], scope, &value2))
        return FALSE;

      if (!shumate_vector_value_get_number (&value, &number))
        return FALSE;
      if (!shumate_vector_value_get_number (&value2, &number2))
        return FALSE;

      shumate_vector_value_set_number (out, number / number2);

      return TRUE;

    case EXPR_CASE:
      for (int i = 0; i < n_expressions; i += 2)
        {
          if (!shumate_vector_expression_eval (expressions[i], scope, &value))
            return FALSE;

          if (i + 1 == n_expressions)
            {
              /* fallback value */
              shumate_vector_value_copy (&value, out);
              return TRUE;
            }
          else
            {
              gboolean bool_result;
              shumate_vector_value_get_boolean (&value, &bool_result);
              if (bool_result)
                {
                  if (!shumate_vector_expression_eval (expressions[i + 1], scope, &value))
                    return FALSE;
                  shumate_vector_value_copy (&value, out);
                  return TRUE;
                }
            }
        }

      /* no case matched and there was no fallback */
      return FALSE;

    case EXPR_COALESCE:
      for (int i = 0; i < n_expressions; i ++)
        {
          if (!shumate_vector_expression_eval (expressions[i], scope, &value))
            continue;

          if (shumate_vector_value_is_null (&value))
            continue;

          shumate_vector_value_copy (&value, out);
          return TRUE;
        }

      /* no expression succeeded, return null */
      shumate_vector_value_unset (out);
      return TRUE;

    default:
      g_assert_not_reached ();
    }
}

static void
shumate_vector_expression_filter_class_init (ShumateVectorExpressionFilterClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ShumateVectorExpressionClass *expr_class = SHUMATE_VECTOR_EXPRESSION_CLASS (klass);

  object_class->finalize = shumate_vector_expression_filter_finalize;
  expr_class->eval = shumate_vector_expression_filter_eval;
}


static void
shumate_vector_expression_filter_init (ShumateVectorExpressionFilter *self)
{
  self->expressions = g_ptr_array_new_with_free_func (g_object_unref);
}
