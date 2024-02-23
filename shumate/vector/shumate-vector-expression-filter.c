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
#include "shumate-vector-expression-interpolate-private.h"
#include "shumate-vector-expression-type-private.h"
#include "../shumate-vector-reader-iter.h"

typedef enum {
  GEOM_SINGLE_POINT = 1 << 0,
  GEOM_MULTI_POINT = 1 << 1,
  GEOM_SINGLE_LINESTRING = 1 << 2,
  GEOM_MULTI_LINESTRING = 1 << 3,
  GEOM_SINGLE_POLYGON = 1 << 4,
  GEOM_MULTI_POLYGON = 1 << 5,
  GEOM_ANY_POINT = GEOM_SINGLE_POINT | GEOM_MULTI_POINT,
  GEOM_ANY_LINESTRING = GEOM_SINGLE_LINESTRING | GEOM_MULTI_LINESTRING,
  GEOM_ANY_POLYGON = GEOM_SINGLE_POLYGON | GEOM_MULTI_POLYGON,
} GeometryTypeFlags;

struct _ShumateVectorExpressionFilter
{
  ShumateVectorExpression parent_instance;

  ExpressionType type;

  GPtrArray *expressions;

  union {
    ShumateVectorValue value;
    GPtrArray *format_parts;
    char *fast_get_key;
    GHashTable *match_expressions;
    struct {
      char *key;
      GHashTable *haystack;
    } fast_in;
    struct {
      ShumateVectorValue value;
      char *key;
    } fast_eq;
    GeometryTypeFlags fast_geometry_type;
  };
};

typedef struct {
  ShumateVectorExpression *string;
  ShumateVectorExpression *font_scale;
  ShumateVectorExpression *text_color;
} FormatExpressionPart;

G_DEFINE_TYPE (ShumateVectorExpressionFilter, shumate_vector_expression_filter, SHUMATE_TYPE_VECTOR_EXPRESSION)


static void
format_expression_part_free (FormatExpressionPart *part)
{
  g_clear_object (&part->string);
  g_clear_object (&part->font_scale);
  g_clear_object (&part->text_color);
  g_free (part);
}
G_DEFINE_AUTOPTR_CLEANUP_FUNC (FormatExpressionPart, format_expression_part_free);


const ExprInfo *shumate_vector_expression_type_lookup (register const char *str, register size_t len);


static ShumateVectorExpression *optimize (ShumateVectorExpressionFilter *self);


ShumateVectorExpression *
shumate_vector_expression_filter_from_literal (ShumateVectorValue *value)
{
  g_autoptr(ShumateVectorExpressionFilter) self = NULL;

  self = g_object_new (SHUMATE_TYPE_VECTOR_EXPRESSION_FILTER, NULL);
  self->type = EXPR_LITERAL;
  shumate_vector_value_copy (value, &self->value);

  return (ShumateVectorExpression *)g_steal_pointer (&self);
}


ShumateVectorExpression *
shumate_vector_expression_filter_from_array_or_literal (JsonNode                        *node,
                                                        ShumateVectorExpressionContext  *ctx,
                                                        GError                         **error)
{
  if (node == NULL || JSON_NODE_HOLDS_NULL (node))
    return shumate_vector_expression_filter_from_literal (&SHUMATE_VECTOR_VALUE_INIT);
  else if (JSON_NODE_HOLDS_VALUE (node))
    {
      g_auto(ShumateVectorValue) value = SHUMATE_VECTOR_VALUE_INIT;
      if (!shumate_vector_value_set_from_json_literal (&value, node, error))
        return NULL;
      return shumate_vector_expression_filter_from_literal (&value);
    }
  else if (JSON_NODE_HOLDS_ARRAY (node))
    return shumate_vector_expression_filter_from_json_array (json_node_get_array (node), ctx, error);
  else
    {
      g_set_error (error,
                   SHUMATE_STYLE_ERROR,
                   SHUMATE_STYLE_ERROR_INVALID_EXPRESSION,
                   "Expected a literal or array");
      return NULL;
    }
}

