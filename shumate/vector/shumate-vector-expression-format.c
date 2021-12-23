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
#include "shumate-vector-expression-format-private.h"
#include "shumate-vector-value-private.h"

struct _ShumateVectorExpressionFormat
{
  ShumateVectorExpression parent_instance;

  /* A list of format segments. Even-indexed segments are literals, every
   * other segment is a variable. */
  char **format;
};

G_DEFINE_TYPE (ShumateVectorExpressionFormat, shumate_vector_expression_format, SHUMATE_TYPE_VECTOR_EXPRESSION)


ShumateVectorExpression *
shumate_vector_expression_format_new (const char *format, GError **error)
{
  g_autoptr(ShumateVectorExpressionFormat) self = g_object_new (SHUMATE_TYPE_VECTOR_EXPRESSION_FORMAT, NULL);
  int balance = 0;

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

  self->format = g_strsplit_set (format, "{}", 0);

  return (ShumateVectorExpression *)g_steal_pointer (&self);
}

static void
shumate_vector_expression_format_finalize (GObject *object)
{
  ShumateVectorExpressionFormat *self = (ShumateVectorExpressionFormat *)object;

  g_clear_pointer (&self->format, g_strfreev);

  G_OBJECT_CLASS (shumate_vector_expression_format_parent_class)->finalize (object);
}


static gboolean
shumate_vector_expression_format_eval (ShumateVectorExpression  *expr,
                                       ShumateVectorRenderScope *scope,
                                       ShumateVectorValue       *out)
{
  ShumateVectorExpressionFormat *self = (ShumateVectorExpressionFormat *)expr;
  g_autoptr(GString) string = g_string_new ("");
  ShumateVectorValue value = SHUMATE_VECTOR_VALUE_INIT;
  char *variable_val;
  int i = 0;

  while (TRUE)
    {
      if (self->format[i] == NULL)
        break;

      g_string_append (string, self->format[i]);
      i ++;

      if (self->format[i] == NULL)
        break;

      shumate_vector_render_scope_get_variable (scope, self->format[i], &value);
      variable_val = shumate_vector_value_as_string (&value);
      g_string_append (string, variable_val);
      g_free (variable_val);
      shumate_vector_value_unset (&value);
      i ++;
    }

  shumate_vector_value_set_string (out, string->str);
  return TRUE;
}


static void
shumate_vector_expression_format_class_init (ShumateVectorExpressionFormatClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ShumateVectorExpressionClass *expr_class = SHUMATE_VECTOR_EXPRESSION_CLASS (klass);

  object_class->finalize = shumate_vector_expression_format_finalize;
  expr_class->eval = shumate_vector_expression_format_eval;
}

static void
shumate_vector_expression_format_init (ShumateVectorExpressionFormat *self)
{
}
