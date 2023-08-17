/*
 * Copyright (C) 2023 James Westman <james@jwestman.net>
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

#include <glib-object.h>

G_BEGIN_DECLS

/**
 * ShumateGeometryType:
 * @SHUMATE_GEOMETRY_TYPE_UNKNOWN: Unknown geometry type
 * @SHUMATE_GEOMETRY_TYPE_POINT: A single point
 * @SHUMATE_GEOMETRY_TYPE_MULTIPOINT: A collection of points
 * @SHUMATE_GEOMETRY_TYPE_LINESTRING: A single line
 * @SHUMATE_GEOMETRY_TYPE_MULTILINESTRING: A collection of lines
 * @SHUMATE_GEOMETRY_TYPE_POLYGON: A single polygon
 * @SHUMATE_GEOMETRY_TYPE_MULTIPOLYGON: A collection of polygons
 *
 * A type of geometry.
 *
 * Since: 1.2
 */
typedef enum {
  SHUMATE_GEOMETRY_TYPE_UNKNOWN,
  SHUMATE_GEOMETRY_TYPE_POINT,
  SHUMATE_GEOMETRY_TYPE_MULTIPOINT,
  SHUMATE_GEOMETRY_TYPE_LINESTRING,
  SHUMATE_GEOMETRY_TYPE_MULTILINESTRING,
  SHUMATE_GEOMETRY_TYPE_POLYGON,
  SHUMATE_GEOMETRY_TYPE_MULTIPOLYGON
} ShumateGeometryType;

#define SHUMATE_TYPE_VECTOR_READER_ITER (shumate_vector_reader_iter_get_type ())
G_DECLARE_FINAL_TYPE (ShumateVectorReaderIter, shumate_vector_reader_iter, SHUMATE, VECTOR_READER_ITER, GObject)

guint shumate_vector_reader_iter_get_layer_count (ShumateVectorReaderIter *self);
void shumate_vector_reader_iter_read_layer (ShumateVectorReaderIter *self, int index);
gboolean shumate_vector_reader_iter_read_layer_by_name (ShumateVectorReaderIter *self, const char *name);
const char *shumate_vector_reader_iter_get_layer_name (ShumateVectorReaderIter *self);

guint shumate_vector_reader_iter_get_layer_extent (ShumateVectorReaderIter *self);
guint shumate_vector_reader_iter_get_layer_feature_count (ShumateVectorReaderIter *self);

void shumate_vector_reader_iter_read_feature (ShumateVectorReaderIter *self, int index);
gboolean shumate_vector_reader_iter_next_feature (ShumateVectorReaderIter *self);
guint64 shumate_vector_reader_iter_get_feature_id (ShumateVectorReaderIter *self);
gboolean shumate_vector_reader_iter_get_feature_tag (ShumateVectorReaderIter *self,
                                                const char          *key,
                                                GValue              *value);
const char **shumate_vector_reader_iter_get_feature_keys (ShumateVectorReaderIter *self);


ShumateGeometryType shumate_vector_reader_iter_get_feature_geometry_type (ShumateVectorReaderIter *self);
gboolean shumate_vector_reader_iter_get_feature_point (ShumateVectorReaderIter *self,
                                                       double                  *x,
                                                       double                  *y);
gboolean shumate_vector_reader_iter_feature_contains_point (ShumateVectorReaderIter *self,
                                                            double                   x,
                                                            double                   y);
G_END_DECLS