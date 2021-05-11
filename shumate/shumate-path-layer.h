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

#ifndef SHUMATE_PATH_LAYER_H
#define SHUMATE_PATH_LAYER_H

#include <shumate/shumate-layer.h>
#include <shumate/shumate-location.h>

#include <gdk/gdk.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define SHUMATE_TYPE_PATH_LAYER shumate_path_layer_get_type ()
G_DECLARE_DERIVABLE_TYPE (ShumatePathLayer, shumate_path_layer, SHUMATE, PATH_LAYER, ShumateLayer)

/**
 * ShumatePathLayer:
 *
 * The #ShumatePathLayer structure contains only private data
 * and should be accessed using the provided API
 */

struct _ShumatePathLayerClass
{
  ShumateLayerClass parent_class;
};

ShumatePathLayer *shumate_path_layer_new (ShumateViewport *viewport);

void shumate_path_layer_add_node (ShumatePathLayer *layer,
    ShumateLocation *location);
void shumate_path_layer_remove_node (ShumatePathLayer *layer,
    ShumateLocation *location);
void shumate_path_layer_remove_all (ShumatePathLayer *layer);
void shumate_path_layer_insert_node (ShumatePathLayer *layer,
    ShumateLocation *location,
    guint position);
GList *shumate_path_layer_get_nodes (ShumatePathLayer *layer);

GdkRGBA *shumate_path_layer_get_fill_color (ShumatePathLayer *layer);
void shumate_path_layer_set_fill_color (ShumatePathLayer *layer,
    const GdkRGBA *color);

GdkRGBA *shumate_path_layer_get_stroke_color (ShumatePathLayer *layer);
void shumate_path_layer_set_stroke_color (ShumatePathLayer *layer,
    const GdkRGBA *color);

GdkRGBA *shumate_path_layer_get_outline_color (ShumatePathLayer *layer);
void shumate_path_layer_set_outline_color (ShumatePathLayer *layer,
    const GdkRGBA *color);

gboolean shumate_path_layer_get_fill (ShumatePathLayer *layer);
void shumate_path_layer_set_fill (ShumatePathLayer *layer,
    gboolean value);

gboolean shumate_path_layer_get_stroke (ShumatePathLayer *layer);
void shumate_path_layer_set_stroke (ShumatePathLayer *layer,
    gboolean value);

double shumate_path_layer_get_stroke_width (ShumatePathLayer *layer);
void shumate_path_layer_set_stroke_width (ShumatePathLayer *layer,
    double value);

double shumate_path_layer_get_outline_width (ShumatePathLayer *layer);
void shumate_path_layer_set_outline_width (ShumatePathLayer *layer,
    double value);

gboolean shumate_path_layer_get_closed (ShumatePathLayer *layer);
void shumate_path_layer_set_closed (ShumatePathLayer *layer,
    gboolean value);

GList *shumate_path_layer_get_dash (ShumatePathLayer *layer);
void shumate_path_layer_set_dash (ShumatePathLayer *layer,
    GList *dash_pattern);

G_END_DECLS

#endif

