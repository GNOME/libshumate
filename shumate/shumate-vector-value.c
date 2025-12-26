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

/**
 * ShumateVectorValue: (copy-func shumate_vector_value_dup) (free-func shumate_vector_value_free)
 *
 * A mutable value used in the vector style specification.
 *
 * Since: 1.6
 */

G_DEFINE_BOXED_TYPE (ShumateVectorValue, shumate_vector_value, shumate_vector_value_dup, shumate_vector_value_free);

enum {
  COLOR_UNSET,
  COLOR_SET,
  COLOR_INVALID,
};


/**
 * shumate_vector_value_new:
 *
 * Creates a new [struct@VectorValue] with a null value.
 *
 * Returns: (transfer full): a new [struct@VectorValue] with a null value
 * Since: 1.6
 */
ShumateVectorValue *
shumate_vector_value_new (void)
{
  ShumateVectorValue *self = g_new0 (ShumateVectorValue, 1);
  return self;
}

/**
 * shumate_vector_value_new_from_value:
 * @value: a [struct@GObject.Value]
 *
 * Creates a new [struct@VectorValue] from a [struct@GObject.Value].
 *
 * Returns: (transfer full): a new [struct@VectorValue] with the value from @value
 * Since: 1.6
 */
ShumateVectorValue *
shumate_vector_value_new_from_value (GValue *value)
{
  ShumateVectorValue *self = g_new0 (ShumateVectorValue, 1);
  shumate_vector_value_set_from_g_value (self, value);
  return self;
}

/**
 * shumate_vector_value_new_number:
 * @number: a number value
 *
 * Creates a new [struct@VectorValue] with a number value.
 *
 * Returns: (transfer full): a new [struct@VectorValue] with the given number value
 * Since: 1.6
 */
ShumateVectorValue *
shumate_vector_value_new_number (double number)
{
  ShumateVectorValue *self = g_new0 (ShumateVectorValue, 1);
  shumate_vector_value_set_number (self, number);
  return self;
}

/**
 * shumate_vector_value_new_string:
 * @string: a string value
 *
 * Creates a new [struct@VectorValue] with a string value.
 *
 * Returns: (transfer full): a new [struct@VectorValue] with the given string value
 * Since: 1.6
 */
ShumateVectorValue *
shumate_vector_value_new_string (const char *string)
{
  ShumateVectorValue *self = g_new0 (ShumateVectorValue, 1);
  shumate_vector_value_set_string (self, string);
  return self;
}

/**
 * shumate_vector_value_new_boolean:
 * @boolean: a boolean value
 *
 * Creates a new [struct@VectorValue] with a boolean value.
 *
 * Returns: (transfer full): a new [struct@VectorValue] with the given boolean value
 * Since: 1.6
 */
ShumateVectorValue *
shumate_vector_value_new_boolean (gboolean boolean)
{
  ShumateVectorValue *self = g_new0 (ShumateVectorValue, 1);
  shumate_vector_value_set_boolean (self, boolean);
  return self;
}

/**
 * shumate_vector_value_new_color:
 * @color: a [struct@Gdk.RGBA]
 *
 * Creates a new [struct@VectorValue] with a color value.
 *
 * Returns: (transfer full): a new [struct@VectorValue] with the given color value
 * Since: 1.6
 */
ShumateVectorValue *
shumate_vector_value_new_color (GdkRGBA *color)
{
  ShumateVectorValue *self = g_new0 (ShumateVectorValue, 1);
  shumate_vector_value_set_color (self, color);
  return self;
}

#ifdef SHUMATE_HAS_VECTOR_RENDERER
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
#endif

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


/**
 * shumate_vector_value_free:
 * @self: a [struct@VectorValue]
 *
 * Frees a [struct@VectorValue].
 *
 * Since: 1.6
 */
void
shumate_vector_value_free (ShumateVectorValue *self)
{
  shumate_vector_value_unset (self);
  g_free (self);
}


