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

#ifndef SHUMATE_LABEL_H
#define SHUMATE_LABEL_H

#include <shumate/shumate-marker.h>

#include <glib-object.h>
#include <clutter/clutter.h>

G_BEGIN_DECLS

#define SHUMATE_TYPE_LABEL shumate_label_get_type ()

#define SHUMATE_LABEL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SHUMATE_TYPE_LABEL, ShumateLabel))

#define SHUMATE_LABEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SHUMATE_TYPE_LABEL, ShumateLabelClass))

#define SHUMATE_IS_LABEL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SHUMATE_TYPE_LABEL))

#define SHUMATE_IS_LABEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SHUMATE_TYPE_LABEL))

#define SHUMATE_LABEL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), SHUMATE_TYPE_LABEL, ShumateLabelClass))

typedef struct _ShumateLabelPrivate ShumateLabelPrivate;

typedef struct _ShumateLabel ShumateLabel;
typedef struct _ShumateLabelClass ShumateLabelClass;

/**
 * ShumateLabel:
 *
 * The #ShumateLabel structure contains only private data
 * and should be accessed using the provided API
 */
struct _ShumateLabel
{
  ShumateMarker parent;

  ShumateLabelPrivate *priv;
};

struct _ShumateLabelClass
{
  ShumateMarkerClass parent_class;
};

GType shumate_label_get_type (void);

ClutterActor *shumate_label_new (void);

ClutterActor *shumate_label_new_with_text (const gchar *text,
    const gchar *font,
    ClutterColor *text_color,
    ClutterColor *label_color);

ClutterActor *shumate_label_new_with_image (ClutterActor *actor);

ClutterActor *shumate_label_new_from_file (const gchar *filename,
    GError **error);

ClutterActor *shumate_label_new_full (const gchar *text,
    ClutterActor *actor);

void shumate_label_set_text (ShumateLabel *label,
    const gchar *text);
void shumate_label_set_image (ShumateLabel *label,
    ClutterActor *image);
void shumate_label_set_use_markup (ShumateLabel *label,
    gboolean use_markup);
void shumate_label_set_alignment (ShumateLabel *label,
    PangoAlignment alignment);
void shumate_label_set_color (ShumateLabel *label,
    const ClutterColor *color);
void shumate_label_set_text_color (ShumateLabel *label,
    const ClutterColor *color);
void shumate_label_set_font_name (ShumateLabel *label,
    const gchar *font_name);
void shumate_label_set_wrap (ShumateLabel *label,
    gboolean wrap);
void shumate_label_set_wrap_mode (ShumateLabel *label,
    PangoWrapMode wrap_mode);
void shumate_label_set_attributes (ShumateLabel *label,
    PangoAttrList *list);
void shumate_label_set_single_line_mode (ShumateLabel *label,
    gboolean mode);
void shumate_label_set_ellipsize (ShumateLabel *label,
    PangoEllipsizeMode mode);
void shumate_label_set_draw_background (ShumateLabel *label,
    gboolean background);
void shumate_label_set_draw_shadow (ShumateLabel *label,
    gboolean shadow);

gboolean shumate_label_get_use_markup (ShumateLabel *label);
const gchar *shumate_label_get_text (ShumateLabel *label);
ClutterActor *shumate_label_get_image (ShumateLabel *label);
PangoAlignment shumate_label_get_alignment (ShumateLabel *label);
ClutterColor *shumate_label_get_color (ShumateLabel *label);
ClutterColor *shumate_label_get_text_color (ShumateLabel *label);
const gchar *shumate_label_get_font_name (ShumateLabel *label);
gboolean shumate_label_get_wrap (ShumateLabel *label);
PangoWrapMode shumate_label_get_wrap_mode (ShumateLabel *label);
PangoEllipsizeMode shumate_label_get_ellipsize (ShumateLabel *label);
gboolean shumate_label_get_single_line_mode (ShumateLabel *label);
gboolean shumate_label_get_draw_background (ShumateLabel *label);
gboolean shumate_label_get_draw_shadow (ShumateLabel *label);
PangoAttrList *shumate_label_get_attributes (ShumateLabel *label);


G_END_DECLS

#endif
