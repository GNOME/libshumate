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

#include <shumate/shumate-defines.h>
#include <shumate/shumate-marker.h>
#include <shumate/shumate-layer.h>
#include <shumate/shumate-bounding-box.h>

#include <glib-object.h>

G_BEGIN_DECLS

#define SHUMATE_TYPE_MARKER_LAYER shumate_marker_layer_get_type ()

#define SHUMATE_MARKER_LAYER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SHUMATE_TYPE_MARKER_LAYER, ShumateMarkerLayer))

#define SHUMATE_MARKER_LAYER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SHUMATE_TYPE_MARKER_LAYER, ShumateMarkerLayerClass))

#define SHUMATE_IS_MARKER_LAYER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SHUMATE_TYPE_MARKER_LAYER))

#define SHUMATE_IS_MARKER_LAYER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SHUMATE_TYPE_MARKER_LAYER))

#define SHUMATE_MARKER_LAYER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), SHUMATE_TYPE_MARKER_LAYER, ShumateMarkerLayerClass))

typedef struct _ShumateMarkerLayerPrivate ShumateMarkerLayerPrivate;

typedef struct _ShumateMarkerLayer ShumateMarkerLayer;
typedef struct _ShumateMarkerLayerClass ShumateMarkerLayerClass;

/**
 * ShumateSelectionMode:
 * @SHUMATE_SELECTION_NONE: No marker can be selected.
 * @SHUMATE_SELECTION_SINGLE: Only one marker can be selected.
 * @SHUMATE_SELECTION_MULTIPLE: Multiple marker can be selected.
 *
 * Selection mode
 */
typedef enum
{
  SHUMATE_SELECTION_NONE,
  SHUMATE_SELECTION_SINGLE,
  SHUMATE_SELECTION_MULTIPLE
} ShumateSelectionMode;

/**
 * ShumateMarkerLayer:
 *
 * The #ShumateMarkerLayer structure contains only private data
 * and should be accessed using the provided API
 */
struct _ShumateMarkerLayer
{
  ShumateLayer parent;

  ShumateMarkerLayerPrivate *priv;
};

struct _ShumateMarkerLayerClass
{
  ShumateLayerClass parent_class;
};

GType shumate_marker_layer_get_type (void);

ShumateMarkerLayer *shumate_marker_layer_new (void);
ShumateMarkerLayer *shumate_marker_layer_new_full (ShumateSelectionMode mode);

void shumate_marker_layer_add_marker (ShumateMarkerLayer *layer,
    ShumateMarker *marker);
void shumate_marker_layer_remove_marker (ShumateMarkerLayer *layer,
    ShumateMarker *marker);
void shumate_marker_layer_remove_all (ShumateMarkerLayer *layer);
GList *shumate_marker_layer_get_markers (ShumateMarkerLayer *layer);
GList *shumate_marker_layer_get_selected (ShumateMarkerLayer *layer);

void shumate_marker_layer_animate_in_all_markers (ShumateMarkerLayer *layer);
void shumate_marker_layer_animate_out_all_markers (ShumateMarkerLayer *layer);

void shumate_marker_layer_show_all_markers (ShumateMarkerLayer *layer);
void shumate_marker_layer_hide_all_markers (ShumateMarkerLayer *layer);

void shumate_marker_layer_set_all_markers_draggable (ShumateMarkerLayer *layer);
void shumate_marker_layer_set_all_markers_undraggable (ShumateMarkerLayer *layer);

void shumate_marker_layer_select_all_markers (ShumateMarkerLayer *layer);
void shumate_marker_layer_unselect_all_markers (ShumateMarkerLayer *layer);

void shumate_marker_layer_set_selection_mode (ShumateMarkerLayer *layer,
    ShumateSelectionMode mode);
ShumateSelectionMode shumate_marker_layer_get_selection_mode (ShumateMarkerLayer *layer);

G_END_DECLS

#endif

