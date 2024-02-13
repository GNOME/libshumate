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

#include <glib-object.h>
#include <cairo/cairo.h>
#include "vector_tile.pb-c.h"
#include "../shumate-vector-sprite-sheet.h"
#include "shumate-vector-value-private.h"
#include "shumate-vector-utils-private.h"
#include "../shumate-vector-reader-iter-private.h"
#include "shumate-vector-index-private.h"

typedef enum {
  SHUMATE_VECTOR_GEOMETRY_POINT = VECTOR_TILE__TILE__GEOM_TYPE__POINT,
  SHUMATE_VECTOR_GEOMETRY_LINESTRING = VECTOR_TILE__TILE__GEOM_TYPE__LINESTRING,
  SHUMATE_VECTOR_GEOMETRY_POLYGON = VECTOR_TILE__TILE__GEOM_TYPE__POLYGON,
} ShumateVectorGeometryType;

typedef struct {
  cairo_t *cr;
  int target_size;
  double scale;
  double scale_factor;
  double zoom_level;
  int tile_x;
  int tile_y;
  int layer_idx;
  int source_layer_idx;

  GPtrArray *symbols;

  ShumateVectorSpriteSheet *sprites;

  float overzoom_x, overzoom_y, overzoom_scale;

  ShumateVectorReaderIter *reader;
  ShumateVectorIndex *index;
  ShumateVectorIndexDescription *index_description;
} ShumateVectorRenderScope;


void shumate_vector_render_scope_exec_geometry (ShumateVectorRenderScope *self);
void shumate_vector_render_scope_get_geometry_center (ShumateVectorRenderScope *self, double *x, double *y);
void shumate_vector_render_scope_get_bounds (ShumateVectorRenderScope *self,
                                             float                    *min_x,
                                             float                    *min_y,
                                             float                    *max_x,
                                             float                    *max_y);
ShumateVectorGeometryType shumate_vector_render_scope_get_geometry_type (ShumateVectorRenderScope *self);

GPtrArray *shumate_vector_render_scope_get_geometry (ShumateVectorRenderScope *self);

void shumate_vector_render_scope_get_variable (ShumateVectorRenderScope *self, const char *variable, ShumateVectorValue *value);

GHashTable *shumate_vector_render_scope_create_tag_table (ShumateVectorRenderScope *self);

void shumate_vector_render_scope_index_layer (ShumateVectorRenderScope *self);
