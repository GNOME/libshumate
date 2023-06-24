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
  if (self->type == SHUMATE_VECTOR_VALUE_TYPE_STRING)
    g_clear_pointer (&self->string, g_free);
  else if (self->type == SHUMATE_VECTOR_VALUE_TYPE_ARRAY)
    g_clear_pointer (&self->array, g_ptr_array_unref);
  else if (self->type == SHUMATE_VECTOR_VALUE_TYPE_RESOLVED_IMAGE)
    {
      g_clear_object (&self->image);
      g_clear_pointer (&self->image_name, g_free);
    }
  else if (self->type == SHUMATE_VECTOR_VALUE_TYPE_FORMATTED_STRING)
    g_clear_pointer (&self->formatted_string, g_ptr_array_unref);

  self->type = SHUMATE_VECTOR_VALUE_TYPE_NULL;
}


gboolean
shumate_vector_value_is_null (ShumateVectorValue *self)
{
  return self->type == SHUMATE_VECTOR_VALUE_TYPE_NULL;
}


void
shumate_vector_value_copy (ShumateVectorValue *self, ShumateVectorValue *out)
{
  shumate_vector_value_unset (out);
  *out = *self;

  if (self->type == SHUMATE_VECTOR_VALUE_TYPE_STRING)
    out->string = g_strdup (out->string);
  else if (self->type == SHUMATE_VECTOR_VALUE_TYPE_ARRAY)
    out->array = g_ptr_array_ref (out->array);
  else if (self->type == SHUMATE_VECTOR_VALUE_TYPE_RESOLVED_IMAGE)
    {
      out->image = g_object_ref (out->image);
      out->image_name = g_strdup (out->image_name);
    }
  else if (self->type == SHUMATE_VECTOR_VALUE_TYPE_FORMATTED_STRING)
    out->formatted_string = g_ptr_array_ref (out->formatted_string);
}


void
shumate_vector_value_set_number (ShumateVectorValue *self, double number)
{
  shumate_vector_value_unset (self);
  self->type = SHUMATE_VECTOR_VALUE_TYPE_NUMBER;
  self->number = number;
}


gboolean
shumate_vector_value_get_number (ShumateVectorValue *self, double *number)
{
  if (self->type != SHUMATE_VECTOR_VALUE_TYPE_NUMBER)
    return FALSE;

  *number = self->number;
  return TRUE;
}


void
shumate_vector_value_set_boolean (ShumateVectorValue *self, gboolean boolean)
{
  shumate_vector_value_unset (self);
  self->type = SHUMATE_VECTOR_VALUE_TYPE_BOOLEAN;
  self->boolean = boolean;
}


gboolean
shumate_vector_value_get_boolean (ShumateVectorValue *self, gboolean *boolean)
{
  if (self->type != SHUMATE_VECTOR_VALUE_TYPE_BOOLEAN)
    return FALSE;

  *boolean = self->boolean;
  return TRUE;
}


void
shumate_vector_value_set_string (ShumateVectorValue *self, const char *string)
{
  shumate_vector_value_unset (self);
  self->type = SHUMATE_VECTOR_VALUE_TYPE_STRING;
  self->string = g_strdup (string);
  self->color_state = COLOR_UNSET;
}


gboolean
shumate_vector_value_get_string (ShumateVectorValue *self, const char **string)
{
  if (self->type != SHUMATE_VECTOR_VALUE_TYPE_STRING)
    return FALSE;

  *string = self->string;
  return TRUE;
}


void
shumate_vector_value_set_color (ShumateVectorValue *self, GdkRGBA *color)
{
  shumate_vector_value_unset (self);
  self->type = SHUMATE_VECTOR_VALUE_TYPE_COLOR;
  self->color = *color;
}


gboolean
shumate_vector_value_get_color (ShumateVectorValue *self, GdkRGBA *color)
{
  if (self->type == SHUMATE_VECTOR_VALUE_TYPE_STRING)
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

  if (self->type != SHUMATE_VECTOR_VALUE_TYPE_COLOR)
    return FALSE;

  *color = self->color;
  return TRUE;
}


void
shumate_vector_value_start_array (ShumateVectorValue *self)
{
  shumate_vector_value_unset (self);
  self->type = SHUMATE_VECTOR_VALUE_TYPE_ARRAY;
  self->array = g_ptr_array_new_with_free_func ((GDestroyNotify)shumate_vector_value_free);
}


void
shumate_vector_value_array_append (ShumateVectorValue *self, ShumateVectorValue *element)
{
  g_autoptr(ShumateVectorValue) copy = g_new0 (ShumateVectorValue, 1);

  g_return_if_fail (self->type == SHUMATE_VECTOR_VALUE_TYPE_ARRAY);

  shumate_vector_value_copy (element, copy);
  g_ptr_array_add (self->array, g_steal_pointer (&copy));
}


