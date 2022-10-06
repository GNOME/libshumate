/*
 * Copyright (C) 2008 Pierre-Luc Beaudoin <pierre-luc@pierlux.com>
 * Copyright (C) 2011-2013 Jiri Techet <techet@gmail.com>
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

#ifndef SHUMATE_MARKER_LAYER_H
#define SHUMATE_MARKER_LAYER_H

#include <shumate/shumate-marker.h>
#include <shumate/shumate-layer.h>
#include <shumate/shumate-viewport.h>

#include <gtk/gtk.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define SHUMATE_TYPE_MARKER_LAYER shumate_marker_layer_get_type ()
G_DECLARE_FINAL_TYPE (ShumateMarkerLayer, shumate_marker_layer, SHUMATE, MARKER_LAYER, ShumateLayer)

ShumateMarkerLayer *shumate_marker_layer_new (ShumateViewport *viewport);
ShumateMarkerLayer *shumate_marker_layer_new_full (ShumateViewport *viewport,
                                                   GtkSelectionMode mode);

void shumate_marker_layer_add_marker (ShumateMarkerLayer *self,
    ShumateMarker *marker);
void shumate_marker_layer_remove_marker (ShumateMarkerLayer *self,
    ShumateMarker *marker);
void shumate_marker_layer_remove_all (ShumateMarkerLayer *self);
GList *shumate_marker_layer_get_markers (ShumateMarkerLayer *self);
GList *shumate_marker_layer_get_selected (ShumateMarkerLayer *self);

gboolean shumate_marker_layer_select_marker (ShumateMarkerLayer *self, ShumateMarker *marker);
void shumate_marker_layer_unselect_marker (ShumateMarkerLayer *self, ShumateMarker *marker);
void shumate_marker_layer_select_all_markers (ShumateMarkerLayer *self);
void shumate_marker_layer_unselect_all_markers (ShumateMarkerLayer *self);

void shumate_marker_layer_set_selection_mode (ShumateMarkerLayer *self,
                                              GtkSelectionMode    mode);
GtkSelectionMode shumate_marker_layer_get_selection_mode (ShumateMarkerLayer *self);

G_END_DECLS

#endif

