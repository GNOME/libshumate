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
#include "vector_tile.pb-c.h"

typedef struct _ShumateVectorPoint ShumateVectorPoint;
typedef struct _ShumateVectorLineString ShumateVectorLineString;
typedef struct _ShumateVectorPointIter ShumateVectorPointIter;

struct _ShumateVectorPoint {
  double x;
  double y;
};

struct _ShumateVectorLineString {
  gsize n_points;
  ShumateVectorPoint *points;
};

struct _ShumateVectorPointIter {
  gsize num_points;
  ShumateVectorPoint *points;
  gsize current_point;
  double distance;
  guint reversed : 1;
};

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

gboolean shumate_vector_json_get_object (JsonNode *node, JsonObject **dest, GError **error);
gboolean shumate_vector_json_get_array (JsonNode *node, JsonArray **dest, GError **error);

void   shumate_vector_point_iter_init                 (ShumateVectorPointIter  *iter,
                                                       ShumateVectorLineString *linestring);
gboolean shumate_vector_point_iter_is_at_end          (ShumateVectorPointIter *iter);
double shumate_vector_point_iter_next_segment         (ShumateVectorPointIter *iter);
double shumate_vector_point_iter_get_segment_length   (ShumateVectorPointIter *iter);
void   shumate_vector_point_iter_get_segment_center   (ShumateVectorPointIter *iter,
                                                       double                  remaining_distance,
                                                       ShumateVectorPoint     *result);
void   shumate_vector_point_iter_get_current_point    (ShumateVectorPointIter *iter,
                                                       ShumateVectorPoint     *result);
double shumate_vector_point_iter_get_average_angle    (ShumateVectorPointIter *iter,
                                                       double                  remaining_distance);
void   shumate_vector_point_iter_advance              (ShumateVectorPointIter *iter,
                                                       double                  distance);
double shumate_vector_point_iter_get_current_angle    (ShumateVectorPointIter *iter);

ShumateVectorLineString *shumate_vector_line_string_copy (ShumateVectorLineString *linestring);
void   shumate_vector_line_string_free                (ShumateVectorLineString *linestring);
double shumate_vector_line_string_length              (ShumateVectorLineString *linestring);
void   shumate_vector_line_string_bounds              (ShumateVectorLineString *linestring,
                                                       ShumateVectorPoint      *radius_out,
                                                       ShumateVectorPoint      *center_out);
GPtrArray *shumate_vector_line_string_simplify        (ShumateVectorLineString *linestring);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (ShumateVectorLineString, shumate_vector_line_string_free)

typedef enum {
  SHUMATE_VECTOR_GEOMETRY_OP_MOVE_TO = 1,
  SHUMATE_VECTOR_GEOMETRY_OP_LINE_TO = 2,
  SHUMATE_VECTOR_GEOMETRY_OP_CLOSE_PATH = 7,
} ShumateVectorGeometryOp;

typedef struct {
  VectorTile__Tile__Feature *feature;
  int i, j;
  int op, repeat;
  int x, y;
  int dx, dy;
  int start_x, start_y;
  int cursor_x, cursor_y;
} ShumateVectorGeometryIter;

gboolean shumate_vector_geometry_iter (ShumateVectorGeometryIter *iter);