ShumateVectorExpression *
shumate_vector_expression_filter_from_json_array (JsonArray                       *array,
                                                  ShumateVectorExpressionContext  *ctx,
                                                  GError                         **error)
{
  g_autoptr(ShumateVectorExpressionFilter) self = NULL;
  JsonNode *op_node;
  const char *op;
  const ExprInfo *info;

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
  info = shumate_vector_expression_type_lookup (op, strlen (op));

  if (info == NULL)
    {
      g_set_error (error,
                   SHUMATE_STYLE_ERROR,
                   SHUMATE_STYLE_ERROR_INVALID_EXPRESSION,
                   "Unrecognized operator %s", op);
      return FALSE;
    }

  /* Expressions that don't use the child expressions array */
  switch (info->type)
    {
      case EXPR_COLLATOR:
        {
          JsonNode *node;
          JsonObject *object;
          ShumateVectorExpression *child = NULL;

          if (json_array_get_length (array) != 2)
            {
              g_set_error (error,
                          SHUMATE_STYLE_ERROR,
                          SHUMATE_STYLE_ERROR_INVALID_EXPRESSION,
                          "Operator `collator` expected exactly 1 argument, got %d",
                          json_array_get_length (array) - 1);
              return NULL;
            }

          node = json_array_get_element (array, 1);
          if (!JSON_NODE_HOLDS_OBJECT (node))
            {
              g_set_error (error,
                          SHUMATE_STYLE_ERROR,
                          SHUMATE_STYLE_ERROR_INVALID_EXPRESSION,
                          "Operator `collator` expected an object");
              return NULL;
            }

          object = json_node_get_object (node);

          self = g_object_new (SHUMATE_TYPE_VECTOR_EXPRESSION_FILTER, NULL);
          self->type = EXPR_COLLATOR;
          self->expressions = g_ptr_array_new_with_free_func (g_object_unref);

          if (!(child = shumate_vector_expression_filter_from_array_or_literal (json_object_get_member (object, "case-sensitive"), ctx, error)))
            return NULL;
          g_ptr_array_add (self->expressions, child);

          return (ShumateVectorExpression *)g_steal_pointer (&self);
        }

      case EXPR_FORMAT:
        {
          self = g_object_new (SHUMATE_TYPE_VECTOR_EXPRESSION_FILTER, NULL);
          self->type = EXPR_FORMAT;
          self->format_parts = g_ptr_array_new_with_free_func ((GDestroyNotify)format_expression_part_free);

          for (int i = 1, n = json_array_get_length (array); i < n;)
            {
              JsonNode *arg_node = json_array_get_element (array, i);
              JsonNode *format_node = NULL;
              FormatExpressionPart *part = g_new0 (FormatExpressionPart, 1);

              g_ptr_array_add (self->format_parts, part);

              part->string = shumate_vector_expression_filter_from_array_or_literal (arg_node, ctx, error);
              if (part->string == NULL)
                return NULL;

              i ++;
              if (i >= n)
                break;

              format_node = json_array_get_element (array, i);
              if (JSON_NODE_HOLDS_OBJECT (format_node))
                {
                  JsonObject *format_object = json_node_get_object (format_node);

                  i ++;

                  if (!(part->text_color = shumate_vector_expression_filter_from_array_or_literal (json_object_get_member (format_object, "text-color"), ctx, error)))
                    return NULL;

                  if (!(part->font_scale = shumate_vector_expression_filter_from_array_or_literal (json_object_get_member (format_object, "font-scale"), ctx, error)))
                    return NULL;
                }
            }

          return (ShumateVectorExpression *)g_steal_pointer (&self);
        }

      case EXPR_TYPE_LITERAL:
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

          return shumate_vector_expression_filter_from_literal (&value);
        }

      case EXPR_INTERPOLATE:
        return shumate_vector_expression_interpolate_from_json_array (array, ctx, error);

      case EXPR_STEP:
        return shumate_vector_expression_step_from_json_array (array, ctx, error);

      case EXPR_MATCH:
        {
          ShumateVectorExpression *input_expr;

          self = g_object_new (SHUMATE_TYPE_VECTOR_EXPRESSION_FILTER, NULL);
          self->type = EXPR_MATCH;
          self->expressions = g_ptr_array_new_with_free_func (g_object_unref);
          self->match_expressions = g_hash_table_new_full (
            (GHashFunc)shumate_vector_value_hash,
            (GEqualFunc)shumate_vector_value_equal,
            (GDestroyNotify)shumate_vector_value_free,
            g_object_unref
          );

          if (json_array_get_length (array) < 3)
            {
              g_set_error (error,
                          SHUMATE_STYLE_ERROR,
                          SHUMATE_STYLE_ERROR_INVALID_EXPRESSION,
                          "Operator `match` expected at least 2 arguments, got %d",
                          json_array_get_length (array) - 1);
              return NULL;
            }

          input_expr = shumate_vector_expression_filter_from_array_or_literal (json_array_get_element (array, 1), ctx, error);
          if (input_expr == NULL)
            return NULL;
          g_ptr_array_add (self->expressions, input_expr);

          for (int i = 2, n = json_array_get_length (array); i + 1 < n; i += 2)
            {
              JsonNode *label_node = json_array_get_element (array, i);
              JsonNode *output_node = json_array_get_element (array, i + 1);
              g_autoptr(ShumateVectorExpression) output_expr =
                shumate_vector_expression_filter_from_array_or_literal (output_node, ctx, error);

              if (output_expr == NULL)
                return NULL;

              if (JSON_NODE_HOLDS_ARRAY (label_node))
                {
                  JsonArray *labels = json_node_get_array (label_node);
                  for (int j = 0, m = json_array_get_length (labels); j < m; j ++)
                    {
                      JsonNode *label = json_array_get_element (labels, j);
                      g_autoptr(ShumateVectorValue) label_value = g_new0 (ShumateVectorValue, 1);

                      if (!shumate_vector_value_set_from_json_literal (label_value, label, error))
                        return NULL;

                      g_hash_table_insert (self->match_expressions, g_steal_pointer (&label_value), g_object_ref (output_expr));
                    }
                }
              else
                {
                  g_autoptr(ShumateVectorValue) label_value = g_new0 (ShumateVectorValue, 1);

                  if (!shumate_vector_value_set_from_json_literal (label_value, label_node, error))
                    return NULL;

                  g_hash_table_insert (self->match_expressions, g_steal_pointer (&label_value), g_object_ref (output_expr));
                }
            }

          if (json_array_get_length (array) % 2 == 1)
            {
              JsonNode *default_node = json_array_get_element (array, json_array_get_length (array) - 1);
              ShumateVectorExpression *default_expr =
                shumate_vector_expression_filter_from_array_or_literal (default_node, ctx, error);

              if (default_expr == NULL)
                return NULL;

              g_ptr_array_add (self->expressions, g_steal_pointer (&default_expr));
            }

          return (ShumateVectorExpression *)g_steal_pointer (&self);
        }

      case EXPR_LET:
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

              expr = shumate_vector_expression_filter_from_array_or_literal (
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

          child_expr = shumate_vector_expression_filter_from_array_or_literal (
            json_array_get_element (array, json_array_get_length (array) - 1),
            &child_ctx,
            error
          );
          return child_expr;
        }

      case EXPR_VAR:
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

      case EXPR_E:
        {
          g_auto(ShumateVectorValue) value = SHUMATE_VECTOR_VALUE_INIT;
          shumate_vector_value_set_number (&value, G_E);
          return shumate_vector_expression_filter_from_literal (&value);
        }

      case EXPR_LN2:
        {
          g_auto(ShumateVectorValue) value = SHUMATE_VECTOR_VALUE_INIT;
          shumate_vector_value_set_number (&value, G_LN2);
          return shumate_vector_expression_filter_from_literal (&value);
        }

      case EXPR_PI:
        {
          g_auto(ShumateVectorValue) value = SHUMATE_VECTOR_VALUE_INIT;
          shumate_vector_value_set_number (&value, G_PI);
          return shumate_vector_expression_filter_from_literal (&value);
        }

      default:
        break;
    }

  self = g_object_new (SHUMATE_TYPE_VECTOR_EXPRESSION_FILTER, NULL);
  self->type = info->type;

  if (info->expect_exprs_set && json_array_get_length (array) - 1 != info->expect_exprs)
    {
      g_set_error (error,
                   SHUMATE_STYLE_ERROR,
                   SHUMATE_STYLE_ERROR_INVALID_EXPRESSION,
                   "Operator `%s` expected exactly %d arguments, got %d",
                   op, info->expect_exprs, json_array_get_length (array) - 1);
      return FALSE;
    }

  if (info->expect_ge_exprs_set && json_array_get_length (array) - 1 < info->expect_ge_exprs)
    {
      g_set_error (error,
                   SHUMATE_STYLE_ERROR,
                   SHUMATE_STYLE_ERROR_INVALID_EXPRESSION,
                   "Operator `%s` expected at least %d arguments, got %d",
                   op, info->expect_ge_exprs, json_array_get_length (array) - 1);
      return FALSE;
    }

  if (info->expect_le_exprs_set && json_array_get_length (array) - 1 > info->expect_le_exprs)
    {
      g_set_error (error,
                   SHUMATE_STYLE_ERROR,
                   SHUMATE_STYLE_ERROR_INVALID_EXPRESSION,
                   "Operator `%s` expected at most %d arguments, got %d",
                   op, info->expect_le_exprs, json_array_get_length (array) - 1);
      return FALSE;
    }

  for (int i = 1; i < json_array_get_length (array); i ++)
    {
      JsonNode *arg = json_array_get_element (array, i);
      g_autoptr(ShumateVectorExpression) expr = NULL;

      if (i == 1
          && info->lookup_first_arg
          && JSON_NODE_HOLDS_VALUE (arg)
          && json_node_get_value_type (arg) == G_TYPE_STRING)
        {
          /* If the first argument of a function is a string,
           * convert it to a GET expression so we can do things like
           * ["==", "admin_level", 2] */
          g_auto(ShumateVectorValue) value = SHUMATE_VECTOR_VALUE_INIT;
          const char *string = json_node_get_string (arg);

          if (g_strcmp0 ("zoom", string) == 0)
            {
              expr = g_object_new (SHUMATE_TYPE_VECTOR_EXPRESSION_FILTER, NULL);
              ((ShumateVectorExpressionFilter *)expr)->type = EXPR_ZOOM;
            }
          else if (g_strcmp0 ("$type", string) == 0)
            {
              expr = g_object_new (SHUMATE_TYPE_VECTOR_EXPRESSION_FILTER, NULL);
              ((ShumateVectorExpressionFilter *)expr)->type = EXPR_GEOMETRY_TYPE;
            }
          else
            {
              expr = g_object_new (SHUMATE_TYPE_VECTOR_EXPRESSION_FILTER, NULL);
              ((ShumateVectorExpressionFilter *)expr)->type = EXPR_FAST_GET;
              ((ShumateVectorExpressionFilter *)expr)->fast_get_key = g_strdup (string);
            }
        }
      else if (!(expr = shumate_vector_expression_filter_from_array_or_literal (arg, ctx, error)))
        return NULL;

      if (self->expressions == NULL)
        self->expressions = g_ptr_array_new_with_free_func (g_object_unref);
      g_ptr_array_add (self->expressions, g_steal_pointer (&expr));
    }

  return optimize (g_steal_pointer (&self));
}


ShumateVectorExpression *
shumate_vector_expression_filter_from_format (const char *format, GError **error)
{
  g_autoptr(ShumateVectorExpressionFilter) self = NULL;
  g_auto(GStrv) parts = NULL;
  int balance = 0;

  if (strchr (format, '{') == NULL || strchr (format, '}') == NULL)
    {
      g_auto(ShumateVectorValue) value = SHUMATE_VECTOR_VALUE_INIT;
      shumate_vector_value_set_string (&value, format);
      return shumate_vector_expression_filter_from_literal (&value);
    }

  self = g_object_new (SHUMATE_TYPE_VECTOR_EXPRESSION_FILTER, NULL);
  self->type = EXPR_CONCAT;
  self->expressions = g_ptr_array_new_with_free_func (g_object_unref);

  /* Ensure the braces in the format string are balanced and not nested */
  for (int i = 0, n = strlen (format); i < n; i ++)
    {
      if (format[i] == '{')
        balance ++;
      else if (format[i] == '}')
        balance --;

      if (balance != 0 && balance != 1)
        {
          g_set_error (error,
                       SHUMATE_STYLE_ERROR,
                       SHUMATE_STYLE_ERROR_INVALID_EXPRESSION,
                       "Format string `%s` is nested or unbalanced",
                       format);
          return NULL;
        }
    }

  parts = g_strsplit_set (format, "{}", 0);

  for (int i = 0; TRUE;)
    {
      g_auto(ShumateVectorValue) value = SHUMATE_VECTOR_VALUE_INIT;
      g_autoptr(ShumateVectorExpressionFilter) get_expr = NULL;

      if (parts[i] == NULL)
        break;

      if (strlen (parts[i]) > 0)
        {
          shumate_vector_value_set_string (&value, parts[i]);
          g_ptr_array_add (self->expressions, shumate_vector_expression_filter_from_literal (&value));
        }
      i ++;

      if (parts[i] == NULL)
        break;

      get_expr = g_object_new (SHUMATE_TYPE_VECTOR_EXPRESSION_FILTER, NULL);
      get_expr->type = EXPR_FAST_GET;
      get_expr->fast_get_key = g_strdup (parts[i]);
      g_ptr_array_add (self->expressions, g_steal_pointer (&get_expr));
      i ++;
    }

  return (ShumateVectorExpression *)g_steal_pointer (&self);
}

