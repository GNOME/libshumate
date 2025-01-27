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

typedef struct {
  GHashTable *bucket_rows;
  /* Use a ptr array because iterating a hash map is fairly expensive, and we
   * have to do it in the detect_collision() hot path */
  GPtrArray *bucket_rows_array;
  GArray *pending_boxes;

  double delta_x, delta_y;
} ShumateVectorCollision;

ShumateVectorCollision *shumate_vector_collision_new (void);
void shumate_vector_collision_free (ShumateVectorCollision *self);

gboolean shumate_vector_collision_check (ShumateVectorCollision *self,
                                         double                  x,
                                         double                  y,
                                         double                  xextent,
                                         double                  yextent,
                                         double                  rotation,
                                         ShumateVectorOverlap    overlap,
                                         gboolean                ignore_placement,
                                         gpointer                tag);
int shumate_vector_collision_save_pending (ShumateVectorCollision *self);
void shumate_vector_collision_rollback_pending (ShumateVectorCollision *self,
                                                int                     save);
void shumate_vector_collision_commit_pending (ShumateVectorCollision *self,
                                              graphene_rect_t         *bounds_out);

gboolean shumate_vector_collision_query_point (ShumateVectorCollision *self,
                                               double                  x,
                                               double                  y,
                                               gpointer                tag);

void shumate_vector_collision_clear (ShumateVectorCollision *self);

void shumate_vector_collision_visualize (ShumateVectorCollision *self,
                                         GtkSnapshot            *snapshot);

G_END_DECLS
