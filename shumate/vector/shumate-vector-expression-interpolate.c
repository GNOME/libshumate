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
#include "shumate-vector-expression-interpolate-private.h"
#include "shumate-vector-expression-filter-private.h"
#include "shumate-vector-utils-private.h"

typedef struct {
  double point;
  ShumateVectorExpression *expr;
} Stop;

typedef enum {
  STEP,
  LINEAR,
  EXPONENTIAL,
} InterpolationType;

struct _ShumateVectorExpressionInterpolate
{
  ShumateVectorExpression parent_instance;

  ShumateVectorExpression *input;
  InterpolationType interpolation;
  double base;
  GPtrArray *stops;
};

G_DEFINE_TYPE (ShumateVectorExpressionInterpolate, shumate_vector_expression_interpolate, SHUMATE_TYPE_VECTOR_EXPRESSION)


ShumateVectorExpression *
shumate_vector_expression_interpolate_from_json_obj (JsonObject *object, GError **error)
{
  g_autoptr(ShumateVectorExpressionInterpolate) self = g_object_new (SHUMATE_TYPE_VECTOR_EXPRESSION_INTERPOLATE, NULL);
  JsonNode *stops_node;

  self->interpolation = EXPONENTIAL;
  self->base = json_object_get_double_member_with_default (object, "base", 1.0);

  if ((stops_node = json_object_get_member (object, "stops")))
    {
      JsonArray *stops;

      if (!shumate_vector_json_get_array (stops_node, &stops, error))
        return NULL;

      for (int i = 0, n = json_array_get_length (stops); i < n; i ++)
        {
          JsonNode *stop_node = json_array_get_element (stops, i);
          JsonArray *stop_array;
          Stop *stop;
          JsonNode *point_node;
          JsonNode *value_node;
          g_auto(GValue) gvalue = G_VALUE_INIT;
          g_auto(ShumateVectorValue) value = SHUMATE_VECTOR_VALUE_INIT;

          if (!shumate_vector_json_get_array (stop_node, &stop_array, error))
            return NULL;

          if (json_array_get_length (stop_array) != 2)
            {
              g_set_error (error,
                           SHUMATE_STYLE_ERROR,
                           SHUMATE_STYLE_ERROR_INVALID_EXPRESSION,
                           "Expected element of \"stops\" to have exactly 2 elements");
              return NULL;
            }

          point_node = json_array_get_element (stop_array, 0);
          value_node = json_array_get_element (stop_array, 1);

          if (!JSON_NODE_HOLDS_VALUE (point_node)
              || !g_value_type_transformable (json_node_get_value_type (point_node), G_TYPE_DOUBLE))
            {
              g_set_error (error,
                           SHUMATE_STYLE_ERROR,
                           SHUMATE_STYLE_ERROR_INVALID_EXPRESSION,
                           "Expected element 1 of \"stops\" to be a number");
              return NULL;
            }

          if (!JSON_NODE_HOLDS_VALUE (value_node))
            {
              g_set_error (error,
                           SHUMATE_STYLE_ERROR,
                           SHUMATE_STYLE_ERROR_INVALID_EXPRESSION,
                           "Expected element 2 of \"stops\" to be a literal value");
              return NULL;
            }

          json_node_get_value (value_node, &gvalue);

          if (!shumate_vector_value_set_from_g_value (&value, &gvalue))
            {
              g_set_error (error,
                           SHUMATE_STYLE_ERROR,
                           SHUMATE_STYLE_ERROR_INVALID_EXPRESSION,
                           "Could not parse literal value");
              return NULL;
            }

          stop = g_new0 (Stop, 1);
          stop->point = json_node_get_double (point_node);
          stop->expr = shumate_vector_expression_filter_from_literal (&value);

          g_ptr_array_add (self->stops, stop);
        }
    }

  return (ShumateVectorExpression *)g_steal_pointer (&self);
}

