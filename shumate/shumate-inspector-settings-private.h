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

#include <glib-object.h>

G_BEGIN_DECLS

#define SHUMATE_TYPE_INSPECTOR_SETTINGS (shumate_inspector_settings_get_type())

G_DECLARE_FINAL_TYPE (ShumateInspectorSettings, shumate_inspector_settings, SHUMATE, INSPECTOR_SETTINGS, GObject)

ShumateInspectorSettings *shumate_inspector_settings_get_default (void);

void shumate_inspector_settings_reset (ShumateInspectorSettings *self);

gboolean shumate_inspector_settings_get_show_debug_overlay (ShumateInspectorSettings *self);
void shumate_inspector_settings_set_show_debug_overlay (ShumateInspectorSettings *self, gboolean show_debug_overlay);

gboolean shumate_inspector_settings_get_show_tile_bounds (ShumateInspectorSettings *self);
void shumate_inspector_settings_set_show_tile_bounds (ShumateInspectorSettings *self, gboolean show_tile_bounds);

gboolean shumate_inspector_settings_get_show_collision_boxes (ShumateInspectorSettings *self);
void shumate_inspector_settings_set_show_collision_boxes (ShumateInspectorSettings *self, gboolean show_collision_boxes);

G_END_DECLS
