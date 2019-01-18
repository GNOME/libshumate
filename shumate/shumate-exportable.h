/*
 * Copyright (C) 2015 Jonas Danielsson
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

#ifndef __SHUMATE_EXPORTABLE_H__
#define __SHUMATE_EXPORTABLE_H__

#include <glib-object.h>
#include <cairo.h>

G_BEGIN_DECLS

#define SHUMATE_TYPE_EXPORTABLE (shumate_exportable_get_type ())

#define SHUMATE_EXPORTABLE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SHUMATE_TYPE_EXPORTABLE, ShumateExportable))

#define SHUMATE_IS_EXPORTABLE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SHUMATE_TYPE_EXPORTABLE))

#define SHUMATE_EXPORTABLE_GET_IFACE(inst) \
  (G_TYPE_INSTANCE_GET_INTERFACE ((inst), SHUMATE_TYPE_EXPORTABLE, ShumateExportableIface))

typedef struct _ShumateExportable ShumateExportable; /* Dummy object */
typedef struct _ShumateExportableIface ShumateExportableIface;

/**
 * ShumateExportable:
 *
 * An interface common to objects having a #cairo_surface_t representation.
 */

/**
 * ShumateExportableIface:
 * @get_surface: virtual function for obtaining the cairo surface.
 * @set_surface: virtual function for setting a cairo surface.
 *
 * An interface common to objects having a #cairo_surface_t representation.
 */
struct _ShumateExportableIface
{
  /*< private >*/
  GTypeInterface g_iface;

  /*< public >*/
  cairo_surface_t *(*get_surface)(ShumateExportable *exportable);
  void (*set_surface)(ShumateExportable *exportable,
      cairo_surface_t *surface);
};

GType shumate_exportable_get_type (void) G_GNUC_CONST;

void shumate_exportable_set_surface (ShumateExportable *exportable,
    cairo_surface_t *surface);
cairo_surface_t * shumate_exportable_get_surface (ShumateExportable *exportable);

G_END_DECLS

#endif /* __SHUMATE_EXPORTABLE_H__ */
