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

G_BEGIN_DECLS

#define SHUMATE_TYPE_VECTOR_SPRITE shumate_vector_sprite_get_type()
G_DECLARE_FINAL_TYPE(ShumateVectorSprite, shumate_vector_sprite, SHUMATE, VECTOR_SPRITE, GObject)


ShumateVectorSprite *shumate_vector_sprite_new (GdkPaintable *source_paintable);
ShumateVectorSprite *shumate_vector_sprite_new_full (GdkPaintable *source_paintable,
                                                     int           width,
                                                     int           height,
                                                     double        scale_factor,
                                                     GdkRectangle *source_rect);

GdkPaintable *shumate_vector_sprite_get_source_paintable (ShumateVectorSprite *self);
int           shumate_vector_sprite_get_width            (ShumateVectorSprite *self);
int           shumate_vector_sprite_get_height           (ShumateVectorSprite *self);
double        shumate_vector_sprite_get_scale_factor     (ShumateVectorSprite *self);
GdkRectangle *shumate_vector_sprite_get_source_rect      (ShumateVectorSprite *self);

G_END_DECLS