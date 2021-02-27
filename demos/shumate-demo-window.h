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

#include <shumate/shumate.h>

G_BEGIN_DECLS

#define SHUMATE_TYPE_DEMO_WINDOW (shumate_demo_window_get_type())

G_DECLARE_FINAL_TYPE (ShumateDemoWindow, shumate_demo_window, SHUMATE, DEMO_WINDOW, GtkApplicationWindow)

ShumateDemoWindow *shumate_demo_window_new (GtkApplication *app);

G_END_DECLS
