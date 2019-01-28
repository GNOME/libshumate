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
 /**
 * SECTION:shumate-exportable
 * @short_description: An interface for objects exportable to a cairo surface
 *
 * By implementing #ShumateCairoExportable the object declares that it has a cairo
 * surface (#cairo_surface_t) representation of it self.
 */

#include "shumate-cairo-exportable.h"
#include "shumate-private.h"

#include <cairo-gobject.h>

G_DEFINE_INTERFACE (ShumateCairoExportable, shumate_cairo_exportable, G_TYPE_OBJECT);


static void
shumate_cairo_exportable_default_init (ShumateCairoExportableInterface *iface)
{
}

/**
 * shumate_cairo_exportable_get_surface:
 * @exportable: a #ShumateCairoExportable
 *
 * Gets the surface
 *
 * Returns: (transfer none): the #cairo_surface_t of the object
 */
cairo_surface_t *
shumate_cairo_exportable_get_surface (ShumateCairoExportable *exportable)
{
  return SHUMATE_CAIRO_EXPORTABLE_GET_IFACE (exportable)->get_surface (exportable);
}
