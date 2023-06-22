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

#include <gtk/gtk.h>
#include "shumate-vector-symbol-info-private.h"
#include "shumate-vector-collision-private.h"

G_BEGIN_DECLS

#define SHUMATE_TYPE_VECTOR_SYMBOL (shumate_vector_symbol_get_type())
G_DECLARE_FINAL_TYPE (ShumateVectorSymbol, shumate_vector_symbol, SHUMATE, VECTOR_SYMBOL, GtkWidget)

ShumateVectorSymbol *shumate_vector_symbol_new (ShumateVectorSymbolInfo *symbol_info);

ShumateVectorSymbolInfo *shumate_vector_symbol_get_symbol_info (ShumateVectorSymbol *self);

gboolean shumate_vector_symbol_calculate_collision (ShumateVectorSymbol    *self,
                                                    ShumateVectorCollision *collision,
                                                    double                  x,
                                                    double                  y,
                                                    double                  zoom_level,
                                                    double                  rotation,
                                                    graphene_rect_t        *bounds_out);
G_END_DECLS
