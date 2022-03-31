/*
 * Copyright (C) 2022 James Westman <james@jwestman.net>
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

#include <gtk/gtk.h>
#include "shumate-vector-utils-private.h"
#include "shumate-vector-symbol-info-private.h"

G_BEGIN_DECLS


typedef struct ShumateVectorCollision ShumateVectorCollision;

typedef struct {
  float x;
  float y;
  float xextent;
  float yextent;
  float rotation;
} ShumateVectorCollisionBBox;

typedef struct {
  ShumateVectorCollisionBBox bbox;
  ShumateVectorSymbolInfo *symbol_info;
  GList *list_link;
  int text_length;
  int tile_x;
  int tile_y;
  int zoom;
  guint seq : 1;
  guint visible : 1;
} ShumateVectorCollisionMarker;

ShumateVectorCollision *shumate_vector_collision_new (int tile_size);
void shumate_vector_collision_free (ShumateVectorCollision *self);

ShumateVectorCollisionMarker *shumate_vector_collision_insert (ShumateVectorCollision  *self,
                                                               int                      zoom,
                                                               ShumateVectorSymbolInfo *symbol_info,
                                                               float                    text_length,
                                                               int                      tile_x,
                                                               int                      tile_y);

void shumate_vector_collision_remove (ShumateVectorCollision       *self,
                                      ShumateVectorCollisionMarker *marker);

void shumate_vector_collision_recalc (ShumateVectorCollision *self,
                                      float                   rot,
                                      float                   zoom);

G_END_DECLS