static gboolean
add_stops (ShumateVectorExpressionInterpolate  *self,
           JsonArray                           *array,
           int                                  start,
           ShumateVectorExpressionContext      *ctx,
           GError                             **error)
{
  double prev_point;

  /* Stops */
  for (int i = start, n = json_array_get_length (array); i < n; i += 2)
    {
      g_auto(ShumateVectorValue) value = SHUMATE_VECTOR_VALUE_INIT;
      g_autoptr(ShumateVectorExpression) expr = NULL;
      Stop *stop;
      double point;

      if (!shumate_vector_value_set_from_json_literal (&value, json_array_get_element (array, i), error))
        return FALSE;

      if (!shumate_vector_value_get_number (&value, &point))
        {
          g_set_error (error,
                       SHUMATE_STYLE_ERROR,
                       SHUMATE_STYLE_ERROR_INVALID_EXPRESSION,
                       "Expected stop input to be a number");
          return FALSE;
        }

      if (i > start && point <= prev_point)
        {
          g_set_error (error,
                       SHUMATE_STYLE_ERROR,
                       SHUMATE_STYLE_ERROR_INVALID_EXPRESSION,
                       "Stop inputs must be in strictly ascending order");
          return FALSE;
        }
      prev_point = point;

      expr = shumate_vector_expression_filter_from_array_or_literal (
        json_array_get_element (array, i + 1),
        ctx,
        error
      );
      if (!expr)
        return FALSE;

      stop = g_new0 (Stop, 1);
      stop->point = point;
      stop->expr = g_steal_pointer (&expr);
      g_ptr_array_add (self->stops, stop);
    }

  return TRUE;
}

ShumateVectorExpression *
shumate_vector_expression_interpolate_from_json_array (JsonArray                       *array,
                                                       ShumateVectorExpressionContext  *ctx,
                                                       GError                         **error)
{
  g_autoptr(ShumateVectorExpressionInterpolate) self = g_object_new (SHUMATE_TYPE_VECTOR_EXPRESSION_INTERPOLATE, NULL);
  JsonArray *interpolation;
  const char *interpolation_type;

  if (json_array_get_length (array) < 5)
    {
      g_set_error (error,
                   SHUMATE_STYLE_ERROR,
                   SHUMATE_STYLE_ERROR_INVALID_EXPRESSION,
                   "Operator `interpolate` expected at least 4 arguments");
      return NULL;
    }

  if (json_array_get_length (array) % 2 == 0)
    {
      g_set_error (error,
                   SHUMATE_STYLE_ERROR,
                   SHUMATE_STYLE_ERROR_INVALID_EXPRESSION,
                   "Operator `interpolate` expected an even number of arguments");
      return NULL;
    }

  /* Interpolation type */
  if (!shumate_vector_json_get_array (json_array_get_element (array, 1), &interpolation, error))
    return NULL;

  if (json_array_get_length (interpolation) == 0)
    {
      g_set_error (error,
                   SHUMATE_STYLE_ERROR,
                   SHUMATE_STYLE_ERROR_INVALID_EXPRESSION,
                   "Expected an interpolation type");
      return NULL;
    }

  if (!shumate_vector_json_get_string (json_array_get_element (interpolation, 0), &interpolation_type, error))
    return NULL;

  if (g_strcmp0 ("linear", interpolation_type) == 0)
    {
      if (json_array_get_length (interpolation) != 1)
        {
          g_set_error (error,
                       SHUMATE_STYLE_ERROR,
                       SHUMATE_STYLE_ERROR_INVALID_EXPRESSION,
                       "Interpolation type `linear` expected 0 arguments");
          return NULL;
        }

      self->interpolation = LINEAR;
    }
  else if (g_strcmp0 ("exponential", interpolation_type) == 0)
    {
      g_auto(ShumateVectorValue) value = SHUMATE_VECTOR_VALUE_INIT;

      if (json_array_get_length (interpolation) != 2)
        {
          g_set_error (error,
                       SHUMATE_STYLE_ERROR,
                       SHUMATE_STYLE_ERROR_INVALID_EXPRESSION,
                       "Interpolation type `exponential` expected 1 argument");
          return NULL;
        }

      if (!shumate_vector_value_set_from_json_literal (&value, json_array_get_element (interpolation, 1), error))
        return NULL;

      self->interpolation = EXPONENTIAL;
      if (!shumate_vector_value_get_number (&value, &self->base))
        {
          g_set_error (error,
                       SHUMATE_STYLE_ERROR,
                       SHUMATE_STYLE_ERROR_INVALID_EXPRESSION,
                       "Expected argument of `exponential` to be a number");
          return NULL;
        }
    }
  else
    {
      g_set_error (error,
                   SHUMATE_STYLE_ERROR,
                   SHUMATE_STYLE_ERROR_INVALID_EXPRESSION,
                   "Unknown interpolation type `%s`",
                   interpolation_type);
    }

  /* Input */
  self->input = shumate_vector_expression_filter_from_array_or_literal (
    json_array_get_element (array, 2),
    ctx,
    error
  );
  if (!self->input)
    return NULL;

  /* Stops */
  if (!add_stops (self, array, 3, ctx, error))
    return NULL;

  return (ShumateVectorExpression *)g_steal_pointer (&self);
}

