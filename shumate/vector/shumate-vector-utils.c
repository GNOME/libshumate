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

#include "shumate-vector-utils-private.h"

gboolean
shumate_vector_json_get_object (JsonNode *node, JsonObject **dest, GError **error)
{
  g_assert (node != NULL);
  g_assert (dest != NULL);

  if (!JSON_NODE_HOLDS_OBJECT (node))
    {
      g_set_error (error,
                   SHUMATE_STYLE_ERROR,
                   SHUMATE_STYLE_ERROR_MALFORMED_STYLE,
                   "Expected object, got %s",
                   json_node_type_name (node));
      return FALSE;
    }

  *dest = json_node_get_object (node);
  return TRUE;
}


gboolean
shumate_vector_json_get_array (JsonNode *node, JsonArray **dest, GError **error)
{
  g_assert (node != NULL);
  g_assert (dest != NULL);

  if (!JSON_NODE_HOLDS_ARRAY (node))
    {
      g_set_error (error,
                   SHUMATE_STYLE_ERROR,
                   SHUMATE_STYLE_ERROR_MALFORMED_STYLE,
                   "Expected array, got %s",
                   json_node_type_name (node));
      return FALSE;
    }

  *dest = json_node_get_array (node);
  return TRUE;
}

gboolean
shumate_vector_json_get_string (JsonNode    *node,
                                const char **dest,
                                GError     **error)
{
  g_assert (node != NULL);
  g_assert (dest != NULL);

  if (!JSON_NODE_HOLDS_VALUE (node) || json_node_get_value_type (node) != G_TYPE_STRING)
    {
      g_set_error (error,
                   SHUMATE_STYLE_ERROR,
                   SHUMATE_STYLE_ERROR_MALFORMED_STYLE,
                   "Expected string, got %s",
                   json_node_type_name (node));
      return FALSE;
    }

  *dest = json_node_get_string (node);
  return TRUE;
}


JsonNode *
get_member (JsonObject *object,
            const char *name)
{
  if (object == NULL)
    return FALSE;
  return json_object_get_member (object, name);
}


gboolean
shumate_vector_json_get_object_member (JsonObject  *object,
                                       const char  *name,
                                       JsonObject **dest,
                                       GError     **error)
{
  JsonNode *node;

  g_assert (dest != NULL);

  node = get_member (object, name);
  if (node == NULL)
    {
      *dest = NULL;
      return TRUE;
    }

  return shumate_vector_json_get_object (node, dest, error);
}


gboolean
shumate_vector_json_get_array_member (JsonObject  *object,
                                      const char  *name,
                                      JsonArray  **dest,
                                      GError     **error)
{
  JsonNode *node;

  g_assert (dest != NULL);

  node = get_member (object, name);
  if (node == NULL)
    {
      *dest = NULL;
      return TRUE;
    }

  return shumate_vector_json_get_array (node, dest, error);
}


gboolean
shumate_vector_json_get_string_member (JsonObject  *object,
                                       const char  *name,
                                       const char **dest,
                                       GError     **error)
{
  JsonNode *node;

  g_assert (dest != NULL);

  node = get_member (object, name);
  if (node == NULL)
    {
      *dest = NULL;
      return TRUE;
    }

  return shumate_vector_json_get_string (node, dest, error);
}
