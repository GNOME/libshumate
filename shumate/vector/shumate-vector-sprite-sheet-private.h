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

#include <gdk/gdk.h>

G_BEGIN_DECLS

#define SHUMATE_TYPE_VECTOR_SPRITE_SHEET (shumate_vector_sprite_sheet_get_type())
G_DECLARE_FINAL_TYPE (ShumateVectorSpriteSheet, shumate_vector_sprite_sheet, SHUMATE, VECTOR_SPRITE_SHEET, GObject)

ShumateVectorSpriteSheet *shumate_vector_sprite_sheet_new (GdkPixbuf     *pixbuf,
                                                           const char    *json,
                                                           GCancellable  *cancellable,
                                                           GError       **error);

GdkPixbuf *shumate_vector_sprite_sheet_get_icon (ShumateVectorSpriteSheet *self,
                                                 const char               *name);

G_END_DECLS
