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

#include <shumate/shumate-defines.h>
#include <shumate/shumate-layer.h>
#include <shumate/shumate-map-source.h>
#include <shumate/shumate-license.h>
#include <shumate/shumate-bounding-box.h>

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

void shumate_view_center_on (ShumateView *view,
    gdouble latitude,
    gdouble longitude);
void shumate_view_go_to (ShumateView *view,
    gdouble latitude,
    gdouble longitude);
void shumate_view_stop_go_to (ShumateView *view);
gdouble shumate_view_get_center_latitude (ShumateView *view);
gdouble shumate_view_get_center_longitude (ShumateView *view);

void shumate_view_zoom_in (ShumateView *view);
void shumate_view_zoom_out (ShumateView *view);
void shumate_view_set_zoom_level (ShumateView *view,
    guint zoom_level);
void shumate_view_set_min_zoom_level (ShumateView *view,
    guint zoom_level);
void shumate_view_set_max_zoom_level (ShumateView *view,
    guint zoom_level);

void shumate_view_ensure_visible (ShumateView *view,
    ShumateBoundingBox *bbox,
    gboolean animate);
void shumate_view_ensure_layers_visible (ShumateView *view,
    gboolean animate);

void shumate_view_set_map_source (ShumateView *view,
    ShumateMapSource *map_source);
void shumate_view_add_overlay_source (ShumateView *view,
    ShumateMapSource *map_source,
    guint8 opacity);
void shumate_view_remove_overlay_source (ShumateView *view,
    ShumateMapSource *map_source);
GList *shumate_view_get_overlay_sources (ShumateView *view);

void shumate_view_set_deceleration (ShumateView *view,
    gdouble rate);
void shumate_view_set_kinetic_mode (ShumateView *view,
    gboolean kinetic);
void shumate_view_set_keep_center_on_resize (ShumateView *view,
    gboolean value);
void shumate_view_set_zoom_on_double_click (ShumateView *view,
    gboolean value);
void shumate_view_set_animate_zoom (ShumateView *view,
    gboolean value);
//void shumate_view_set_background_pattern (ShumateView *view,
//    ClutterContent *background);
void shumate_view_set_world (ShumateView *view,
    ShumateBoundingBox *bbox);
void shumate_view_set_horizontal_wrap (ShumateView *view,
    gboolean wrap);
void shumate_view_add_layer (ShumateView *view,
    ShumateLayer *layer);
void shumate_view_remove_layer (ShumateView *view,
    ShumateLayer *layer);

guint shumate_view_get_zoom_level (ShumateView *view);
guint shumate_view_get_min_zoom_level (ShumateView *view);
guint shumate_view_get_max_zoom_level (ShumateView *view);
ShumateMapSource *shumate_view_get_map_source (ShumateView *view);
gdouble shumate_view_get_deceleration (ShumateView *view);
gboolean shumate_view_get_kinetic_mode (ShumateView *view);
gboolean shumate_view_get_keep_center_on_resize (ShumateView *view);
gboolean shumate_view_get_zoom_on_double_click (ShumateView *view);
gboolean shumate_view_get_animate_zoom (ShumateView *view);
ShumateState shumate_view_get_state (ShumateView *view);
//ClutterContent *shumate_view_get_background_pattern (ShumateView *view);
ShumateBoundingBox *shumate_view_get_world (ShumateView *view);
gboolean shumate_view_get_horizontal_wrap (ShumateView *view);

void shumate_view_reload_tiles (ShumateView *view);

gdouble shumate_view_x_to_longitude (ShumateView *view,
    gdouble x);
gdouble shumate_view_y_to_latitude (ShumateView *view,
    gdouble y);
gdouble shumate_view_longitude_to_x (ShumateView *view,
    gdouble longitude);
gdouble shumate_view_latitude_to_y (ShumateView *view,
    gdouble latitude);

void shumate_view_get_viewport_anchor (ShumateView *view,
    gint *anchor_x,
    gint *anchor_y);
void shumate_view_get_viewport_origin (ShumateView *view,
    gint *x,
    gint *y);

ShumateBoundingBox *shumate_view_get_bounding_box (ShumateView *view);
ShumateBoundingBox *shumate_view_get_bounding_box_for_zoom_level (ShumateView *view,
    guint zoom_level);

G_END_DECLS

#endif