static GeometryTypeFlags
parse_geometry_type_flag (const char *flag)
{
  if (g_strcmp0 (flag, "Point") == 0)
    return GEOM_SINGLE_POINT;
  else if (g_strcmp0 (flag, "MultiPoint") == 0)
    return GEOM_MULTI_POINT;
  else if (g_strcmp0 (flag, "LineString") == 0)
    return GEOM_SINGLE_LINESTRING;
  else if (g_strcmp0 (flag, "MultiLineString") == 0)
    return GEOM_MULTI_LINESTRING;
  else if (g_strcmp0 (flag, "Polygon") == 0)
    return GEOM_SINGLE_POLYGON;
  else if (g_strcmp0 (flag, "MultiPolygon") == 0)
    return GEOM_MULTI_POLYGON;
  else
    return 0;
}

static ShumateVectorExpression *
optimize (ShumateVectorExpressionFilter *self)
{
  switch (self->type)
    {
    case EXPR_ANY:
    case EXPR_ALL:
      if (self->expressions == NULL || self->expressions->len == 0)
        {
          g_clear_pointer (&self->expressions, g_ptr_array_unref);
          shumate_vector_value_set_boolean (&self->value, self->type == EXPR_ALL);
          self->type = EXPR_LITERAL;
        }
      break;

    case EXPR_EQ:
    case EXPR_NE:
      {
        ShumateVectorExpressionFilter *key_expr, *value_expr;

        if (self->expressions->len != 2)
          break;

        key_expr = self->expressions->pdata[0];
        value_expr = self->expressions->pdata[1];

        if (!SHUMATE_IS_VECTOR_EXPRESSION_FILTER (key_expr) || !SHUMATE_IS_VECTOR_EXPRESSION_FILTER (value_expr))
          break;
        if (value_expr->type != EXPR_LITERAL)
          break;

        switch (key_expr->type)
          {
          case EXPR_FAST_GET:
            self->type = self->type == EXPR_EQ ? EXPR_FAST_EQ : EXPR_FAST_NE;
            self->fast_eq.key = g_strdup (key_expr->fast_get_key);
            shumate_vector_value_copy (&value_expr->value, &self->fast_eq.value);
            g_clear_pointer (&self->expressions, g_ptr_array_unref);
            break;
          case EXPR_GEOMETRY_TYPE:
            {
              const char *geometry_type;

              if (!shumate_vector_value_get_string (&value_expr->value, &geometry_type))
                break;

              self->type = self->type == EXPR_EQ ? EXPR_FAST_GEOMETRY_TYPE : EXPR_FAST_NOT_GEOMETRY_TYPE;
              self->fast_geometry_type = parse_geometry_type_flag (geometry_type);
              g_clear_pointer (&self->expressions, g_ptr_array_unref);
              break;
            }
          default:
            break;
          }

        break;
      }

    case EXPR_GET:
    case EXPR_HAS:
    case EXPR_NOT_HAS:
      {
        ShumateVectorExpressionFilter *key_expr = self->expressions->pdata[0];
        const char *key;

        if (self->expressions->len != 1)
          break;

        if (!SHUMATE_IS_VECTOR_EXPRESSION_FILTER (key_expr))
          break;
        if (key_expr->type != EXPR_LITERAL)
          break;

        if (shumate_vector_value_get_string (&key_expr->value, &key))
          {
            self->type = (self->type == EXPR_GET) ? EXPR_FAST_GET : (self->type == EXPR_HAS) ? EXPR_FAST_HAS : EXPR_FAST_NOT_HAS;
            self->fast_get_key = g_strdup (key);
            g_clear_pointer (&self->expressions, g_ptr_array_unref);
          }
        break;
      }

    case EXPR_IN:
    case EXPR_NOT_IN:
      {
        ShumateVectorExpressionFilter *key_expr = self->expressions->pdata[0];

        if (!SHUMATE_IS_VECTOR_EXPRESSION_FILTER (key_expr))
          break;

        switch (key_expr->type)
          {
          case EXPR_GEOMETRY_TYPE:
            for (int i = 1, n = self->expressions->len; i < n; i ++)
              {
                ShumateVectorExpressionFilter *expr = self->expressions->pdata[i];
                if (!SHUMATE_IS_VECTOR_EXPRESSION_FILTER (expr) || expr->type != EXPR_LITERAL)
                  break;
              }

            self->type = self->type == EXPR_IN ? EXPR_FAST_GEOMETRY_TYPE : EXPR_FAST_NOT_GEOMETRY_TYPE;
            self->fast_geometry_type = 0;

            for (int i = 1; i < self->expressions->len; i ++)
              {
                ShumateVectorExpressionFilter *value_expr = self->expressions->pdata[i];

                if (value_expr->value.type == SHUMATE_VECTOR_VALUE_TYPE_ARRAY)
                  {
                    for (int j = 0, n = value_expr->value.array->len; j < n; j ++)
                      {
                        const char *geometry_type;
                        if (shumate_vector_value_get_string (g_ptr_array_index (value_expr->value.array, j), &geometry_type))
                          self->fast_geometry_type |= parse_geometry_type_flag (geometry_type);
                      }
                  }
                else
                  {
                    const char *geometry_type;
                    if (shumate_vector_value_get_string (&value_expr->value, &geometry_type))
                      self->fast_geometry_type |= parse_geometry_type_flag (geometry_type);
                  }
              }

            g_clear_pointer (&self->expressions, g_ptr_array_unref);
            break;

          case EXPR_FAST_GET:
            for (int i = 1, n = self->expressions->len; i < n; i ++)
              {
                ShumateVectorExpressionFilter *expr = self->expressions->pdata[i];
                if (!SHUMATE_IS_VECTOR_EXPRESSION_FILTER (expr) || expr->type != EXPR_LITERAL)
                  break;
              }

            self->type = self->type == EXPR_IN ? EXPR_FAST_IN : EXPR_FAST_NOT_IN;
            self->fast_in.key = g_strdup (key_expr->fast_get_key);

            self->fast_in.haystack = g_hash_table_new_full (
              (GHashFunc)shumate_vector_value_hash, (GEqualFunc)shumate_vector_value_equal, (GDestroyNotify)shumate_vector_value_free, NULL
            );
            for (int i = 1; i < self->expressions->len; i ++)
              {
                ShumateVectorExpressionFilter *value_expr = self->expressions->pdata[i];

                if (value_expr->value.type == SHUMATE_VECTOR_VALUE_TYPE_ARRAY)
                  {
                    for (int j = 0, n = value_expr->value.array->len; j < n; j ++)
                      {
                        ShumateVectorValue *value = g_new0 (ShumateVectorValue, 1);
                        shumate_vector_value_copy (g_ptr_array_index (value_expr->value.array, j), value);
                        g_hash_table_add (self->fast_in.haystack, value);
                      }
                  }
                else
                  {
                    ShumateVectorValue *value = g_new0 (ShumateVectorValue, 1);
                    shumate_vector_value_copy (&value_expr->value, value);
                    g_hash_table_add (self->fast_in.haystack, value);
                  }
              }

            g_clear_pointer (&self->expressions, g_ptr_array_unref);
            break;

          default:
            break;
          }

        break;
      }

    default:
      break;
    }

  return (ShumateVectorExpression *)self;
}


