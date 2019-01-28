/*
 * Copyright (C) 2015 Jonas Danielsson
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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#if !defined (__SHUMATE_SHUMATE_H_INSIDE__) && !defined (SHUMATE_COMPILATION)
#error "Only <shumate/shumate.h> can be included directly."
#endif

#ifndef __SHUMATE_CAIRO_EXPORTABLE_H__
#define __SHUMATE_CAIRO_EXPORTABLE_H__

#include <glib-object.h>
#include <cairo.h>

G_BEGIN_DECLS

#define SHUMATE_TYPE_CAIRO_EXPORTABLE (shumate_cairo_exportable_get_type ())

G_DECLARE_INTERFACE (ShumateCairoExportable, shumate_cairo_exportable, SHUMATE, CAIRO_EXPORTABLE, GObject)

/**
 * ShumateCairoExportable:
 *
 * An interface common to objects able to be exported to a #cairo_surface_t.
 */

/**
 * ShumateCairoExportableInterface:
 * @get_surface: virtual function for obtaining the cairo surface.
 *
 * An interface common to objects able to be exported to a #cairo_surface_t.
 */
struct _ShumateCairoExportableInterface
{
  /*< private >*/
  GTypeInterface g_iface;

  /*< public >*/
  cairo_surface_t *(*get_surface)(ShumateCairoExportable *exportable);
};

cairo_surface_t * shumate_cairo_exportable_get_surface (ShumateCairoExportable *exportable);

G_END_DECLS

#endif /* _SHUMATE_CAIRO_EXPORTABLE_H__ */