ShumateVectorExpression *
shumate_vector_expression_step_from_json_array (JsonArray                       *array,
                                                ShumateVectorExpressionContext  *ctx,
                                                GError                         **error)
{
  g_autoptr(ShumateVectorExpressionInterpolate) self = g_object_new (SHUMATE_TYPE_VECTOR_EXPRESSION_INTERPOLATE, NULL);
  Stop *stop;

  self->interpolation = STEP;

  if (json_array_get_length (array) < 5)
    {
      g_set_error (error,
                   SHUMATE_STYLE_ERROR,
                   SHUMATE_STYLE_ERROR_INVALID_EXPRESSION,
                   "Operator `interpolate` expected at least 4 arguments");
      return NULL;
    }

  if (json_array_get_length (array) % 2 == 0)
    {
      g_set_error (error,
                   SHUMATE_STYLE_ERROR,
                   SHUMATE_STYLE_ERROR_INVALID_EXPRESSION,
                   "Operator `interpolate` expected an even number of arguments");
      return NULL;
    }

  /* Input */
  self->input = shumate_vector_expression_filter_from_array_or_literal (
    json_array_get_element (array, 1),
    ctx,
    error
  );
  if (!self->input)
    return NULL;

  /* First stop */
  stop = g_new0 (Stop, 1);
  g_ptr_array_add (self->stops, stop);
  stop->point = -G_MAXDOUBLE;
  stop->expr = shumate_vector_expression_filter_from_array_or_literal (
    json_array_get_element (array, 2),
    ctx,
    error
  );
  if (!stop->expr)
    return NULL;

  /* Stops */
  if (!add_stops (self, array, 3, ctx, error))
    return NULL;

  return (ShumateVectorExpression *)g_steal_pointer (&self);
}


static void
stop_free (Stop *stop)
{
  g_clear_object (&stop->expr);
  g_free (stop);
}


static double
lerp_double (double a, double b, double pos)
{
  return (b - a) * pos + a;
}


static void
lerp (ShumateVectorValue *last_value, ShumateVectorValue *next_value, double pos, ShumateVectorValue *out)
{
  gdouble last_double, next_double;
  GdkRGBA last_color, next_color;

  if (shumate_vector_value_get_number (last_value, &last_double)
      && shumate_vector_value_get_number (next_value, &next_double))
    shumate_vector_value_set_number (out, lerp_double (last_double, next_double, pos));
  else if (shumate_vector_value_get_color (last_value, &last_color)
      && shumate_vector_value_get_color (next_value, &next_color))
    {
      GdkRGBA color = {
        .red = lerp_double (last_color.red, next_color.red, pos),
        .green = lerp_double (last_color.green, next_color.green, pos),
        .blue = lerp_double (last_color.blue, next_color.blue, pos),
        .alpha = lerp_double (last_color.alpha, next_color.alpha, pos),
      };
      shumate_vector_value_set_color (out, &color);
    }
  else
    shumate_vector_value_steal (last_value, out);
}