static void
shumate_vector_expression_filter_finalize (GObject *object)
{
  ShumateVectorExpressionFilter *self = (ShumateVectorExpressionFilter *)object;

  g_clear_pointer (&self->expressions, g_ptr_array_unref);

  switch (self->type)
    {
    case EXPR_LITERAL:
      shumate_vector_value_unset (&self->value);
      break;
    case EXPR_FORMAT:
      g_clear_pointer (&self->format_parts, g_ptr_array_unref);
      break;
    case EXPR_MATCH:
      g_clear_pointer (&self->match_expressions, g_hash_table_unref);
      break;
    case EXPR_FAST_GET:
    case EXPR_FAST_HAS:
    case EXPR_FAST_NOT_HAS:
      g_clear_pointer (&self->fast_get_key, g_free);
      break;
    case EXPR_FAST_IN:
    case EXPR_FAST_NOT_IN:
      g_clear_pointer (&self->fast_in.key, g_free);
      g_clear_pointer (&self->fast_in.haystack, g_hash_table_unref);
      break;
    case EXPR_FAST_EQ:
    case EXPR_FAST_NE:
      g_clear_pointer (&self->fast_eq.key, g_free);
      shumate_vector_value_unset (&self->fast_eq.value);
      break;
    default:
      break;
    }

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
  g_auto(ShumateVectorValue) value3 = SHUMATE_VECTOR_VALUE_INIT;
  gboolean inverted = FALSE;
  gboolean boolean;
  double number, number2;
  g_autofree char *string = NULL;
  ShumateVectorExpression **expressions = NULL;
  ShumateVectorCollator collator;
  guint n_expressions = 0;

  if (self->expressions)
    {
      expressions = (ShumateVectorExpression **)self->expressions->pdata;
      n_expressions = self->expressions->len;
    }

  switch (self->type)
    {
    case EXPR_LITERAL:
      shumate_vector_value_copy (&self->value, out);
      return TRUE;

    case EXPR_TO_BOOLEAN:
      {
        g_assert (n_expressions == 1);

        if (!shumate_vector_expression_eval (expressions[0], scope, &value))
          return FALSE;

        switch (value.type)
          {
            case SHUMATE_VECTOR_VALUE_TYPE_NULL:
              shumate_vector_value_set_boolean (out, FALSE);
              return TRUE;
            case SHUMATE_VECTOR_VALUE_TYPE_BOOLEAN:
              shumate_vector_value_copy (&value, out);
              return TRUE;
            case SHUMATE_VECTOR_VALUE_TYPE_NUMBER:
              shumate_vector_value_set_boolean (out, value.number != 0 && !isnan (value.number));
              return TRUE;
            case SHUMATE_VECTOR_VALUE_TYPE_STRING:
              shumate_vector_value_set_boolean (out, value.string != NULL && value.string[0] != '\0');
              return TRUE;
            default:
              shumate_vector_value_set_boolean (out, TRUE);
              return TRUE;
          }
      }

    case EXPR_TO_COLOR:
      for (int i = 0; i < n_expressions; i ++)
        {
          GdkRGBA rgba;

          if (!shumate_vector_expression_eval (expressions[i], scope, &value))
            continue;

          if (shumate_vector_value_get_color (&value, &rgba))
            {
              shumate_vector_value_set_color (out, &rgba);
              return TRUE;
            }
        }

      return FALSE;

    case EXPR_TO_NUMBER:
      for (int i = 0; i < n_expressions; i ++)
        {
          if (!shumate_vector_expression_eval (expressions[i], scope, &value))
            continue;

          switch (value.type)
            {
              case SHUMATE_VECTOR_VALUE_TYPE_NULL:
                shumate_vector_value_set_number (out, 0);
                return TRUE;
              case SHUMATE_VECTOR_VALUE_TYPE_BOOLEAN:
                shumate_vector_value_set_number (out, value.boolean ? 1 : 0);
                return TRUE;
              case SHUMATE_VECTOR_VALUE_TYPE_NUMBER:
                shumate_vector_value_copy (&value, out);
                return TRUE;
              case SHUMATE_VECTOR_VALUE_TYPE_STRING:
                {
                  char *endptr = NULL;
                  number = g_ascii_strtod (value.string, &endptr);
                  if (errno == 0 && endptr != NULL && *endptr == '\0')
                    {
                      shumate_vector_value_set_number (out, number);
                      return TRUE;
                    }
                }
                break;
              default:
                break;
            }
        }

      return FALSE;

    case EXPR_TO_STRING:
      {
        g_autofree char *string = NULL;

        g_assert (n_expressions == 1);

        if (!shumate_vector_expression_eval (expressions[0], scope, &value))
          return FALSE;
        string = shumate_vector_value_as_string (&value);
        shumate_vector_value_set_string (out, string);
        return TRUE;
      }

    case EXPR_TYPEOF:
      {
        g_assert (n_expressions == 1);

        if (!shumate_vector_expression_eval (expressions[0], scope, &value))
          return FALSE;

        switch (value.type)
          {
            case SHUMATE_VECTOR_VALUE_TYPE_NULL:
              shumate_vector_value_set_string (out, "null");
              return TRUE;
            case SHUMATE_VECTOR_VALUE_TYPE_BOOLEAN:
              shumate_vector_value_set_string (out, "boolean");
              return TRUE;
            case SHUMATE_VECTOR_VALUE_TYPE_NUMBER:
              shumate_vector_value_set_string (out, "number");
              return TRUE;
            case SHUMATE_VECTOR_VALUE_TYPE_STRING:
              shumate_vector_value_set_string (out, "string");
              return TRUE;
            case SHUMATE_VECTOR_VALUE_TYPE_ARRAY:
              shumate_vector_value_set_string (out, "array");
              return TRUE;
            case SHUMATE_VECTOR_VALUE_TYPE_COLOR:
              shumate_vector_value_set_string (out, "color");
              return TRUE;
            case SHUMATE_VECTOR_VALUE_TYPE_RESOLVED_IMAGE:
              shumate_vector_value_set_string (out, "resolvedImage");
              return TRUE;
            default:
              shumate_vector_value_set_string (out, "value");
              return TRUE;
          }
      }

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
      shumate_vector_render_scope_get_variable (scope, string, out);
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

          if (shumate_vector_value_equal (&value, &value2))
            {
              shumate_vector_value_set_boolean (out, TRUE ^ inverted);
              return TRUE;
            }
          else if (value2.type == SHUMATE_VECTOR_VALUE_TYPE_ARRAY)
            {
              for (int i = 0, n = value2.array->len; i < n; i ++)
                if (shumate_vector_value_equal (&value, g_ptr_array_index (value2.array, i)))
                  {
                    shumate_vector_value_set_boolean (out, TRUE ^ inverted);
                    return TRUE;
                  }
            }
        }

      shumate_vector_value_set_boolean (out, FALSE ^ inverted);
      return TRUE;

    case EXPR_NE:
      inverted = TRUE;
      G_GNUC_FALLTHROUGH;
    case EXPR_EQ:
      g_assert (n_expressions == 2 || n_expressions == 3);

      if (!shumate_vector_expression_eval (expressions[0], scope, &value))
        return FALSE;
      if (!shumate_vector_expression_eval (expressions[1], scope, &value2))
        return FALSE;

      if (n_expressions == 3)
        {
          const char *str1, *str2;
          g_autofree char *str1_auto = NULL;
          g_autofree char *str2_auto = NULL;

          if (!shumate_vector_value_get_string (&value, &str1))
            return FALSE;
          if (!shumate_vector_value_get_string (&value2, &str2))
            return FALSE;

          if (!shumate_vector_expression_eval (expressions[2], scope, &value3))
            return FALSE;
          if (!shumate_vector_value_get_collator (&value3, &collator))
            return FALSE;

          if (!collator.case_sensitive)
            {
              str1 = str1_auto = g_utf8_casefold (str1, -1);
              str2 = str2_auto = g_utf8_casefold (str2, -1);
            }

          shumate_vector_value_set_boolean (out, (g_utf8_collate (str1, str2) == 0) ^ inverted);
          return TRUE;
        }
      else
        {
          shumate_vector_value_set_boolean (out, shumate_vector_value_equal (&value, &value2) ^ inverted);
          return TRUE;
        }

    case EXPR_LT:
    case EXPR_GT:
    case EXPR_LE:
    case EXPR_GE:
      {
        int cmp;
        const char *str, *str2;
        g_autofree char *str_auto = NULL;
        g_autofree char *str2_auto = NULL;
        g_assert (n_expressions == 2 || n_expressions == 3);

        if (!shumate_vector_expression_eval (expressions[0], scope, &value))
          return FALSE;
        if (!shumate_vector_expression_eval (expressions[1], scope, &value2))
          return FALSE;

        if (n_expressions == 3)
          {
            if (!shumate_vector_expression_eval (expressions[2], scope, &value3))
              return FALSE;
            if (!shumate_vector_value_get_collator (&value3, &collator))
              return FALSE;

            if (!shumate_vector_value_get_string (&value, &str))
              return FALSE;
            if (!shumate_vector_value_get_string (&value2, &str2))
              return FALSE;

            if (!collator.case_sensitive)
              {
                str = str_auto = g_utf8_casefold (str, -1);
                str2 = str2_auto = g_utf8_casefold (str2, -1);
              }

            cmp = g_utf8_collate (str, str2);
          }
        else if (shumate_vector_value_get_number (&value, &number))
          {
            if (!shumate_vector_value_get_number (&value2, &number2))
              return FALSE;

            cmp = number > number2 ? 1 : number < number2 ? -1 : 0;
          }
        else if (shumate_vector_value_get_string (&value, &str))
          {
            if (!shumate_vector_value_get_string (&value2, &str2))
              return FALSE;

            cmp = g_utf8_collate (str, str2);
          }
        else
          return FALSE;

        switch (self->type)
          {
          case EXPR_LT:
            shumate_vector_value_set_boolean (out, (cmp < 0) ^ inverted);
            break;
          case EXPR_GT:
            shumate_vector_value_set_boolean (out, (cmp > 0) ^ inverted);
            break;
          case EXPR_LE:
            shumate_vector_value_set_boolean (out, (cmp <= 0) ^ inverted);
            break;
          case EXPR_GE:
            shumate_vector_value_set_boolean (out, (cmp >= 0) ^ inverted);
            break;
          default:
            g_assert_not_reached ();
          }

        return TRUE;
      }

    case EXPR_CASE:
      for (int i = 0; i < n_expressions; i += 2)
        {
          if (!shumate_vector_expression_eval (expressions[i], scope, &value))
            return FALSE;

          if (i + 1 == n_expressions)
            {
              /* fallback value */
              shumate_vector_value_steal (&value, out);
              return TRUE;
            }
          else
            {
              gboolean bool_result;
              if (!shumate_vector_value_get_boolean (&value, &bool_result))
                return FALSE;

              if (bool_result)
                return shumate_vector_expression_eval (expressions[i + 1], scope, out);
            }
        }

      /* no case matched and there was no fallback */
      return FALSE;

    case EXPR_COALESCE:
      for (int i = 0; i < n_expressions; i ++)
        {
          if (!shumate_vector_expression_eval (expressions[i], scope, out))
            continue;

          if (shumate_vector_value_is_null (out))
            continue;

          return TRUE;
        }

      /* no expression succeeded, return null */
      shumate_vector_value_unset (out);
      return TRUE;

    case EXPR_MATCH:
      {
        ShumateVectorExpression *output_expr;

        g_assert (n_expressions == 1 || n_expressions == 2);

        if (!shumate_vector_expression_eval (expressions[0], scope, &value))
          return FALSE;

        output_expr = g_hash_table_lookup (self->match_expressions, &value);

        if (output_expr != NULL)
          return shumate_vector_expression_eval (output_expr, scope, out);
        else if (n_expressions == 2)
          return shumate_vector_expression_eval (expressions[1], scope, out);
        else
          return FALSE;
      }

      /* no case matched and there was no fallback */
      return FALSE;

    case EXPR_CONCAT:
      {
        g_autoptr(GString) string_builder = g_string_new ("");

        for (int i = 0; i < n_expressions; i ++)
          {
            if (!shumate_vector_expression_eval (expressions[i], scope, &value))
              return FALSE;

            g_free (string);
            string = shumate_vector_value_as_string (&value);

            g_string_append (string_builder, string);
          }

        shumate_vector_value_set_string (out, string_builder->str);
        return TRUE;
      }

    case EXPR_DOWNCASE:
      {
        const char *str;
        g_autofree char *string_down = NULL;
        g_assert (n_expressions == 1);

        if (!shumate_vector_expression_eval (expressions[0], scope, &value))
          return FALSE;
        if (!shumate_vector_value_get_string (&value, &str))
          return FALSE;

        string_down = g_utf8_strdown (str, -1);

        shumate_vector_value_set_string (out, string_down);
        return TRUE;
      }

    case EXPR_RESOLVED_LOCALE:
      {
        g_autofree char *locale = NULL;

        g_assert (n_expressions == 1);

        if (!shumate_vector_expression_eval (expressions[0], scope, &value))
          return FALSE;
        if (!shumate_vector_value_get_collator (&value, &collator))
          return FALSE;

        locale = g_strdup (g_get_language_names ()[0]);

        /* if there is an encoding at the end, remove it */
        if (strchr (locale, '.'))
          *strchr (locale, '.') = '\0';

        /* replace _ with - */
        g_strdelimit (locale, "_", '-');

        shumate_vector_value_set_string (out, locale);
        return TRUE;
      }

    case EXPR_UPCASE:
      {
        const char *str;
        g_autofree char *string_up = NULL;
        g_assert (n_expressions == 1);

        if (!shumate_vector_expression_eval (expressions[0], scope, &value))
          return FALSE;
        if (!shumate_vector_value_get_string (&value, &str))
          return FALSE;

        string_up = g_utf8_strup (str, -1);

        shumate_vector_value_set_string (out, string_up);
        return TRUE;
      }

    case EXPR_ADD:
    case EXPR_MUL:
    case EXPR_MIN:
    case EXPR_MAX:
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

          switch (self->type)
            {
            case EXPR_ADD:
              number += number2;
              break;
            case EXPR_MUL:
              number *= number2;
              break;
            case EXPR_MIN:
              number = MIN (number, number2);
              break;
            case EXPR_MAX:
              number = MAX (number, number2);
              break;
            default:
              g_assert_not_reached ();
            }
        }

      shumate_vector_value_set_number (out, number);
      return TRUE;

    case EXPR_SUB:
      g_assert (n_expressions >= 1);
      g_assert (n_expressions <= 2);

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

    case EXPR_DIV:
    case EXPR_REM:
    case EXPR_POW:
      g_assert (n_expressions == 2);

      if (!shumate_vector_expression_eval (expressions[0], scope, &value))
        return FALSE;
      if (!shumate_vector_value_get_number (&value, &number))
        return FALSE;

      if (!shumate_vector_expression_eval (expressions[1], scope, &value2))
        return FALSE;
      if (!shumate_vector_value_get_number (&value2, &number2))
        return FALSE;

      switch (self->type)
        {
        case EXPR_DIV:
          if (number2 == 0)
            shumate_vector_value_set_number (out, number == 0 ? NAN : number > 0 ? INFINITY : -INFINITY);
          else
            shumate_vector_value_set_number (out, number / number2);
          return TRUE;

        case EXPR_REM:
          if (number2 == 0)
            shumate_vector_value_set_number (out, NAN);
          else
            shumate_vector_value_set_number (out, fmod (number, number2));
          return TRUE;

        case EXPR_POW:
          number = pow (number, number2);
          if (isnan (number))
            return FALSE;

          shumate_vector_value_set_number (out, number);
          return TRUE;

        default:
          g_assert_not_reached ();
        }

      return TRUE;

    case EXPR_ABS:
    case EXPR_ACOS:
    case EXPR_ASIN:
    case EXPR_ATAN:
    case EXPR_CEIL:
    case EXPR_COS:
    case EXPR_FLOOR:
    case EXPR_LN:
    case EXPR_LOG10:
    case EXPR_LOG2:
    case EXPR_ROUND:
    case EXPR_SIN:
    case EXPR_SQRT:
    case EXPR_TAN:
      g_assert (n_expressions == 1);

      if (!shumate_vector_expression_eval (expressions[0], scope, &value))
        return FALSE;
      if (!shumate_vector_value_get_number (&value, &number))
        return FALSE;

      switch (self->type)
        {
        case EXPR_ABS:
          number = fabs (number);
          break;
        case EXPR_ACOS:
          number = acos (number);
          break;
        case EXPR_ASIN:
          number = asin (number);
          break;
        case EXPR_ATAN:
          number = atan (number);
          break;
        case EXPR_CEIL:
          number = ceil (number);
          break;
        case EXPR_COS:
          number = cos (number);
          break;
        case EXPR_FLOOR:
          number = floor (number);
          break;
        case EXPR_LN:
          number = log (number);
          break;
        case EXPR_LOG10:
          number = log10 (number);
          break;
        case EXPR_LOG2:
          number = log2 (number);
          break;
        case EXPR_ROUND:
          number = round (number);
          break;
        case EXPR_SIN:
          number = sin (number);
          break;
        case EXPR_SQRT:
          number = sqrt (number);
          break;
        case EXPR_TAN:
          number = tan (number);
          break;
        default:
          g_assert_not_reached ();
        }

      if (isnan (number))
        return FALSE;

      shumate_vector_value_set_number (out, number);
      return TRUE;

    case EXPR_IMAGE:
        {
          g_autoptr(ShumateVectorSprite) sprite = NULL;
          g_assert (n_expressions == 1);

          string = shumate_vector_expression_eval_string (expressions[0], scope, NULL);
          if (string == NULL)
            {
              shumate_vector_value_unset (out);
              return TRUE;
            }

          sprite = shumate_vector_sprite_sheet_get_sprite (scope->sprites, string, scope->scale_factor);
          if (sprite == NULL)
            {
              shumate_vector_value_unset (out);
              return TRUE;
            }

          shumate_vector_value_set_image (out, sprite, string);
          return TRUE;
        }

    case EXPR_COLLATOR:
      g_assert (n_expressions == 1);

      collator.case_sensitive = shumate_vector_expression_eval_boolean (expressions[0], scope, FALSE);
      shumate_vector_value_set_collator (out, &collator);
      return TRUE;

    case EXPR_FORMAT:
        {
          FormatExpressionPart **format_expressions = (FormatExpressionPart **)self->format_parts->pdata;
          n_expressions = self->format_parts->len;
          g_autoptr(GPtrArray) format_parts = g_ptr_array_new_full (n_expressions, (GDestroyNotify)shumate_vector_format_part_free);

          for (int i = 0; i < n_expressions; i++)
            {
              g_autoptr(ShumateVectorFormatPart) part = g_new0 (ShumateVectorFormatPart, 1);

              if (!shumate_vector_expression_eval (format_expressions[i]->string, scope, &value))
                return FALSE;

              switch (value.type)
                {
                case SHUMATE_VECTOR_VALUE_TYPE_STRING:
                  {
                    const char *string;
                    shumate_vector_value_get_string (&value, &string);

                    if (strlen (string) == 0)
                      continue;

                    part->string = g_strdup (string);
                    break;
                  }

                case SHUMATE_VECTOR_VALUE_TYPE_RESOLVED_IMAGE:
                  {
                    part->string = shumate_vector_value_as_string (&value);
                    shumate_vector_value_get_image (&value, &part->sprite);
                    g_object_ref (part->sprite);
                    break;
                  }

                case SHUMATE_VECTOR_VALUE_TYPE_NULL:
                  continue;

                default:
                  return FALSE;
                }

              if (format_expressions[i]->text_color != NULL)
                {
                  if (!shumate_vector_expression_eval (format_expressions[i]->text_color, scope, &value))
                    return FALSE;

                  if (!shumate_vector_value_is_null (&value))
                    {
                      if (!shumate_vector_value_get_color (&value, &part->text_color))
                        return FALSE;
                      part->has_text_color = TRUE;
                    }
                }

              if (format_expressions[i]->font_scale != NULL)
                {
                  if (!shumate_vector_expression_eval (format_expressions[i]->font_scale, scope, &value))
                    return FALSE;

                  if (!shumate_vector_value_is_null (&value))
                    {
                      if (!shumate_vector_value_get_number (&value, &part->font_scale))
                        return FALSE;
                      part->has_font_scale = TRUE;
                    }
                }

              g_ptr_array_add (format_parts, g_steal_pointer (&part));
            }

          shumate_vector_value_set_formatted (out, format_parts);
          return TRUE;
        }

    case EXPR_GEOMETRY_TYPE:
      {
        VectorTile__Tile__Feature *feature = shumate_vector_reader_iter_get_feature_struct (scope->reader);
        ShumateGeometryType geometry_type;

        if (feature == NULL)
          {
            shumate_vector_value_unset (out);
            return TRUE;
          }

        geometry_type = shumate_vector_reader_iter_get_feature_geometry_type (scope->reader);

        switch (geometry_type)
          {
          case SHUMATE_GEOMETRY_TYPE_POINT:
            shumate_vector_value_set_string (out, "Point");
            return TRUE;
          case SHUMATE_GEOMETRY_TYPE_MULTIPOINT:
            shumate_vector_value_set_string (out, "MultiPoint");
            return TRUE;
          case SHUMATE_GEOMETRY_TYPE_LINESTRING:
            shumate_vector_value_set_string (out, "LineString");
            return TRUE;
          case SHUMATE_GEOMETRY_TYPE_MULTILINESTRING:
            shumate_vector_value_set_string (out, "MultiLineString");
            return TRUE;
          case SHUMATE_GEOMETRY_TYPE_POLYGON:
            shumate_vector_value_set_string (out, "Polygon");
            return TRUE;
          case SHUMATE_GEOMETRY_TYPE_MULTIPOLYGON:
            shumate_vector_value_set_string (out, "MultiPolygon");
            return TRUE;
          default:
            shumate_vector_value_unset (out);
            return TRUE;
          }
      }

    case EXPR_ID:
      {
        VectorTile__Tile__Feature *feature = shumate_vector_reader_iter_get_feature_struct (scope->reader);

        if (!feature || !feature->has_id)
          shumate_vector_value_unset (out);
        else
          shumate_vector_value_set_number (out, feature->id);
        return TRUE;
      }

    case EXPR_ZOOM:
      shumate_vector_value_set_number (out, scope->zoom_level);
      return TRUE;

    case EXPR_AT:
      {
        GPtrArray *array;
        g_assert (n_expressions == 2);

        if (!shumate_vector_expression_eval (expressions[0], scope, &value))
          return FALSE;
        if (!shumate_vector_value_get_number (&value, &number))
          return FALSE;

        if (!shumate_vector_expression_eval (expressions[1], scope, &value2))
          return FALSE;
        if (!(array = shumate_vector_value_get_array (&value2)))
          return FALSE;

        if (number < 0 || number >= array->len || number != (int)number)
          return FALSE;

        shumate_vector_value_copy (g_ptr_array_index (array, (int)number), out);
        return TRUE;
      }

    case EXPR_INDEX_OF:
      g_assert (n_expressions == 2 || n_expressions == 3);

      if (!shumate_vector_expression_eval (expressions[0], scope, &value))
        return FALSE;

      if (n_expressions == 3)
        {
          if (!shumate_vector_expression_eval (expressions[2], scope, &value2))
            return FALSE;
          if (!shumate_vector_value_get_number (&value2, &number))
            return FALSE;
          if (number != (int)number || number < 0)
            return FALSE;
        }
      else
        number = 0;

      if (!shumate_vector_expression_eval (expressions[1], scope, &value2))
        return FALSE;

      switch (value2.type)
        {
          case SHUMATE_VECTOR_VALUE_TYPE_STRING:
            {
              const char *string;
              const char *substring;
              const char *found;

              shumate_vector_value_get_string (&value2, &string);
              if (!shumate_vector_value_get_string (&value, &substring))
                return FALSE;

              number = MIN (number, g_utf8_strlen (string, -1));
              found = strstr (g_utf8_offset_to_pointer (string, (int)number), substring);

              if (found == NULL)
                shumate_vector_value_set_number (out, -1);
              else
                shumate_vector_value_set_number (out, g_utf8_pointer_to_offset (string, found));

              return TRUE;
            }

          case SHUMATE_VECTOR_VALUE_TYPE_ARRAY:
            {
              GPtrArray *array = shumate_vector_value_get_array (&value2);

              for (; number < array->len; number ++)
                {
                  if (shumate_vector_value_equal (g_ptr_array_index (array, (int)number), &value))
                    {
                      shumate_vector_value_set_number (out, number);
                      return TRUE;
                    }
                }

              shumate_vector_value_set_number (out, -1);
              return TRUE;
            }

          default:
            return FALSE;
        }

    case EXPR_LENGTH:
      g_assert (n_expressions == 1);

      if (!shumate_vector_expression_eval (expressions[0], scope, &value))
        return FALSE;

      switch (value.type)
        {
        case SHUMATE_VECTOR_VALUE_TYPE_STRING:
          {
            const char *string;
            shumate_vector_value_get_string (&value, &string);
            shumate_vector_value_set_number (out, g_utf8_strlen (string, -1));
            return TRUE;
          }

        case SHUMATE_VECTOR_VALUE_TYPE_ARRAY:
          {
            GPtrArray *array = shumate_vector_value_get_array (&value);
            shumate_vector_value_set_number (out, array->len);
            return TRUE;
          }

        default:
          return FALSE;
        }

    case EXPR_SLICE:
      {
        double start, end;

        g_assert (n_expressions == 2 || n_expressions == 3);

        if (!shumate_vector_expression_eval (expressions[0], scope, &value))
          return FALSE;

        if (!shumate_vector_expression_eval (expressions[1], scope, &value2))
          return FALSE;
        if (!shumate_vector_value_get_number (&value2, &start))
          return FALSE;
        if (start != (int)start)
          return FALSE;

        if (n_expressions == 3)
          {
            if (!shumate_vector_expression_eval (expressions[2], scope, &value2))
              return FALSE;
            if (!shumate_vector_value_get_number (&value2, &end))
              return FALSE;
            if (end != (int)end)
              return FALSE;
          }
        else
          end = G_MAXDOUBLE;

        switch (value.type)
          {
          case SHUMATE_VECTOR_VALUE_TYPE_STRING:
            {
              const char *input;
              const char *start_ptr, *end_ptr;
              long len;

              shumate_vector_value_get_string (&value, &input);
              len = g_utf8_strlen (input, -1);

              if (start < 0)
                start = MAX (len + start, 0);
              else
                start = MIN (start, len);

              if (end < 0)
                end = MAX (len + end, 0);
              else
                end = MIN (end, len);

              if (start >= end)
                {
                  shumate_vector_value_set_string (out, "");
                  return TRUE;
                }

              start_ptr = g_utf8_offset_to_pointer (input, (int)start);
              end_ptr = g_utf8_offset_to_pointer (input, (int)end);

              string = g_strndup (start_ptr, end_ptr - start_ptr);
              shumate_vector_value_set_string (out, string);
              return TRUE;
            }

          case SHUMATE_VECTOR_VALUE_TYPE_ARRAY:
            {
              GPtrArray *array = shumate_vector_value_get_array (&value);
              long len = array->len;

              if (start < 0)
                start = MAX (len + start, 0);
              else
                start = MIN (start, len);

              if (end < 0)
                end = MAX (len + end, 0);
              else
                end = MIN (end, len);

              shumate_vector_value_start_array (out);
              for (int i = start; i < end; i ++)
                {
                  shumate_vector_value_copy (
                    g_ptr_array_index (array, (int)i),
                    &value2
                  );
                  shumate_vector_value_array_append (out, &value2);
                }
              return TRUE;
            }

          default:
            return FALSE;
          }
      }

    case EXPR_FAST_NOT_HAS:
      inverted = TRUE;
      G_GNUC_FALLTHROUGH;
    case EXPR_FAST_HAS:
      shumate_vector_render_scope_get_variable (scope, self->fast_get_key, &value);
      shumate_vector_value_set_boolean (out, !shumate_vector_value_is_null (&value) ^ inverted);
      return TRUE;

    case EXPR_FAST_GET:
      shumate_vector_render_scope_get_variable (scope, self->fast_get_key, out);
      return TRUE;

    case EXPR_FAST_NOT_IN:
      inverted = TRUE;
      G_GNUC_FALLTHROUGH;
    case EXPR_FAST_IN:
      shumate_vector_render_scope_get_variable (scope, self->fast_in.key, &value);
      shumate_vector_value_set_boolean (out, g_hash_table_contains (self->fast_in.haystack, &value) ^ inverted);
      return TRUE;

    case EXPR_FAST_NE:
      inverted = TRUE;
      G_GNUC_FALLTHROUGH;
    case EXPR_FAST_EQ:
      shumate_vector_render_scope_get_variable (scope, self->fast_eq.key, &value);
      shumate_vector_value_set_boolean (out, shumate_vector_value_equal (&value, &self->fast_eq.value) ^ inverted);
      return TRUE;

    case EXPR_FAST_NOT_GEOMETRY_TYPE:
      inverted = TRUE;
      G_GNUC_FALLTHROUGH;
    case EXPR_FAST_GEOMETRY_TYPE:
      {
        VectorTile__Tile__Feature *feature = shumate_vector_reader_iter_get_feature_struct (scope->reader);
        gboolean result = FALSE;

        if (feature == NULL)
          {
            shumate_vector_value_unset (out);
            return TRUE;
          }

        switch (feature->type)
          {
          case VECTOR_TILE__TILE__GEOM_TYPE__POINT:
            if (self->fast_geometry_type & GEOM_ANY_POINT)
              {
                if ((self->fast_geometry_type & GEOM_ANY_POINT) == GEOM_ANY_POINT)
                  result = TRUE;
                else
                  {
                    ShumateGeometryType geometry_type = shumate_vector_reader_iter_get_feature_geometry_type (scope->reader);
                    if (self->fast_geometry_type & GEOM_SINGLE_POINT)
                      result = (geometry_type == SHUMATE_GEOMETRY_TYPE_POINT);
                    else if (self->fast_geometry_type & GEOM_MULTI_POINT)
                      result = (geometry_type == SHUMATE_GEOMETRY_TYPE_MULTIPOINT);
                    else
                      g_assert_not_reached ();
                  }
              }
            else
              result = FALSE;
            break;

          case VECTOR_TILE__TILE__GEOM_TYPE__LINESTRING:
            if (self->fast_geometry_type & GEOM_ANY_LINESTRING)
              {
                if ((self->fast_geometry_type & GEOM_ANY_LINESTRING) == GEOM_ANY_LINESTRING)
                  result = TRUE;
                else
                  {
                    ShumateGeometryType geometry_type = shumate_vector_reader_iter_get_feature_geometry_type (scope->reader);
                    if (self->fast_geometry_type & GEOM_SINGLE_LINESTRING)
                      result = (geometry_type == SHUMATE_GEOMETRY_TYPE_LINESTRING);
                    else if (self->fast_geometry_type & GEOM_MULTI_LINESTRING)
                      result = (geometry_type == SHUMATE_GEOMETRY_TYPE_MULTILINESTRING);
                    else
                      g_assert_not_reached ();
                  }
              }
            else
              result = FALSE;
            break;

          case VECTOR_TILE__TILE__GEOM_TYPE__POLYGON:
            if (self->fast_geometry_type & GEOM_ANY_POLYGON)
              {
                if ((self->fast_geometry_type & GEOM_ANY_POLYGON) == GEOM_ANY_POLYGON)
                  result = TRUE;
                else
                  {
                    ShumateGeometryType geometry_type = shumate_vector_reader_iter_get_feature_geometry_type (scope->reader);
                    if (self->fast_geometry_type & GEOM_SINGLE_POLYGON)
                      result = (geometry_type == SHUMATE_GEOMETRY_TYPE_POLYGON);
                    else if (self->fast_geometry_type & GEOM_MULTI_POLYGON)
                      result = (geometry_type == SHUMATE_GEOMETRY_TYPE_MULTIPOLYGON);
                    else
                      g_assert_not_reached ();
                  }
              }
            else
              result = FALSE;
            break;

          default:
            result = FALSE;
            break;
          }

        shumate_vector_value_set_boolean (out, result ^ inverted);
        return TRUE;
      }

    default:
      g_assert_not_reached ();
    }
}

