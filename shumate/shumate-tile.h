/*
 * Copyright (C) 2008-2009 Pierre-Luc Beaudoin <pierre-luc@pierlux.com>
 * Copyright (C) 2010-2013 Jiri Techet <techet@gmail.com>
 * Copyright (C) 2019 Marcus Lundblad <ml@update.uu.se>
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

#ifndef SHUMATE_MAP_TILE_H
#define SHUMATE_MAP_TILE_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define SHUMATE_TYPE_TILE shumate_tile_get_type ()
G_DECLARE_DERIVABLE_TYPE (ShumateTile, shumate_tile, SHUMATE, TILE, GtkWidget)

/**
 * ShumateState:
 * @SHUMATE_STATE_NONE: Initial or undefined state
 * @SHUMATE_STATE_LOADING: Tile is loading
 * @SHUMATE_STATE_LOADED: Tile is loaded but not yet displayed
 * @SHUMATE_STATE_DONE: Tile loading finished. Also used to inform map sources
 *     that tile loading has been cancelled.
 *
 * Tile loading state.
 */
typedef enum
{
  SHUMATE_STATE_NONE,
  SHUMATE_STATE_LOADING,
  SHUMATE_STATE_LOADED,
  SHUMATE_STATE_DONE
} ShumateState;

struct _ShumateTileClass
{
  GtkWidgetClass parent_class;
};

ShumateTile *shumate_tile_new (void);
ShumateTile *shumate_tile_new_full (guint x,
                                    guint y,
                                    guint size,
                                    guint zoom_level);

guint shumate_tile_get_x (ShumateTile *self);
void shumate_tile_set_x (ShumateTile *self,
                         guint        x);

guint shumate_tile_get_y (ShumateTile *self);
void shumate_tile_set_y (ShumateTile *self,
                         guint        y);

guint shumate_tile_get_zoom_level (ShumateTile *self);
void shumate_tile_set_zoom_level (ShumateTile *self,
                                  guint        zoom_level);

guint shumate_tile_get_size (ShumateTile *self);
void shumate_tile_set_size (ShumateTile *self,
                            guint size);

ShumateState shumate_tile_get_state (ShumateTile *self);
void shumate_tile_set_state (ShumateTile *self,
                             ShumateState state);

GDateTime *shumate_tile_get_modified_time (ShumateTile *self);
void shumate_tile_set_modified_time (ShumateTile *self,
                                     GDateTime   *modified_time);

const char *shumate_tile_get_etag (ShumateTile *self);
void shumate_tile_set_etag (ShumateTile *self,
                            const char  *etag);

gboolean shumate_tile_get_fade_in (ShumateTile *self);
void shumate_tile_set_fade_in (ShumateTile *self,
                               gboolean     fade_in);

GdkTexture *shumate_tile_get_texture (ShumateTile *self);
void shumate_tile_set_texture (ShumateTile *self,
                               GdkTexture  *texture);
G_END_DECLS

#endif /* SHUMATE_MAP_TILE_H */
