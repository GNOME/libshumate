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

#if !defined (__SHUMATE_SHUMATE_H_INSIDE__) && !defined (SHUMATE_COMPILATION)
#error "Only <shumate/shumate.h> can be included directly."
#endif

#ifndef SHUMATE_CUSTOM_MARKER_H
#define SHUMATE_CUSTOM_MARKER_H

#include <shumate/shumate-marker.h>

#include <glib-object.h>
#include <clutter/clutter.h>

G_BEGIN_DECLS

#ifndef GTK_DISABLE_DEPRECATED

#define SHUMATE_TYPE_CUSTOM_MARKER shumate_custom_marker_get_type ()

#define SHUMATE_CUSTOM_MARKER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SHUMATE_TYPE_CUSTOM_MARKER, ShumateCustomMarker))

#define SHUMATE_CUSTOM_MARKER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SHUMATE_TYPE_CUSTOM_MARKER, ShumateCustomMarkerClass))

#define SHUMATE_IS_CUSTOM_MARKER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SHUMATE_TYPE_CUSTOM_MARKER))

#define SHUMATE_IS_CUSTOM_MARKER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SHUMATE_TYPE_CUSTOM_MARKER))

#define SHUMATE_CUSTOM_MARKER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), SHUMATE_TYPE_CUSTOM_MARKER, ShumateCustomMarkerClass))

typedef struct _ShumateCustomMarkerPrivate ShumateCustomMarkerPrivate;

typedef struct _ShumateCustomMarker ShumateCustomMarker;
typedef struct _ShumateCustomMarkerClass ShumateCustomMarkerClass;

/**
 * ShumateCustomMarker:
 *
 * The #ShumateCustomMarker structure contains only private data
 * and should be accessed using the provided API
 *
 * Since: 0.10
 *
 * Deprecated: 0.12.4: #ShumateMarker is a concrete class now and can be used
 * instead.
 */
struct _ShumateCustomMarker
{
  ShumateMarker parent;

  ShumateCustomMarkerPrivate *priv;
};

struct _ShumateCustomMarkerClass
{
  ShumateMarkerClass parent_class;
};

GType shumate_custom_marker_get_type (void);

ClutterActor *shumate_custom_marker_new (void);

#endif

G_END_DECLS

#endif