/**
 * shumate_vector_value_unset:
 * @self: a [struct@VectorValue]
 *
 * Sets @self to a null value.
 *
 * Since: 1.6
 */
void
shumate_vector_value_unset (ShumateVectorValue *self)
{
  switch (self->type)
    {
    case SHUMATE_VECTOR_VALUE_TYPE_STRING:
      g_clear_pointer (&self->string, g_free);
      break;
    case SHUMATE_VECTOR_VALUE_TYPE_ARRAY:
      g_clear_pointer (&self->array, g_ptr_array_unref);
      break;
    case SHUMATE_VECTOR_VALUE_TYPE_RESOLVED_IMAGE:
      g_clear_object (&self->image);
      g_clear_pointer (&self->image_name, g_free);
      break;
    case SHUMATE_VECTOR_VALUE_TYPE_FORMATTED_STRING:
      g_clear_pointer (&self->formatted_string, g_ptr_array_unref);
      break;
    case SHUMATE_VECTOR_VALUE_TYPE_NULL:
    case SHUMATE_VECTOR_VALUE_TYPE_NUMBER:
    case SHUMATE_VECTOR_VALUE_TYPE_BOOLEAN:
    case SHUMATE_VECTOR_VALUE_TYPE_COLOR:
    case SHUMATE_VECTOR_VALUE_TYPE_COLLATOR:
      break;
    }

  self->type = SHUMATE_VECTOR_VALUE_TYPE_NULL;
}


/**
 * shumate_vector_value_is_null:
 * @self: a [struct@VectorValue]
 *
 * Checks if @self is a null value.
 *
 * Returns: %TRUE if @self is a null value, %FALSE otherwise
 *
 * Since: 1.6
 */
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

  switch (self->type)
    {
    case SHUMATE_VECTOR_VALUE_TYPE_STRING:
      out->string = g_strdup (self->string);
      break;
    case SHUMATE_VECTOR_VALUE_TYPE_ARRAY:
      out->array = g_ptr_array_ref (self->array);
      break;
    case SHUMATE_VECTOR_VALUE_TYPE_RESOLVED_IMAGE:
      out->image = g_object_ref (self->image);
      out->image_name = g_strdup (self->image_name);
      break;
    case SHUMATE_VECTOR_VALUE_TYPE_FORMATTED_STRING:
      out->formatted_string = g_ptr_array_ref (self->formatted_string);
      break;
    case SHUMATE_VECTOR_VALUE_TYPE_NULL:
    case SHUMATE_VECTOR_VALUE_TYPE_NUMBER:
    case SHUMATE_VECTOR_VALUE_TYPE_BOOLEAN:
    case SHUMATE_VECTOR_VALUE_TYPE_COLOR:
    case SHUMATE_VECTOR_VALUE_TYPE_COLLATOR:
      break;
    }
}

void
shumate_vector_value_steal (ShumateVectorValue *self, ShumateVectorValue *out)
{
  *out = *self;
  self->type = SHUMATE_VECTOR_VALUE_TYPE_NULL;
}


/**
 * shumate_vector_value_dup:
 * @self: a [struct@VectorValue]
 *
 * Creates a duplicate of @self.
 *
 * Returns: (transfer full): a new [struct@VectorValue] which is a duplicate of @self
 *
 * Since: 1.6
 */
ShumateVectorValue *
shumate_vector_value_dup (ShumateVectorValue *self)
{
  ShumateVectorValue *new_value = g_new0 (ShumateVectorValue, 1);
  shumate_vector_value_copy (self, new_value);
  return new_value;
}

/**
 * shumate_vector_value_get_value_type:
 * @self: a [struct@VectorValue]
 *
 * Gets the type of value stored in @self.
 *
 * Returns: the type of @self
 *
 * Since: 1.6
 */
ShumateVectorValueType
shumate_vector_value_get_value_type (ShumateVectorValue *self)
{
  return self->type;
}

