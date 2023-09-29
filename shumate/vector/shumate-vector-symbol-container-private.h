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

#include "shumate-map-source.h"
#include "shumate-layer.h"
#include "shumate-vector-collision-private.h"

G_BEGIN_DECLS


#define SHUMATE_TYPE_VECTOR_SYMBOL_CONTAINER (shumate_vector_symbol_container_get_type())
G_DECLARE_FINAL_TYPE (ShumateVectorSymbolContainer, shumate_vector_symbol_container, SHUMATE, VECTOR_SYMBOL_CONTAINER, ShumateLayer)

ShumateVectorSymbolContainer *shumate_vector_symbol_container_new (ShumateMapSource *map_source,
                                                                   ShumateViewport  *viewport);

void shumate_vector_symbol_container_add_symbols (ShumateVectorSymbolContainer *self,
                                                  GPtrArray                    *symbol_infos,
                                                  int                           tile_x,
                                                  int                           tile_y,
                                                  int                           zoom);

void shumate_vector_symbol_container_remove_symbols (ShumateVectorSymbolContainer *self,
                                                     int                           tile_x,
                                                     int                           tile_y,
                                                     int                           zoom);

ShumateMapSource *shumate_vector_symbol_container_get_map_source (ShumateVectorSymbolContainer *self);

ShumateVectorCollision *shumate_vector_symbol_container_get_collision (ShumateVectorSymbolContainer *self);

char *shumate_vector_symbol_container_get_debug_text (ShumateVectorSymbolContainer *self);

G_END_DECLS
