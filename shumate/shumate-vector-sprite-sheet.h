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
#include <shumate/shumate-vector-sprite.h>

G_BEGIN_DECLS

#define SHUMATE_TYPE_VECTOR_SPRITE_SHEET (shumate_vector_sprite_sheet_get_type())
G_DECLARE_FINAL_TYPE (ShumateVectorSpriteSheet, shumate_vector_sprite_sheet, SHUMATE, VECTOR_SPRITE_SHEET, GObject)

ShumateVectorSpriteSheet *shumate_vector_sprite_sheet_new (void);

void shumate_vector_sprite_sheet_add_sprite (ShumateVectorSpriteSheet *self,
                                             const char               *name,
                                             ShumateVectorSprite      *sprite);
gboolean shumate_vector_sprite_sheet_add_page (ShumateVectorSpriteSheet  *self,
                                               GdkTexture                *texture,
                                               const char                *json,
                                               double                     default_scale,
                                               GError                   **error);

ShumateVectorSprite *shumate_vector_sprite_sheet_get_sprite (ShumateVectorSpriteSheet *self,
                                                             const char               *name,
                                                             double                    scale);

typedef ShumateVectorSprite *(ShumateVectorSpriteFallbackFunc) (ShumateVectorSpriteSheet *sprite_sheet,
                                                                const char               *name,
                                                                double                    scale,
                                                                gpointer                  user_data);

void shumate_vector_sprite_sheet_set_fallback (ShumateVectorSpriteSheet        *self,
                                               ShumateVectorSpriteFallbackFunc  fallback,
                                               gpointer                         user_data,
                                               GDestroyNotify                   notify);

G_END_DECLS
