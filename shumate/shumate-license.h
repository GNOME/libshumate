/*
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

#ifndef SHUMATE_LICENSE_H
#define SHUMATE_LICENSE_H

#include <shumate/shumate-map-source.h>

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define SHUMATE_TYPE_LICENSE shumate_license_get_type ()
G_DECLARE_FINAL_TYPE (ShumateLicense, shumate_license, SHUMATE, LICENSE, GtkWidget)

ShumateLicense *shumate_license_new (void);

void shumate_license_set_extra_text (ShumateLicense *license,
                                     const char     *text);
const char *shumate_license_get_extra_text (ShumateLicense *license);

void shumate_license_set_xalign (ShumateLicense *license,
                                 float           xalign);
float shumate_license_get_xalign (ShumateLicense *license);

void shumate_license_append_map_source (ShumateLicense   *license,
                                        ShumateMapSource *map_source);
void shumate_license_prepend_map_source (ShumateLicense   *license,
                                         ShumateMapSource *map_source);
void shumate_license_remove_map_source (ShumateLicense   *license,
                                        ShumateMapSource *map_source);

G_END_DECLS

#endif
