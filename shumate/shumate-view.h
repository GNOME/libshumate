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

#ifndef SHUMATE_VIEW_H
#define SHUMATE_VIEW_H

#include <shumate/shumate-layer.h>
#include <shumate/shumate-map-source.h>
#include <shumate/shumate-license.h>
#include <shumate/shumate-viewport.h>

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
//#include <clutter/clutter.h>

G_BEGIN_DECLS

#define SHUMATE_TYPE_VIEW shumate_view_get_type ()
G_DECLARE_DERIVABLE_TYPE (ShumateView, shumate_view, SHUMATE, VIEW, GtkWidget)

/**
 * ShumateView:
 *
 * The #ShumateView structure contains only private data
 * and should be accessed using the provided API
 */

struct _ShumateViewClass
{
  GtkWidgetClass parent_class;
};

ShumateView *shumate_view_new (void);
ShumateView *shumate_view_new_simple (void);

ShumateViewport *shumate_view_get_viewport (ShumateView *self);

void shumate_view_center_on (ShumateView *view,
    double latitude,
    double longitude);
void shumate_view_go_to (ShumateView *view,
    double latitude,
    double longitude);
void shumate_view_stop_go_to (ShumateView *view);

void shumate_view_set_map_source (ShumateView *view,
    ShumateMapSource *map_source);
void shumate_view_add_overlay_source (ShumateView *view,
    ShumateMapSource *map_source);
void shumate_view_remove_overlay_source (ShumateView *view,
    ShumateMapSource *map_source);
GList *shumate_view_get_overlay_sources (ShumateView *view);

void shumate_view_set_deceleration (ShumateView *view,
    double rate);
void shumate_view_set_kinetic_mode (ShumateView *view,
    gboolean kinetic);
void shumate_view_set_zoom_on_double_click (ShumateView *view,
    gboolean value);
void shumate_view_set_animate_zoom (ShumateView *view,
    gboolean value);
void shumate_view_add_layer (ShumateView *view,
    ShumateLayer *layer);
void shumate_view_remove_layer (ShumateView *view,
    ShumateLayer *layer);

double shumate_view_get_deceleration (ShumateView *view);
gboolean shumate_view_get_kinetic_mode (ShumateView *view);
gboolean shumate_view_get_zoom_on_double_click (ShumateView *view);
gboolean shumate_view_get_animate_zoom (ShumateView *view);
ShumateState shumate_view_get_state (ShumateView *view);

G_END_DECLS

#endif
