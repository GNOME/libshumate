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

#include "shumate-vector-renderer.h"
#include "shumate-utils-private.h"

void shumate_vector_renderer_render (ShumateVectorRenderer  *self,
                                     ShumateTile            *tile,
                                     GBytes                 *data,
                                     ShumateGridPosition    *source_position,
                                     GdkPaintable          **paintable,
                                     GPtrArray             **symbols);
