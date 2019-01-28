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
 * SECTION:shumate-cairo-importable
 * @short_description: An interface for objects able to be set from a cairo surface
 *
 * By implementing #ShumateCairoImportable the object declares that it can be
 * set from a cairo surface (#cairo_surface_t)
 */

#include "shumate-cairo-importable.h"
#include "shumate-private.h"

#include <cairo-gobject.h>

G_DEFINE_INTERFACE (ShumateCairoImportable, shumate_cairo_importable, G_TYPE_OBJECT);


static void
shumate_cairo_importable_default_init (ShumateCairoImportableInterface *iface)
{
}

/**
 * shumate_cairo_importable_set_surface:
 * @importable: the #ShumateCairoImportable
 * @surface: the #cairo_surface_t
 *
 * Set a #cairo_surface_t to be associated with this object.
 */
void
shumate_cairo_importable_set_surface (ShumateCairoImportable *importable,
    cairo_surface_t     *surface)
{
  g_return_if_fail (SHUMATE_CAIRO_IMPORTABLE (importable));
  g_return_if_fail (surface != NULL);

  SHUMATE_CAIRO_IMPORTABLE_GET_IFACE (importable)->set_surface (importable, surface);
}

