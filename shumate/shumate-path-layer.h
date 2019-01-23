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

#ifndef SHUMATE_PATH_LAYER_H
#define SHUMATE_PATH_LAYER_H

#include <shumate/shumate-defines.h>
#include <shumate/shumate-layer.h>
#include <shumate/shumate-location.h>
#include <shumate/shumate-bounding-box.h>

#include <glib-object.h>
#include <clutter/clutter.h>

G_BEGIN_DECLS

#define SHUMATE_TYPE_PATH_LAYER shumate_path_layer_get_type ()

#define SHUMATE_PATH_LAYER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SHUMATE_TYPE_PATH_LAYER, ShumatePathLayer))

#define SHUMATE_PATH_LAYER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SHUMATE_TYPE_PATH_LAYER, ShumatePathLayerClass))

#define SHUMATE_IS_PATH_LAYER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SHUMATE_TYPE_PATH_LAYER))

#define SHUMATE_IS_PATH_LAYER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SHUMATE_TYPE_PATH_LAYER))

#define SHUMATE_PATH_LAYER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), SHUMATE_TYPE_PATH_LAYER, ShumatePathLayerClass))

typedef struct _ShumatePathLayerPrivate ShumatePathLayerPrivate;

typedef struct _ShumatePathLayer ShumatePathLayer;
typedef struct _ShumatePathLayerClass ShumatePathLayerClass;


/**
 * ShumatePathLayer:
 *
 * The #ShumatePathLayer structure contains only private data
 * and should be accessed using the provided API
 */
struct _ShumatePathLayer
{
  ShumateLayer parent;

  ShumatePathLayerPrivate *priv;
};

struct _ShumatePathLayerClass
{
  ShumateLayerClass parent_class;
};

GType shumate_path_layer_get_type (void);

ShumatePathLayer *shumate_path_layer_new (void);

void shumate_path_layer_add_node (ShumatePathLayer *layer,
    ShumateLocation *location);
void shumate_path_layer_remove_node (ShumatePathLayer *layer,
    ShumateLocation *location);
void shumate_path_layer_remove_all (ShumatePathLayer *layer);
void shumate_path_layer_insert_node (ShumatePathLayer *layer,
    ShumateLocation *location,
    guint position);
GList *shumate_path_layer_get_nodes (ShumatePathLayer *layer);

ClutterColor *shumate_path_layer_get_fill_color (ShumatePathLayer *layer);
void shumate_path_layer_set_fill_color (ShumatePathLayer *layer,
    const ClutterColor *color);

ClutterColor *shumate_path_layer_get_stroke_color (ShumatePathLayer *layer);
void shumate_path_layer_set_stroke_color (ShumatePathLayer *layer,
    const ClutterColor *color);

gboolean shumate_path_layer_get_fill (ShumatePathLayer *layer);
void shumate_path_layer_set_fill (ShumatePathLayer *layer,
    gboolean value);

gboolean shumate_path_layer_get_stroke (ShumatePathLayer *layer);
void shumate_path_layer_set_stroke (ShumatePathLayer *layer,
    gboolean value);

gdouble shumate_path_layer_get_stroke_width (ShumatePathLayer *layer);
void shumate_path_layer_set_stroke_width (ShumatePathLayer *layer,
    gdouble value);

gboolean shumate_path_layer_get_visible (ShumatePathLayer *layer);
void shumate_path_layer_set_visible (ShumatePathLayer *layer,
    gboolean value);

gboolean shumate_path_layer_get_closed (ShumatePathLayer *layer);
void shumate_path_layer_set_closed (ShumatePathLayer *layer,
    gboolean value);

GList *shumate_path_layer_get_dash (ShumatePathLayer *layer);
void shumate_path_layer_set_dash (ShumatePathLayer *layer,
    GList *dash_pattern);

G_END_DECLS

#endif