/**
 * shumate_vector_value_set_number:
 * @self: a [struct@VectorValue]
 * @number: a number value
 *
 * Sets @self to a number value.
 *
 * Since: 1.6
 */
void
shumate_vector_value_set_number (ShumateVectorValue *self, double number)
{
  shumate_vector_value_unset (self);
  self->type = SHUMATE_VECTOR_VALUE_TYPE_NUMBER;
  self->number = number;
}


/**
 * shumate_vector_value_get_number:
 * @self: a [struct@VectorValue]
 * @number: (out): a location to store the number value
 *
 * Gets the number value of @self.
 *
 * Returns: %TRUE if @self is a number value and @number was set, %FALSE otherwise
 * Since: 1.6
 */
gboolean
shumate_vector_value_get_number (ShumateVectorValue *self, double *number)
{
  if (self->type != SHUMATE_VECTOR_VALUE_TYPE_NUMBER)
    return FALSE;

  *number = self->number;
  return TRUE;
}


/**
 * shumate_vector_value_set_boolean:
 * @self: a [struct@VectorValue]
 * @boolean: a boolean value
 *
 * Sets @self to a boolean value.
 *
 * Since: 1.6
 */
void
shumate_vector_value_set_boolean (ShumateVectorValue *self, gboolean boolean)
{
  shumate_vector_value_unset (self);
  self->type = SHUMATE_VECTOR_VALUE_TYPE_BOOLEAN;
  self->boolean = boolean;
}


/**
 * shumate_vector_value_get_boolean:
 * @self: a [struct@VectorValue]
 * @boolean: (out): a location to store the boolean value
 *
 * Gets the boolean value of @self.
 *
 * Returns: %TRUE if @self is a boolean value and @boolean was set, %FALSE otherwise
 * Since: 1.6
 */
gboolean
shumate_vector_value_get_boolean (ShumateVectorValue *self, gboolean *boolean)
{
  if (self->type != SHUMATE_VECTOR_VALUE_TYPE_BOOLEAN)
    return FALSE;

  *boolean = self->boolean;
  return TRUE;
}


/**
 * shumate_vector_value_set_string:
 * @self: a [struct@VectorValue]
 * @string: a string value
 *
 * Sets @self to a string value.
 *
 * Since: 1.6
 */
void
shumate_vector_value_set_string (ShumateVectorValue *self, const char *string)
{
  shumate_vector_value_unset (self);
  self->type = SHUMATE_VECTOR_VALUE_TYPE_STRING;
  self->string = g_strdup (string);
  self->color_state = COLOR_UNSET;
}


/**
 * shumate_vector_value_get_string:
 * @self: a [struct@VectorValue]
 * @string: (out): a location to store the string value
 *
 * Gets the string value of @self.
 *
 * Returns: %TRUE if @self is a string value and @string was set, %FALSE otherwise
 * Since: 1.6
 */
gboolean
shumate_vector_value_get_string (ShumateVectorValue *self, const char **string)
{
  if (self->type != SHUMATE_VECTOR_VALUE_TYPE_STRING)
    return FALSE;

  *string = self->string;
  return TRUE;
}


/**
 * shumate_vector_value_set_color:
 * @self: a [struct@VectorValue]
 * @color: a [struct@Gdk.RGBA]
 *
 * Sets @self to a color value.
 *
 * Since: 1.6
 */
void
shumate_vector_value_set_color (ShumateVectorValue *self, GdkRGBA *color)
{
  shumate_vector_value_unset (self);
  self->type = SHUMATE_VECTOR_VALUE_TYPE_COLOR;
  self->color = *color;
}


/**
 * shumate_vector_value_get_color:
 * @self: a [struct@VectorValue]
 * @color: (out): a location to store the color value
 *
 * Gets the color value of @self.
 *
 * If @self is a string value, it will attempt to parse the string as a color.
 *
 * Returns: %TRUE if @self is a color value and @color was set, %FALSE otherwise
 * Since: 1.6
 */
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


