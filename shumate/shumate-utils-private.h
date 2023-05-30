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

#include "glib-object.h"

typedef struct
{
  int x;
  int y;
  int zoom;
} ShumateGridPosition;

#define SHUMATE_GRID_POSITION_INIT(x, y, zoom) ((ShumateGridPosition) {x, y, zoom})

void shumate_grid_position_init (ShumateGridPosition *self,
                                 int                  x,
                                 int                  y,
                                 int                  zoom);
ShumateGridPosition * shumate_grid_position_new (int x,
                                                 int y,
                                                 int zoom);
ShumateGridPosition *shumate_grid_position_copy (ShumateGridPosition *pos);

guint shumate_grid_position_hash (gconstpointer pointer);
gboolean shumate_grid_position_equal (gconstpointer a, gconstpointer b);
void shumate_grid_position_free (gpointer pointer);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (ShumateGridPosition, shumate_grid_position_free);
