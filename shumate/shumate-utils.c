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

#include "shumate-utils-private.h"

void
shumate_grid_position_init (ShumateGridPosition *self,
                            int                  x,
                            int                  y,
                            int                  zoom)
{
  self->x = x;
  self->y = y;
  self->zoom = zoom;
}

ShumateGridPosition *
shumate_grid_position_new (int x,
                           int y,
                           int zoom)
{
  ShumateGridPosition *self = g_new0 (ShumateGridPosition, 1);
  shumate_grid_position_init (self, x, y, zoom);
  return self;
}

ShumateGridPosition *
shumate_grid_position_copy (ShumateGridPosition *pos)
{
  return shumate_grid_position_new (pos->x, pos->y, pos->zoom);
}

guint
shumate_grid_position_hash (gconstpointer pointer)
{
  const ShumateGridPosition *self = pointer;
  return self->x ^ self->y ^ self->zoom;
}

gboolean
shumate_grid_position_equal (gconstpointer a, gconstpointer b)
{
  const ShumateGridPosition *pos_a = a;
  const ShumateGridPosition *pos_b = b;
  return pos_a->x == pos_b->x && pos_a->y == pos_b->y && pos_a->zoom == pos_b->zoom;
}

void
shumate_grid_position_free (gpointer pointer)
{
  ShumateGridPosition *self = pointer;
  if (!self)
    return;

  g_free (self);
}

