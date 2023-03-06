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

ShumateVectorCollision *shumate_vector_collision_new ();
void shumate_vector_collision_free (ShumateVectorCollision *self);

gboolean shumate_vector_collision_check (ShumateVectorCollision *self,
                                         float                   x,
                                         float                   y,
                                         float                   xextent,
                                         float                   yextent,
                                         float                   rotation);
int shumate_vector_collision_save_pending (ShumateVectorCollision *self);
void shumate_vector_collision_rollback_pending (ShumateVectorCollision *self,
                                                int                     save);
void shumate_vector_collision_commit_pending (ShumateVectorCollision *self,
                                              graphene_rect_t         *bounds_out);

void shumate_vector_collision_clear (ShumateVectorCollision *self);

void shumate_vector_collision_visualize (ShumateVectorCollision *self,
                                         GtkSnapshot            *snapshot);

G_END_DECLS
