/*
 * Copyright (C) 2008 Pierre-Luc Beaudoin <pierre-luc@pierlux.com>
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

#if !defined (__SHUMATE_SHUMATE_H_INSIDE__) && !defined (SHUMATE_COMPILATION)
#error "Only <shumate/shumate.h> can be included directly."
#endif

#ifndef SHUMATE_MAP_H
#define SHUMATE_MAP_H

#include <shumate/shumate-layer.h>
#include <shumate/shumate-map-source.h>
#include <shumate/shumate-viewport.h>

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define SHUMATE_TYPE_MAP shumate_map_get_type ()
G_DECLARE_FINAL_TYPE (ShumateMap, shumate_map, SHUMATE, MAP, GtkWidget)

ShumateMap *shumate_map_new (void);
ShumateMap *shumate_map_new_simple (void);

ShumateViewport *shumate_map_get_viewport (ShumateMap *self);

void shumate_map_center_on (ShumateMap *self,
                            double      latitude,
                            double      longitude);

void shumate_map_go_to (ShumateMap *self,
                        double      latitude,
                        double      longitude);
void shumate_map_go_to_full (ShumateMap *self,
                             double      latitude,
                             double      longitude,
                             double      zoom_level);
void shumate_map_go_to_full_with_duration (ShumateMap *self,
                                           double      latitude,
                                           double      longitude,
                                           double      zoom_level,
                                           guint       duration_ms);

void shumate_map_zoom_in (ShumateMap *self);
void shumate_map_zoom_out (ShumateMap *self);

void shumate_map_stop_go_to (ShumateMap *self);
guint shumate_map_get_go_to_duration (ShumateMap *self);
void shumate_map_set_go_to_duration (ShumateMap *self,
                                     guint       duration);
void shumate_map_set_map_source (ShumateMap       *self,
                                 ShumateMapSource *map_source);
void shumate_map_set_zoom_on_double_click (ShumateMap *self,
                                           gboolean    value);
void shumate_map_set_animate_zoom (ShumateMap *self,
                                   gboolean    value);
void shumate_map_add_layer (ShumateMap   *self,
                            ShumateLayer *layer);
void shumate_map_remove_layer (ShumateMap   *self,
                               ShumateLayer *layer);
void shumate_map_insert_layer_behind (ShumateMap   *self,
                                      ShumateLayer *layer,
                                      ShumateLayer *next_sibling);
void shumate_map_insert_layer_above (ShumateMap   *self,
                                     ShumateLayer *layer,
                                     ShumateLayer *next_sibling);
gboolean shumate_map_get_zoom_on_double_click (ShumateMap *self);
gboolean shumate_map_get_animate_zoom (ShumateMap *self);
ShumateState shumate_map_get_state (ShumateMap *self);

G_END_DECLS

#endif