static void
exp_interp (double last_point,
            double next_point,
            ShumateVectorValue *last_value,
            ShumateVectorValue *next_value,
            double input,
            double base,
            ShumateVectorValue *dest)
{
  double diff = next_point - last_point;
  double pos = input - last_point;

  lerp (last_value, next_value, (pow (base, pos) - 1.0) / (pow (base, diff) - 1.0), dest);
}


static void
shumate_vector_expression_interpolate_finalize (GObject *object)
{
  ShumateVectorExpressionInterpolate *self = (ShumateVectorExpressionInterpolate *)object;

  g_clear_object (&self->input);
  g_clear_pointer (&self->stops, g_ptr_array_unref);

  G_OBJECT_CLASS (shumate_vector_expression_interpolate_parent_class)->finalize (object);
}


static gboolean
shumate_vector_expression_interpolate_eval (ShumateVectorExpression  *expr,
                                            ShumateVectorRenderScope *scope,
                                            ShumateVectorValue       *out)
{
  ShumateVectorExpressionInterpolate *self = (ShumateVectorExpressionInterpolate *)expr;
  g_auto(ShumateVectorValue) value = SHUMATE_VECTOR_VALUE_INIT;
  double input;
  Stop **stops = (Stop **)self->stops->pdata;
  guint n_stops = self->stops->len;

  if (n_stops == 0)
    return FALSE;

  if (self->input != NULL)
    {
      if (!shumate_vector_expression_eval (self->input, scope, &value))
        return FALSE;
      if (!shumate_vector_value_get_number (&value, &input))
        return FALSE;
    }
  else
    input = scope->zoom_level;

  if (input < stops[0]->point)
    return shumate_vector_expression_eval (stops[0]->expr, scope, out);

  for (int i = 1; i < n_stops; i ++)
    {
      Stop *last = stops[i - 1];
      Stop *next = stops[i];
      if (last->point <= input && input < next->point)
        {
          double pos_norm = (input - last->point) / (next->point - last->point);
          g_auto(ShumateVectorValue) last_value = SHUMATE_VECTOR_VALUE_INIT;
          g_auto(ShumateVectorValue) next_value = SHUMATE_VECTOR_VALUE_INIT;

          if (!shumate_vector_expression_eval (last->expr, scope, &last_value))
            return FALSE;
          if (!shumate_vector_expression_eval (next->expr, scope, &next_value))
            return FALSE;

          switch (self->interpolation)
            {
              case STEP:
                shumate_vector_value_steal (&last_value, out);
                return TRUE;
              case LINEAR:
                lerp (&last_value, &next_value, pos_norm, out);
                return TRUE;
              case EXPONENTIAL:
                if (self->base == 1.0)
                  lerp (&last_value, &next_value, pos_norm, out);
                else
                  exp_interp (last->point, next->point, &last_value, &next_value, input, self->base, out);
                return TRUE;
              default:
                g_assert_not_reached ();
            }
        }
    }

  return shumate_vector_expression_eval (stops[n_stops - 1]->expr, scope, out);
}


static void
shumate_vector_expression_interpolate_class_init (ShumateVectorExpressionInterpolateClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ShumateVectorExpressionClass *expr_class = SHUMATE_VECTOR_EXPRESSION_CLASS (klass);

  object_class->finalize = shumate_vector_expression_interpolate_finalize;
  expr_class->eval = shumate_vector_expression_interpolate_eval;
}

static void
shumate_vector_expression_interpolate_init (ShumateVectorExpressionInterpolate *self)
{
  self->stops = g_ptr_array_new_with_free_func ((GDestroyNotify) stop_free);
}
