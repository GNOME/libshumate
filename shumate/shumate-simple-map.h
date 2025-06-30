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

#include <gtk/gtk.h>
#include <shumate/shumate-map.h>
#include <shumate/shumate-map-layer.h>
#include <shumate/shumate-map-source.h>
#include <shumate/shumate-layer.h>
#include <shumate/shumate-compass.h>
#include <shumate/shumate-license.h>
#include <shumate/shumate-scale.h>

G_BEGIN_DECLS

#define SHUMATE_TYPE_SIMPLE_MAP (shumate_simple_map_get_type())

G_DECLARE_FINAL_TYPE (ShumateSimpleMap, shumate_simple_map, SHUMATE, SIMPLE_MAP, GtkWidget)

ShumateSimpleMap *shumate_simple_map_new (void);

ShumateViewport  *shumate_simple_map_get_viewport (ShumateSimpleMap *self);

ShumateMapSource *shumate_simple_map_get_map_source (ShumateSimpleMap *self);
void              shumate_simple_map_set_map_source (ShumateSimpleMap *self,
                                                     ShumateMapSource *map_source);

void              shumate_simple_map_add_overlay_layer (ShumateSimpleMap *self,
                                                        ShumateLayer     *layer);
void              shumate_simple_map_insert_overlay_layer_above (ShumateSimpleMap *self,
                                                                 ShumateLayer     *layer,
                                                                 ShumateLayer     *sibling);
void              shumate_simple_map_insert_overlay_layer_behind (ShumateSimpleMap *self,
                                                                  ShumateLayer     *layer,
                                                                  ShumateLayer     *sibling);
void              shumate_simple_map_remove_overlay_layer (ShumateSimpleMap *self,
                                                           ShumateLayer     *layer);

ShumateCompass   *shumate_simple_map_get_compass (ShumateSimpleMap *self);

ShumateLicense   *shumate_simple_map_get_license (ShumateSimpleMap *self);

ShumateScale     *shumate_simple_map_get_scale (ShumateSimpleMap *self);

gboolean          shumate_simple_map_get_show_zoom_buttons (ShumateSimpleMap *self);
void              shumate_simple_map_set_show_zoom_buttons (ShumateSimpleMap *self,
                                                            gboolean          show_zoom_buttons);

ShumateMap *shumate_simple_map_get_map (ShumateSimpleMap *self);

ShumateMapLayer *shumate_simple_map_get_base_map_layer (ShumateSimpleMap *self);

G_END_DECLS