GPtrArray *
shumate_vector_value_get_array (ShumateVectorValue *self)
{
  if (self->type != SHUMATE_VECTOR_VALUE_TYPE_ARRAY)
    return NULL;

  return self->array;
}


void
shumate_vector_value_set_image (ShumateVectorValue *self,
                                GdkPixbuf          *image,
                                const char         *image_name)
{
  g_assert (GDK_IS_PIXBUF (image));
  g_assert (image_name != NULL);

  shumate_vector_value_unset (self);
  self->type = SHUMATE_VECTOR_VALUE_TYPE_RESOLVED_IMAGE;
  self->image = g_object_ref (image);
  self->image_name = g_strdup (image_name);
}

gboolean
shumate_vector_value_get_image (ShumateVectorValue *self, GdkPixbuf **image)
{
  if (self->type != SHUMATE_VECTOR_VALUE_TYPE_RESOLVED_IMAGE)
    return FALSE;

  *image = self->image;
  return TRUE;
}

void
shumate_vector_value_set_formatted (ShumateVectorValue *self,
                                    GPtrArray          *format_parts)
{
  shumate_vector_value_unset (self);
  self->type = SHUMATE_VECTOR_VALUE_TYPE_FORMATTED_STRING;
  self->formatted_string = g_ptr_array_ref (format_parts);
}

gboolean
shumate_vector_value_get_formatted (ShumateVectorValue  *self,
                                    GPtrArray          **format_parts)
{
  if (self->type != SHUMATE_VECTOR_VALUE_TYPE_FORMATTED_STRING)
    return FALSE;

  *format_parts = self->formatted_string;
  return TRUE;
}

void
shumate_vector_format_part_free (ShumateVectorFormatPart *format_part)
{
  g_clear_pointer (&format_part->string, g_free);
  g_clear_object (&format_part->image);
  g_free (format_part);
}

gboolean
shumate_vector_value_equal (ShumateVectorValue *a, ShumateVectorValue *b)
{
  if (a->type != b->type)
    return FALSE;

  switch (a->type)
    {
    case SHUMATE_VECTOR_VALUE_TYPE_NULL:
      return TRUE;
    case SHUMATE_VECTOR_VALUE_TYPE_NUMBER:
      return a->number == b->number;
    case SHUMATE_VECTOR_VALUE_TYPE_BOOLEAN:
      return a->boolean == b->boolean;
    case SHUMATE_VECTOR_VALUE_TYPE_STRING:
      return g_strcmp0 (a->string, b->string) == 0;
    case SHUMATE_VECTOR_VALUE_TYPE_COLOR:
      return gdk_rgba_equal (&a->color, &b->color);
    case SHUMATE_VECTOR_VALUE_TYPE_ARRAY:
      if (a->array->len != b->array->len)
        return FALSE;

      for (int i = 0; i < a->array->len; i ++)
        if (!shumate_vector_value_equal (g_ptr_array_index (a->array, i), g_ptr_array_index (b->array, i)))
          return FALSE;

      return TRUE;
    case SHUMATE_VECTOR_VALUE_TYPE_RESOLVED_IMAGE:
      return g_strcmp0 (a->image_name, b->image_name) == 0;
    case SHUMATE_VECTOR_VALUE_TYPE_FORMATTED_STRING:
      /* Not supported */
      return FALSE;
    default:
      g_assert_not_reached ();
    }
}


char *
shumate_vector_value_as_string (ShumateVectorValue *self)
{
  switch (self->type)
    {
    case SHUMATE_VECTOR_VALUE_TYPE_NULL:
      return g_strdup ("");
    case SHUMATE_VECTOR_VALUE_TYPE_NUMBER:
      return g_strdup_printf ("%g", self->number);
    case SHUMATE_VECTOR_VALUE_TYPE_BOOLEAN:
      return g_strdup (self->boolean ? "true" : "false");
    case SHUMATE_VECTOR_VALUE_TYPE_STRING:
      return g_strdup (self->string);
    case SHUMATE_VECTOR_VALUE_TYPE_COLOR:
      return gdk_rgba_to_string (&self->color);
    case SHUMATE_VECTOR_VALUE_TYPE_ARRAY:
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
    case SHUMATE_VECTOR_VALUE_TYPE_RESOLVED_IMAGE:
      return g_strdup (self->image_name);
    case SHUMATE_VECTOR_VALUE_TYPE_FORMATTED_STRING:
      {
        g_autoptr(GString) result = g_string_new ("");

        for (int i = 0; i < self->formatted_string->len; i ++)
          {
            ShumateVectorFormatPart *part = g_ptr_array_index (self->formatted_string, i);
            if (!part->image)
              g_string_append (result, part->string);
          }

        return g_string_free (g_steal_pointer (&result), FALSE);
      }
    default:
      g_assert_not_reached ();
    }
}
