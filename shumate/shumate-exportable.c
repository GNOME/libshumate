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
 * By implementing #ShumateExportable the object declares that it has a cairo
 * surface (#cairo_surface_t) representation of it self.
 */

#include "shumate-exportable.h"
#include "shumate-private.h"

#include <cairo-gobject.h>

typedef ShumateExportableIface ShumateExportableInterface;

G_DEFINE_INTERFACE (ShumateExportable, shumate_exportable, G_TYPE_OBJECT);


static void
shumate_exportable_default_init (ShumateExportableInterface *iface)
{
  /**
   * ShumateExportable:surface:
   *
   * A #cairo_surface_t representation
   *
   * Since: 0.12.12
   */
  g_object_interface_install_property (iface,
      g_param_spec_boxed ("surface",
          "Surface",
          "Cairo surface representaion",
          CAIRO_GOBJECT_TYPE_SURFACE,
          G_PARAM_READWRITE));
}


/**
 * shumate_exportable_set_surface:
 * @exportable: the #ShumateExportable
 * @surface: the #cairo_surface_t
 *
 * Set a #cairo_surface_t to be associated with this tile.
 *
 * Since: 0.12.12
 */
void
shumate_exportable_set_surface (ShumateExportable *exportable,
    cairo_surface_t     *surface)
{
  g_return_if_fail (SHUMATE_EXPORTABLE (exportable));
  g_return_if_fail (surface != NULL);

  SHUMATE_EXPORTABLE_GET_IFACE (exportable)->set_surface (exportable, surface);
}


/**
 * shumate_exportable_get_surface:
 * @exportable: a #ShumateExportable
 *
 * Gets the surface
 *
 * Returns: (transfer none): the #cairo_surface_t of the object
 *
 * Since: 0.12.12
 */
cairo_surface_t *
shumate_exportable_get_surface (ShumateExportable *exportable)
{
  return SHUMATE_EXPORTABLE_GET_IFACE (exportable)->get_surface (exportable);
}