static ShumateVectorIndexBitset *
shumate_vector_expression_filter_eval_bitset (ShumateVectorExpression  *expr,
                                              ShumateVectorRenderScope *scope,
                                              ShumateVectorIndexBitset *mask)
{
  ShumateVectorExpressionFilter *self = (ShumateVectorExpressionFilter *)expr;
  VectorTile__Tile__Layer *layer = shumate_vector_reader_iter_get_layer_struct (scope->reader);
  ShumateVectorIndexBitset *bitset = NULL;

  switch (self->type)
    {
      case EXPR_LITERAL:
        {
          gboolean result = FALSE;
          bitset = shumate_vector_index_bitset_new (layer->n_features);
          shumate_vector_value_get_boolean (&self->value, &result);
          if (result)
            shumate_vector_index_bitset_not (bitset);
          return bitset;
        }

      case EXPR_FAST_EQ:
      case EXPR_FAST_NE:
        bitset = shumate_vector_index_get_bitset (scope->index, scope->source_layer_idx, self->fast_eq.key, &self->fast_eq.value);
        if (bitset == NULL)
          bitset = shumate_vector_index_bitset_new (layer->n_features);
        else
          bitset = shumate_vector_index_bitset_copy (bitset);
        if (self->type == EXPR_FAST_NE)
          shumate_vector_index_bitset_not (bitset);
        return bitset;

      case EXPR_FAST_NOT_IN:
      case EXPR_FAST_IN:
        {
          GHashTableIter iter;
          ShumateVectorValue *value;
          bitset = shumate_vector_index_bitset_new (layer->n_features);
          g_hash_table_iter_init (&iter, self->fast_in.haystack);
          while (g_hash_table_iter_next (&iter, NULL, (gpointer *)&value))
            {
              ShumateVectorIndexBitset *current_bitset = shumate_vector_index_get_bitset (scope->index, scope->source_layer_idx, self->fast_in.key, value);
              if (current_bitset != NULL)
                shumate_vector_index_bitset_or (bitset, current_bitset);
            }

          if (self->type == EXPR_FAST_NOT_IN)
            shumate_vector_index_bitset_not (bitset);

          return bitset;
        }

      case EXPR_FAST_HAS:
      case EXPR_FAST_NOT_HAS:
        bitset = shumate_vector_index_get_bitset_has (scope->index, scope->source_layer_idx, self->fast_get_key);
        if (bitset == NULL)
          bitset = shumate_vector_index_bitset_new (layer->n_features);
        else
          bitset = shumate_vector_index_bitset_copy (bitset);
        if (self->type == EXPR_FAST_NOT_HAS)
          shumate_vector_index_bitset_not (bitset);
        return bitset;

      case EXPR_FAST_GEOMETRY_TYPE:
      case EXPR_FAST_NOT_GEOMETRY_TYPE:
        {
          ShumateVectorIndexBitset *current_bitset;
          bitset = shumate_vector_index_bitset_new (layer->n_features);

          switch (self->fast_geometry_type & GEOM_ANY_POINT)
            {
              case GEOM_ANY_POINT:
                current_bitset = shumate_vector_index_get_bitset_broad_geometry_type (scope->index, scope->source_layer_idx, SHUMATE_GEOMETRY_TYPE_POINT);
                break;
              case GEOM_SINGLE_POINT:
                current_bitset = shumate_vector_index_get_bitset_geometry_type (scope->index, scope->source_layer_idx, SHUMATE_GEOMETRY_TYPE_POINT);
                break;
              case GEOM_MULTI_POINT:
                current_bitset = shumate_vector_index_get_bitset_geometry_type (scope->index, scope->source_layer_idx, SHUMATE_GEOMETRY_TYPE_MULTIPOINT);
                break;
              default:
                current_bitset = NULL;
                break;
            }
          if (current_bitset != NULL)
            shumate_vector_index_bitset_or (bitset, current_bitset);

          switch (self->fast_geometry_type & GEOM_ANY_LINESTRING)
            {
              case GEOM_ANY_LINESTRING:
                current_bitset = shumate_vector_index_get_bitset_broad_geometry_type (scope->index, scope->source_layer_idx, SHUMATE_GEOMETRY_TYPE_LINESTRING);
                break;
              case GEOM_SINGLE_LINESTRING:
                current_bitset = shumate_vector_index_get_bitset_geometry_type (scope->index, scope->source_layer_idx, SHUMATE_GEOMETRY_TYPE_LINESTRING);
                break;
              case GEOM_MULTI_LINESTRING:
                current_bitset = shumate_vector_index_get_bitset_geometry_type (scope->index, scope->source_layer_idx, SHUMATE_GEOMETRY_TYPE_MULTILINESTRING);
                break;
              default:
                current_bitset = NULL;
                break;
            }
          if (current_bitset != NULL)
            shumate_vector_index_bitset_or (bitset, current_bitset);

          switch (self->fast_geometry_type & GEOM_ANY_POLYGON)
            {
              case GEOM_ANY_POLYGON:
                current_bitset = shumate_vector_index_get_bitset_broad_geometry_type (scope->index, scope->source_layer_idx, SHUMATE_GEOMETRY_TYPE_POLYGON);
                break;
              case GEOM_SINGLE_POLYGON:
                current_bitset = shumate_vector_index_get_bitset_geometry_type (scope->index, scope->source_layer_idx, SHUMATE_GEOMETRY_TYPE_POLYGON);
                break;
              case GEOM_MULTI_POLYGON:
                current_bitset = shumate_vector_index_get_bitset_geometry_type (scope->index, scope->source_layer_idx, SHUMATE_GEOMETRY_TYPE_MULTIPOLYGON);
                break;
              default:
                current_bitset = NULL;
                break;
            }
          if (current_bitset != NULL)
            shumate_vector_index_bitset_or (bitset, current_bitset);

          return bitset;
        }

      case EXPR_NOT:
        bitset = shumate_vector_expression_filter_eval_bitset (g_ptr_array_index (self->expressions, 0), scope, mask);
        shumate_vector_index_bitset_not (bitset);
        return bitset;

      case EXPR_ALL:
        for (int i = 0; i < self->expressions->len; i ++)
          {
            ShumateVectorIndexBitset *current_bitset = shumate_vector_expression_filter_eval_bitset (g_ptr_array_index (self->expressions, i), scope, bitset);
            if (i == 0)
              bitset = current_bitset;
            else
              {
                shumate_vector_index_bitset_and (bitset, current_bitset);
                shumate_vector_index_bitset_free (current_bitset);
              }

            if (bitset == NULL)
              return NULL;
          }
        return bitset;

      case EXPR_ANY:
      case EXPR_NONE:
        for (int i = 0; i < self->expressions->len; i ++)
          {
            ShumateVectorIndexBitset *current_bitset;
            current_bitset = shumate_vector_expression_filter_eval_bitset (g_ptr_array_index (self->expressions, i), scope, bitset);
            /* Invert the result bitset so we can pass it to the mask parameter of eval_bitset(), saving some work
               if there are any non-bitset-optimized expressions */
            shumate_vector_index_bitset_not (current_bitset);
            if (i == 0)
              bitset = current_bitset;
            else
              {
                shumate_vector_index_bitset_and (bitset, current_bitset);
                shumate_vector_index_bitset_free (current_bitset);
              }
          }

        if (self->type == EXPR_ANY)
          shumate_vector_index_bitset_not (bitset);

        return bitset;

      default:
        return SHUMATE_VECTOR_EXPRESSION_CLASS (shumate_vector_expression_filter_parent_class)->eval_bitset (expr, scope, mask);
    }
}

