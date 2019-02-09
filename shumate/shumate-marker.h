/*
 * Copyright (C) 2008 Pierre-Luc Beaudoin <pierre-luc@pierlux.com>
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

#ifndef SHUMATE_MARKER_H
#define SHUMATE_MARKER_H

#include <shumate/shumate-defines.h>
#include <shumate/shumate-location.h>

#include <gdk/gdk.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define SHUMATE_TYPE_MARKER shumate_marker_get_type ()

#define SHUMATE_MARKER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SHUMATE_TYPE_MARKER, ShumateMarker))

#define SHUMATE_MARKER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SHUMATE_TYPE_MARKER, ShumateMarkerClass))

#define SHUMATE_IS_MARKER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SHUMATE_TYPE_MARKER))

#define SHUMATE_IS_MARKER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SHUMATE_TYPE_MARKER))

#define SHUMATE_MARKER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), SHUMATE_TYPE_MARKER, ShumateMarkerClass))

typedef struct _ShumateMarkerPrivate ShumateMarkerPrivate;

typedef struct _ShumateMarker ShumateMarker;
typedef struct _ShumateMarkerClass ShumateMarkerClass;


/**
 * ShumateMarker:
 *
 * The #ShumateMarker structure contains only private data
 * and should be accessed using the provided API
 */
struct _ShumateMarker
{
  GObject parent;

  ShumateMarkerPrivate *priv;
};

struct _ShumateMarkerClass
{
  GObjectClass parent_class;
};

GType shumate_marker_get_type (void);


ShumateMarker *shumate_marker_new (void);

void shumate_marker_set_selectable (ShumateMarker *marker,
    gboolean value);
gboolean shumate_marker_get_selectable (ShumateMarker *marker);

void shumate_marker_set_draggable (ShumateMarker *marker,
    gboolean value);
gboolean shumate_marker_get_draggable (ShumateMarker *marker);

void shumate_marker_set_selected (ShumateMarker *marker,
    gboolean value);
gboolean shumate_marker_get_selected (ShumateMarker *marker);

void shumate_marker_animate_in (ShumateMarker *marker);
void shumate_marker_animate_in_with_delay (ShumateMarker *marker,
    guint delay);
void shumate_marker_animate_out (ShumateMarker *marker);
void shumate_marker_animate_out_with_delay (ShumateMarker *marker,
    guint delay);

void shumate_marker_set_selection_color (GdkRGBA *color);
const GdkRGBA *shumate_marker_get_selection_color (void);

void shumate_marker_set_selection_text_color (GdkRGBA *color);
const GdkRGBA *shumate_marker_get_selection_text_color (void);

G_END_DECLS

#endif
