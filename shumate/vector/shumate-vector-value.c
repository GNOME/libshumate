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

#include "shumate-vector-value-private.h"

enum {
  TYPE_NULL,
  TYPE_NUMBER,
  TYPE_BOOLEAN,
  TYPE_STRING,
  TYPE_COLOR,
};

enum {
  COLOR_UNSET,
  COLOR_SET,
  COLOR_INVALID,
};

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
shumate_vector_value_unset (ShumateVectorValue *self)
{
  if (self->type == TYPE_STRING)
    g_clear_pointer (&self->string, g_free);
  self->type = TYPE_NULL;
}


void
shumate_vector_value_copy (ShumateVectorValue *self, ShumateVectorValue *out)
{
  shumate_vector_value_unset (out);
  *out = *self;

  if (self->type == TYPE_STRING)
    out->string = g_strdup (out->string);
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
    default:
      g_assert_not_reached ();
    }
}
