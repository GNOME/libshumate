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

#if !defined (__SHUMATE_SHUMATE_H_INSIDE__) && !defined (SHUMATE_COMPILATION)
#error "Only <shumate/shumate.h> can be included directly."
#endif

#ifndef SHUMATE_MARKER_ITEM_FACTORY_H
#define SHUMATE_MARKER_ITEM_FACTORY_H

#include <glib-object.h>
#include <shumate/shumate-version.h>

G_BEGIN_DECLS

#define SHUMATE_TYPE_MARKER_ITEM_FACTORY         (shumate_marker_item_factory_get_type ())
#define SHUMATE_MARKER_ITEM_FACTORY(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), SHUMATE_TYPE_MARKER_ITEM_FACTORY, ShumateMarkerItemFactory))
#define SHUMATE_MARKER_ITEM_FACTORY_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), SHUMATE_TYPE_MARKER_ITEM_FACTORY, ShumateMarkerItemFactoryClass))
#define SHUMATE_IS_MARKER_ITEM_FACTORY(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), SHUMATE_TYPE_MARKER_ITEM_FACTORY))
#define SHUMATE_IS_MARKER_ITEM_FACTORY_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), SHUMATE_TYPE_MARKER_ITEM_FACTORY))
#define SHUMATE_MARKER_ITEM_FACTORY_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), SHUMATE_TYPE_MARKER_ITEM_FACTORY, ShumateMarkerItemFactoryClass))

typedef struct _ShumateMarkerItemFactoryClass ShumateMarkerItemFactoryClass;
typedef struct _ShumateMarkerItemFactory ShumateMarkerItemFactory;

SHUMATE_AVAILABLE_IN_1_8
GType        shumate_marker_item_factory_get_type       (void);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(ShumateMarkerItemFactory, g_object_unref)

G_END_DECLS

#endif