/**
 * shumate_vector_value_start_array:
 * @self: a [struct@VectorValue]
 *
 * Sets @self to an empty array value.
 *
 * Since: 1.6
 */
void
shumate_vector_value_start_array (ShumateVectorValue *self)
{
  shumate_vector_value_unset (self);
  self->type = SHUMATE_VECTOR_VALUE_TYPE_ARRAY;
  self->array = g_ptr_array_new_with_free_func ((GDestroyNotify)shumate_vector_value_free);
}


/**
 * shumate_vector_value_array_append:
 * @self: an array [struct@VectorValue]
 * @element: a [struct@VectorValue] to append to the array
 *
 * Appends @element to the array value of @self. The value of @element is copied.
 *
 * Since: 1.6
 */
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
shumate_vector_value_set_image (ShumateVectorValue  *self,
                                ShumateVectorSprite *image,
                                const char          *image_name)
{
  g_assert (SHUMATE_IS_VECTOR_SPRITE (image));
  g_assert (image_name != NULL);

  shumate_vector_value_unset (self);
  self->type = SHUMATE_VECTOR_VALUE_TYPE_RESOLVED_IMAGE;
  self->image = g_object_ref (image);
  self->image_name = g_strdup (image_name);
}

gboolean
shumate_vector_value_get_image (ShumateVectorValue *self, ShumateVectorSprite **image)
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
  g_clear_object (&format_part->sprite);
  g_free (format_part);
}

void
shumate_vector_value_set_collator (ShumateVectorValue    *self,
                                   ShumateVectorCollator *collator)
{
  shumate_vector_value_unset (self);
  self->type = SHUMATE_VECTOR_VALUE_TYPE_COLLATOR;
  self->collator = *collator;
}

gboolean
shumate_vector_value_get_collator (ShumateVectorValue    *self,
                                   ShumateVectorCollator *collator)
{
  if (self->type != SHUMATE_VECTOR_VALUE_TYPE_COLLATOR)
    return FALSE;

  *collator = self->collator;
  return TRUE;
}


/**
 * shumate_vector_value_hash:
 * @self: a [struct@VectorValue]
 *
 * Calculates a hash value for @self.
 *
 * Returns: a hash value for @self
 * Since: 1.6
 */
gint
shumate_vector_value_hash (ShumateVectorValue *self)
{
  switch (self->type)
    {
    case SHUMATE_VECTOR_VALUE_TYPE_NULL:
      return 0;
    case SHUMATE_VECTOR_VALUE_TYPE_NUMBER:
      return g_double_hash (&self->number);
    case SHUMATE_VECTOR_VALUE_TYPE_BOOLEAN:
      return !!self->boolean;
    case SHUMATE_VECTOR_VALUE_TYPE_STRING:
      return g_str_hash (self->string);
    case SHUMATE_VECTOR_VALUE_TYPE_COLOR:
      return gdk_rgba_hash (&self->color);
    case SHUMATE_VECTOR_VALUE_TYPE_ARRAY:
      {
        guint hash = 0;

        for (int i = 0; i < self->array->len; i ++)
          hash ^= shumate_vector_value_hash (g_ptr_array_index (self->array, i));

        return hash;
      }
    case SHUMATE_VECTOR_VALUE_TYPE_RESOLVED_IMAGE:
      return g_str_hash (self->image_name);
    case SHUMATE_VECTOR_VALUE_TYPE_FORMATTED_STRING:
    case SHUMATE_VECTOR_VALUE_TYPE_COLLATOR:
      /* Not supported */
      return 0;
    default:
      g_assert_not_reached ();
    }
}


/**
 * shumate_vector_value_equal:
 * @a: a [struct@VectorValue]
 * @b: a [struct@VectorValue]
 *
 * Compares two [struct@VectorValue]s for equality.
 *
 * Returns: %TRUE if @a and @b are equal, %FALSE otherwise
 * Since: 1.6
 */
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
    case SHUMATE_VECTOR_VALUE_TYPE_COLLATOR:
      /* Not supported */
      return FALSE;
    default:
      g_assert_not_reached ();
    }
}

