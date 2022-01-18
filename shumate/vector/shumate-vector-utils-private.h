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

#pragma once

#include <json-glib/json-glib.h>


struct _ShumateVectorPoint {
  float x;
  float y;
};

typedef struct _ShumateVectorPoint ShumateVectorPoint;


gboolean shumate_vector_json_get_object (JsonNode    *node,
                                         JsonObject **dest,
                                         GError     **error);
gboolean shumate_vector_json_get_array (JsonNode   *node,
                                        JsonArray **dest,
                                        GError    **error);
gboolean shumate_vector_json_get_string (JsonNode    *node,
                                         const char **dest,
                                         GError     **error);

gboolean shumate_vector_json_get_object_member (JsonObject  *object,
                                                const char  *name,
                                                JsonObject **dest,
                                                GError     **error);
gboolean shumate_vector_json_get_array_member (JsonObject  *object,
                                               const char  *name,
                                               JsonArray  **dest,
                                               GError     **error);
gboolean shumate_vector_json_get_string_member (JsonObject  *object,
                                                const char  *name,
                                                const char **dest,
                                                GError     **error);

const char *shumate_vector_json_read_string_member (JsonReader *reader,
                                                    const char *name);

gboolean shumate_vector_json_read_int_member (JsonReader *reader,
                                              const char *name,
                                              int        *dest);
gboolean shumate_vector_json_read_double_member (JsonReader *reader,
                                                 const char *name,
                                                 double     *dest);
