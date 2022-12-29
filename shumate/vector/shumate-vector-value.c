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
#include "shumate-vector-value-private.h"

enum {
  TYPE_NULL,
  TYPE_NUMBER,
  TYPE_BOOLEAN,
  TYPE_STRING,
  TYPE_COLOR,
  TYPE_ARRAY,
};

enum {
  COLOR_UNSET,
  COLOR_SET,
  COLOR_INVALID,
};

gboolean
shumate_vector_value_set_from_json_literal (ShumateVectorValue *self, JsonNode *node, GError **error)
{
  if (JSON_NODE_HOLDS_NULL (node))
    {
      shumate_vector_value_unset (self);
      return TRUE;
    }
  else if (JSON_NODE_HOLDS_VALUE (node))
    {
      g_auto(GValue) gvalue = G_VALUE_INIT;

      json_node_get_value (node, &gvalue);
      if (!shumate_vector_value_set_from_g_value (self, &gvalue))
        {
          g_set_error (error,
                      SHUMATE_STYLE_ERROR,
                      SHUMATE_STYLE_ERROR_INVALID_EXPRESSION,
                      "Unsupported literal value in expression");
          return FALSE;
        }

      return TRUE;
    }
  else if (JSON_NODE_HOLDS_ARRAY (node))
    {
      g_auto(ShumateVectorValue) value = SHUMATE_VECTOR_VALUE_INIT;
      JsonArray *array = json_node_get_array (node);

      shumate_vector_value_start_array (self);

      for (int i = 0, n = json_array_get_length (array); i < n; i ++)
        {
          if (!shumate_vector_value_set_from_json_literal (&value, json_array_get_element (array, i), error))
            return FALSE;

          shumate_vector_value_array_append (self, &value);
        }

      return TRUE;
    }
  else if (JSON_NODE_HOLDS_OBJECT (node))
    {
      g_set_error (error,
                   SHUMATE_STYLE_ERROR,
                   SHUMATE_STYLE_ERROR_UNSUPPORTED,
                   "Object literals are not supported");
      return FALSE;
    }
  else
    {
      g_warn_if_reached ();
      return FALSE;
    }
}

gboolean
shumate_vector_value_set_from_g_value (ShumateVectorValue *self, const GValue *value)
{
  g_auto(GValue) tmp = G_VALUE_INIT;

  if (value == NULL)
    shumate_vector_value_unset (self);
  else if (g_value_type_transformable (G_VALUE_TYPE (value), G_TYPE_DOUBLE))
    {
      g_value_init (&tmp, G_TYPE_DOUBLE);
      g_value_transform (value, &tmp);
      shumate_vector_value_set_number (self, g_value_get_double (&tmp));
    }
  else if (g_value_type_transformable (G_VALUE_TYPE (value), G_TYPE_BOOLEAN))
    {
      g_value_init (&tmp, G_TYPE_BOOLEAN);
      g_value_transform (value, &tmp);
      shumate_vector_value_set_boolean (self, g_value_get_boolean (&tmp));
    }
  else if (g_value_type_transformable (G_VALUE_TYPE (value), G_TYPE_STRING))
    {
      g_value_init (&tmp, G_TYPE_STRING);
      g_value_transform (value, &tmp);
      shumate_vector_value_set_string (self, g_value_get_string (&tmp));
    }
  else
    return FALSE;

  return TRUE;
}


void
shumate_vector_value_set_from_feature_value (ShumateVectorValue *self, VectorTile__Tile__Value *value)
{
  if (value->has_int_value)
    shumate_vector_value_set_number (self, value->int_value);
  else if (value->has_uint_value)
    shumate_vector_value_set_number (self, value->uint_value);
  else if (value->has_sint_value)
    shumate_vector_value_set_number (self, value->sint_value);
  else if (value->has_float_value)
    shumate_vector_value_set_number (self, value->float_value);
  else if (value->has_double_value)
    shumate_vector_value_set_number (self, value->double_value);
  else if (value->has_bool_value)
    shumate_vector_value_set_boolean (self, value->bool_value);
  else if (value->string_value != NULL)
    shumate_vector_value_set_string (self, value->string_value);
  else
    shumate_vector_value_unset (self);
}


void
shumate_vector_value_free (ShumateVectorValue *self)
{
  shumate_vector_value_unset (self);
  g_free (self);
}


void
shumate_vector_value_unset (ShumateVectorValue *self)
{
  if (self->type == TYPE_STRING)
    g_clear_pointer (&self->string, g_free);
  else if (self->type == TYPE_ARRAY)
    g_clear_pointer (&self->array, g_ptr_array_unref);
  self->type = TYPE_NULL;
}


gboolean
shumate_vector_value_is_null (ShumateVectorValue *self)
{
  return self->type == TYPE_NULL;
}


void
shumate_vector_value_copy (ShumateVectorValue *self, ShumateVectorValue *out)
{
  shumate_vector_value_unset (out);
  *out = *self;

  if (self->type == TYPE_STRING)
    out->string = g_strdup (out->string);
  else if (self->type == TYPE_ARRAY)
    out->array = g_ptr_array_ref (out->array);
}