#ifdef SHUMATE_HAS_VECTOR_RENDERER
static JsonNode *
shumate_vector_value_as_json (ShumateVectorValue *value)
{
  JsonNode *node;

  switch (value->type)
    {
    case SHUMATE_VECTOR_VALUE_TYPE_NULL:
      node = json_node_new (JSON_NODE_NULL);
      return node;
    case SHUMATE_VECTOR_VALUE_TYPE_NUMBER:
      node = json_node_new (JSON_NODE_VALUE);
      if (fmod (value->number, 1) == 0)
        json_node_set_int (node, value->number);
      else
        json_node_set_double (node, value->number);
      return node;
    case SHUMATE_VECTOR_VALUE_TYPE_BOOLEAN:
      node = json_node_new (JSON_NODE_VALUE);
      json_node_set_boolean (node, value->boolean);
      return node;
    case SHUMATE_VECTOR_VALUE_TYPE_ARRAY:
      {
        g_autoptr(JsonBuilder) builder = json_builder_new ();
        json_builder_begin_array (builder);
        for (int i = 0; i < value->array->len; i ++)
          {
            ShumateVectorValue *child = g_ptr_array_index (value->array, i);
            json_builder_add_value (builder, shumate_vector_value_as_json (child));
          }

        json_builder_end_array (builder);
        return json_builder_get_root (builder);
      }
    default:
      {
        g_autofree char *string = shumate_vector_value_as_string (value);
        node = json_node_new (JSON_NODE_VALUE);
        json_node_set_string (node, string);
        return node;
      }
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
      /* printf produces nan, inf, and -inf, but the spec says we should act like ECMAScript
         which uses NaN, -Infinity, and Infinity */
      if (G_UNLIKELY (isnan (self->number)))
        return g_strdup ("NaN");
      else if (G_UNLIKELY (isinf (self->number)))
        {
          if (self->number < 0)
            return g_strdup ("-Infinity");
          else
            return g_strdup ("Infinity");
        }
      else
        return g_strdup_printf ("%g", self->number);
    case SHUMATE_VECTOR_VALUE_TYPE_BOOLEAN:
      return g_strdup (self->boolean ? "true" : "false");
    case SHUMATE_VECTOR_VALUE_TYPE_STRING:
      return g_strdup (self->string);
    case SHUMATE_VECTOR_VALUE_TYPE_COLOR:
      /* gdk_rgba_to_string() produces `rgb(...)` when alpha is ~1, which is
         not consistent with the MapLibre spec */
      return g_strdup_printf (
        "rgba(%d,%d,%d,%g)",
        (int)round (255 * CLAMP (self->color.red, 0, 1)),
        (int)round (255 * CLAMP (self->color.green, 0, 1)),
        (int)round (255 * CLAMP (self->color.blue, 0, 1)),
        CLAMP (self->color.alpha, 0, 1)
      );
    case SHUMATE_VECTOR_VALUE_TYPE_ARRAY:
      {
        g_autoptr(JsonNode) node = shumate_vector_value_as_json (self);
        return json_to_string (node, FALSE);
      }
    case SHUMATE_VECTOR_VALUE_TYPE_RESOLVED_IMAGE:
      return g_strdup (self->image_name);
    case SHUMATE_VECTOR_VALUE_TYPE_FORMATTED_STRING:
      {
        g_autoptr(GString) result = g_string_new ("");

        for (int i = 0; i < self->formatted_string->len; i ++)
          {
            ShumateVectorFormatPart *part = g_ptr_array_index (self->formatted_string, i);
            if (!part->sprite)
              g_string_append (result, part->string);
          }

        return g_string_free (g_steal_pointer (&result), FALSE);
      }
    case SHUMATE_VECTOR_VALUE_TYPE_COLLATOR:
      /* Not supported */
      return g_strdup ("");
    default:
      g_assert_not_reached ();
    }
}
#endif
