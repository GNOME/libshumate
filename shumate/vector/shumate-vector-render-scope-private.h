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
#include "shumate-vector-sprite-sheet-private.h"
#include "shumate-vector-value-private.h"
#include "shumate-vector-utils-private.h"

typedef struct {
  cairo_t *cr;
  int target_size;
  double scale;
  double zoom_level;
  int tile_x;
  int tile_y;

  GPtrArray *symbols;

  ShumateVectorSpriteSheet *sprites;

  VectorTile__Tile *tile;
  VectorTile__Tile__Layer *layer;
  VectorTile__Tile__Feature *feature;
} ShumateVectorRenderScope;


gboolean shumate_vector_render_scope_find_layer (ShumateVectorRenderScope *self, const char *layer_name);
void shumate_vector_render_scope_exec_geometry (ShumateVectorRenderScope *self);
void shumate_vector_render_scope_get_geometry_center (ShumateVectorRenderScope *self, double *x, double *y);
void shumate_vector_render_scope_get_bounds (ShumateVectorRenderScope *self,
                                             double                   *min_x,
                                             double                   *min_y,
                                             double                   *max_x,
                                             double                   *max_y);

GPtrArray *shumate_vector_render_scope_get_geometry (ShumateVectorRenderScope *self);

void shumate_vector_render_scope_get_variable (ShumateVectorRenderScope *self, const char *variable, ShumateVectorValue *value);

GHashTable *shumate_vector_render_scope_create_tag_table (ShumateVectorRenderScope *self);