void
shumate_vector_value_set_number (ShumateVectorValue *self, double number)
{
  shumate_vector_value_unset (self);
  self->type = TYPE_NUMBER;
  self->number = number;
}


gboolean
shumate_vector_value_get_number (ShumateVectorValue *self, double *number)
{
  if (self->type != TYPE_NUMBER)
    return FALSE;

  *number = self->number;
  return TRUE;
}


void
shumate_vector_value_set_boolean (ShumateVectorValue *self, gboolean boolean)
{
  shumate_vector_value_unset (self);
  self->type = TYPE_BOOLEAN;
  self->boolean = boolean;
}


gboolean
shumate_vector_value_get_boolean (ShumateVectorValue *self, gboolean *boolean)
{
  if (self->type != TYPE_BOOLEAN)
    return FALSE;

  *boolean = self->boolean;
  return TRUE;
}



void
shumate_vector_value_set_string (ShumateVectorValue *self, const char *string)
{
  shumate_vector_value_unset (self);
  self->type = TYPE_STRING;
  self->string = g_strdup (string);
  self->color_state = COLOR_UNSET;
}


gboolean
shumate_vector_value_get_string (ShumateVectorValue *self, const char **string)
{
  if (self->type != TYPE_STRING)
    return FALSE;

  *string = self->string;
  return TRUE;
}


void
shumate_vector_value_set_color (ShumateVectorValue *self, GdkRGBA *color)
{
  shumate_vector_value_unset (self);
  self->type = TYPE_COLOR;
  self->color = *color;
}


gboolean
shumate_vector_value_get_color (ShumateVectorValue *self, GdkRGBA *color)
{
  if (self->type == TYPE_STRING)
    {
      if (self->color_state == COLOR_UNSET)
        {
          if (gdk_rgba_parse (&self->color, self->string))
            self->color_state = COLOR_SET;
          else
            self->color_state = COLOR_INVALID;
        }

      if (self->color_state == COLOR_SET)
        {
          *color = self->color;
          return TRUE;
        }
      else
        return FALSE;
    }

  if (self->type != TYPE_COLOR)
    return FALSE;

  *color = self->color;
  return TRUE;
}


void
shumate_vector_value_start_array (ShumateVectorValue *self)
{
  shumate_vector_value_unset (self);
  self->type = TYPE_ARRAY;
  self->array = g_ptr_array_new_with_free_func ((GDestroyNotify)shumate_vector_value_free);
}


void
shumate_vector_value_array_append (ShumateVectorValue *self, ShumateVectorValue *element)
{
  g_autoptr(ShumateVectorValue) copy = g_new0 (ShumateVectorValue, 1);

  g_return_if_fail (self->type == TYPE_ARRAY);

  shumate_vector_value_copy (element, copy);
  g_ptr_array_add (self->array, g_steal_pointer (&copy));
}


gboolean
shumate_vector_value_array_contains (ShumateVectorValue *self, ShumateVectorValue *element)
{
  if (self->type != TYPE_ARRAY)
    return FALSE;

  for (int i = 0, n = self->array->len; i < n; i ++)
    if (shumate_vector_value_equal (element, g_ptr_array_index (self->array, i)))
      return TRUE;

  return FALSE;
}

gboolean
shumate_vector_value_equal (ShumateVectorValue *a, ShumateVectorValue *b)
{
  if (a->type != b->type)
    return FALSE;

  switch (a->type)
    {
    case TYPE_NULL:
      return TRUE;
    case TYPE_NUMBER:
      return a->number == b->number;
    case TYPE_BOOLEAN:
      return a->boolean == b->boolean;
    case TYPE_STRING:
      return g_strcmp0 (a->string, b->string) == 0;
    case TYPE_COLOR:
      return gdk_rgba_equal (&a->color, &b->color);
    case TYPE_ARRAY:
      if (a->array->len != b->array->len)
        return FALSE;

      for (int i = 0; i < a->array->len; i ++)
        if (!shumate_vector_value_equal (g_ptr_array_index (a->array, i), g_ptr_array_index (b->array, i)))
          return FALSE;

      return TRUE;
    default:
      g_assert_not_reached ();
    }
}


char *
shumate_vector_value_as_string (ShumateVectorValue *self)
{
  switch (self->type)
    {
    case TYPE_NULL:
      return g_strdup ("");
    case TYPE_NUMBER:
      return g_strdup_printf ("%f", self->number);
    case TYPE_BOOLEAN:
      return g_strdup (self->boolean ? "true" : "false");
    case TYPE_STRING:
      return g_strdup (self->string);
    case TYPE_COLOR:
      return gdk_rgba_to_string (&self->color);
    case TYPE_ARRAY:
      {
        g_autofree char *result = NULL;
        g_autoptr(GStrvBuilder) builder = g_strv_builder_new ();
        g_auto(GStrv) strv = NULL;

        for (int i = 0; i < self->array->len; i ++)
          {
            g_autofree char *string = shumate_vector_value_as_string (g_ptr_array_index (self->array, i));
            g_strv_builder_add (builder, string);
          }

        strv = g_strv_builder_end (builder);
        result = g_strjoinv (", ", strv);
        return g_strconcat ("[", result, "]", NULL);
      }
    default:
      g_assert_not_reached ();
    }
}
