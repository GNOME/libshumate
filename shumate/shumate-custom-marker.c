/*
 * Copyright (C) 2011-2013 Jiri Techet <techet@gmail.com>
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

/**
 * SECTION:shumate-custom-marker
 * @short_description: A marker implementing the
 * #ClutterContainer interface. Deprecated.
 *
 * A marker implementing the #ClutterContainer interface. You can insert
 * your custom actors into the container. Don't forget to set the marker's
 * pointer position using #clutter_actor_set_translation.
 */

#include "config.h"

#include "shumate.h"
#include "shumate-defines.h"
#include "shumate-marshal.h"
#include "shumate-private.h"

#include <clutter/clutter.h>
#include <glib.h>
#include <glib-object.h>


enum
{
  /* normal signals */
  LAST_SIGNAL
};

enum
{
  PROP_0,
};

struct _ShumateCustomMarkerPrivate
{
  ClutterContainer *dummy;
};

G_DEFINE_TYPE (ShumateCustomMarker, shumate_custom_marker, SHUMATE_TYPE_MARKER)

#define GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), SHUMATE_TYPE_CUSTOM_MARKER, ShumateCustomMarkerPrivate))


static void
shumate_custom_marker_class_init (ShumateCustomMarkerClass *klass)
{
  g_type_class_add_private (klass, sizeof (ShumateCustomMarkerPrivate));
}


static void
shumate_custom_marker_init (ShumateCustomMarker *custom_marker)
{
}


/**
 * shumate_custom_marker_new:
 *
 * Creates an instance of #ShumateCustomMarker.
 *
 * Returns: a new #ShumateCustomMarker.
 *
 * Since: 0.10
 *
 * Deprecated: 0.12.4: #ShumateMarker is a concrete class now and can be used
 * instead.
 */
ClutterActor *
shumate_custom_marker_new (void)
{
  return CLUTTER_ACTOR (g_object_new (SHUMATE_TYPE_CUSTOM_MARKER, NULL));
}
