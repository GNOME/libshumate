/*
 * Copyright 2026 Corentin Noël <corentin.noel@collabora.com>
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

#include "shumate-marker-item-factory-private.h"

G_DEFINE_TYPE (ShumateMarkerItemFactory, shumate_marker_item_factory, G_TYPE_OBJECT)

ShumateMarkerItemFactory *
shumate_marker_item_factory_new (void)
{
  return g_object_new (SHUMATE_TYPE_MARKER_ITEM_FACTORY, NULL);
}

static void
shumate_marker_item_factory_class_init (ShumateMarkerItemFactoryClass *klass)
{
}

static void
shumate_marker_item_factory_init (ShumateMarkerItemFactory *self)
{

}