static void
shumate_vector_expression_filter_collect_indexes (ShumateVectorExpression       *expr,
                                                  const char                    *layer_name,
                                                  ShumateVectorIndexDescription *index_description)
{
  ShumateVectorExpressionFilter *self = (ShumateVectorExpressionFilter *)expr;

  switch (self->type)
    {
      case EXPR_FAST_EQ:
      case EXPR_FAST_NE:
        shumate_vector_index_description_add (index_description, layer_name, self->fast_eq.key, &self->fast_eq.value);
        break;

      case EXPR_FAST_IN:
      case EXPR_FAST_NOT_IN:
        {
          GHashTableIter iter;
          ShumateVectorValue *value;
          g_hash_table_iter_init (&iter, self->fast_in.haystack);
          while (g_hash_table_iter_next (&iter, NULL, (gpointer *)&value))
            shumate_vector_index_description_add (index_description, layer_name, self->fast_in.key, value);
          break;
        }

      case EXPR_FAST_HAS:
      case EXPR_FAST_NOT_HAS:
        shumate_vector_index_description_add_has_index (index_description, layer_name, self->fast_get_key);
        break;

      case EXPR_FAST_GEOMETRY_TYPE:
      case EXPR_FAST_NOT_GEOMETRY_TYPE:
        switch (self->fast_geometry_type & GEOM_ANY_POINT)
          {
            case GEOM_ANY_POINT:
              shumate_vector_index_description_add_broad_geometry_type (index_description, layer_name);
              break;
            case GEOM_SINGLE_POINT:
              shumate_vector_index_description_add_geometry_type (index_description, layer_name);
              break;
            case GEOM_MULTI_POINT:
              shumate_vector_index_description_add_geometry_type (index_description, layer_name);
              break;
          }

        switch (self->fast_geometry_type & GEOM_ANY_LINESTRING)
          {
            case GEOM_ANY_LINESTRING:
              shumate_vector_index_description_add_broad_geometry_type (index_description, layer_name);
              break;
            case GEOM_SINGLE_LINESTRING:
              shumate_vector_index_description_add_geometry_type (index_description, layer_name);
              break;
            case GEOM_MULTI_LINESTRING:
              shumate_vector_index_description_add_geometry_type (index_description, layer_name);
              break;
          }

        switch (self->fast_geometry_type & GEOM_ANY_POLYGON)
          {
            case GEOM_ANY_POLYGON:
              shumate_vector_index_description_add_broad_geometry_type (index_description, layer_name);
              break;
            case GEOM_SINGLE_POLYGON:
              shumate_vector_index_description_add_geometry_type (index_description, layer_name);
              break;
            case GEOM_MULTI_POLYGON:
              shumate_vector_index_description_add_geometry_type (index_description, layer_name);
              break;
          }
        break;

      case EXPR_ANY:
      case EXPR_ALL:
      case EXPR_NONE:
      case EXPR_NOT:
        for (int i = 0; i < self->expressions->len; i ++)
          shumate_vector_expression_collect_indexes (g_ptr_array_index (self->expressions, i), layer_name, index_description);
        break;

      default:
        break;
    }
}

static void
shumate_vector_expression_filter_class_init (ShumateVectorExpressionFilterClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ShumateVectorExpressionClass *expr_class = SHUMATE_VECTOR_EXPRESSION_CLASS (klass);

  object_class->finalize = shumate_vector_expression_filter_finalize;
  expr_class->eval = shumate_vector_expression_filter_eval;
  expr_class->eval_bitset = shumate_vector_expression_filter_eval_bitset;
  expr_class->collect_indexes = shumate_vector_expression_filter_collect_indexes;
}


static void
shumate_vector_expression_filter_init (ShumateVectorExpressionFilter *self)
{
}
