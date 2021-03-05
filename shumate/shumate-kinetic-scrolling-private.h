/*
 * Copyright (C) 2014 Lieven van der Heide
 * Copyright (C) 2021 Georges Basile Stavracas Neto <georges.stavracas@gmail.com>
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#pragma once

#include <glib.h>

G_BEGIN_DECLS

typedef struct _ShumateKineticScrolling ShumateKineticScrolling;

ShumateKineticScrolling *shumate_kinetic_scrolling_new  (double decel_friction,
                                                         double initial_velocity);
void shumate_kinetic_scrolling_free (ShumateKineticScrolling  *kinetic);

gboolean shumate_kinetic_scrolling_tick (ShumateKineticScrolling *data,
                                         double                   time_delta_us,
                                         double                  *position);

G_END_DECLS
