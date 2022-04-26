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
                                      double           zoom_level);
double shumate_viewport_get_zoom_level (ShumateViewport *self);

void shumate_viewport_set_max_zoom_level (ShumateViewport *self,
                                          guint            max_zoom_level);
guint shumate_viewport_get_max_zoom_level (ShumateViewport *self);

void shumate_viewport_set_min_zoom_level (ShumateViewport *self,
                                          guint            min_zoom_level);
guint shumate_viewport_get_min_zoom_level (ShumateViewport *self);

void shumate_viewport_set_reference_map_source (ShumateViewport  *self,
                                                ShumateMapSource *map_source);
ShumateMapSource *shumate_viewport_get_reference_map_source (ShumateViewport  *self);

void shumate_viewport_set_rotation (ShumateViewport *self,
                                    double           rotation);
double shumate_viewport_get_rotation (ShumateViewport *self);

void shumate_viewport_widget_coords_to_location (ShumateViewport *self,
                                                 GtkWidget       *widget,
                                                 double           x,
                                                 double           y,
                                                 double          *latitude,
                                                 double          *longitude);
void shumate_viewport_location_to_widget_coords (ShumateViewport *self,
                                                 GtkWidget       *widget,
                                                 double           latitude,
                                                 double           longitude,
                                                 double          *x,
                                                 double          *y);

G_END_DECLS

#endif /* __SHUMATE_VIEWPORT_H__ */
