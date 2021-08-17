/*
 * Copyright (C) 2021 Collabora Ltd. (https://www.collabora.com)
 * Copyright (C) 2021 Corentin Noël <corentin.noel@collabora.com>
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

#ifndef SHUMATE_COMPASS_H
#define SHUMATE_COMPASS_H

#include <shumate/shumate-viewport.h>

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define SHUMATE_TYPE_COMPASS (shumate_compass_get_type())

G_DECLARE_FINAL_TYPE (ShumateCompass, shumate_compass, SHUMATE, COMPASS, GtkWidget)

ShumateCompass *shumate_compass_new (ShumateViewport *viewport);

ShumateViewport *shumate_compass_get_viewport (ShumateCompass *compass);
void shumate_compass_set_viewport (ShumateCompass  *compass,
                                   ShumateViewport *viewport);

G_END_DECLS

#endif
