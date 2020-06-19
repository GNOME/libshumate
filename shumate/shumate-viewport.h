/* shumate-viewport.h: Viewport actor
 *
 * Copyright (C) 2008 OpenedHand
 * Copyright (C) 2011-2013 Jiri Techet <techet@gmail.com>
 * Copyright (C) 2019 Marcus Lundblad <ml@update.uu.se>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Written by: Chris Lord <chris@openedhand.com>
 */

#ifndef __SHUMATE_VIEWPORT_H__
#define __SHUMATE_VIEWPORT_H__

#include <glib-object.h>

#include <shumate/shumate-location.h>
#include <shumate/shumate-map-source.h>

G_BEGIN_DECLS

#define SHUMATE_TYPE_VIEWPORT shumate_viewport_get_type ()
G_DECLARE_FINAL_TYPE (ShumateViewport, shumate_viewport, SHUMATE, VIEWPORT, GObject)

ShumateViewport *shumate_viewport_new (void);

void shumate_viewport_set_zoom_level (ShumateViewport *self,
                                      guint            zoom_level);
guint shumate_viewport_get_zoom_level (ShumateViewport *self);

void shumate_viewport_set_max_zoom_level (ShumateViewport *self,
                                          guint            max_zoom_level);
guint shumate_viewport_get_max_zoom_level (ShumateViewport *self);

void shumate_viewport_set_min_zoom_level (ShumateViewport *self,
                                          guint            min_zoom_level);
guint shumate_viewport_get_min_zoom_level (ShumateViewport *self);

void shumate_viewport_zoom_in (ShumateViewport *self);
void shumate_viewport_zoom_out (ShumateViewport *self);

void shumate_viewport_set_reference_map_source (ShumateViewport  *self,
                                                ShumateMapSource *map_source);
ShumateMapSource *shumate_viewport_get_reference_map_source (ShumateViewport  *self);

gdouble shumate_viewport_widget_x_to_longitude (ShumateViewport *self,
                                                GtkWidget       *widget,
                                                gdouble          x);
gdouble shumate_viewport_widget_y_to_latitude (ShumateViewport *self,
                                               GtkWidget       *widget,
                                               gdouble          y);
gdouble shumate_viewport_longitude_to_widget_x (ShumateViewport *self,
                                                GtkWidget       *widget,
                                                gdouble          longitude);
gdouble shumate_viewport_latitude_to_widget_y (ShumateViewport *self,
                                               GtkWidget       *widget,
                                               gdouble          latitude);

G_END_DECLS

#endif /* __SHUMATE_VIEWPORT_H__ */
