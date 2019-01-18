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

#ifndef __SHUMATE_LAYER_H__
#define __SHUMATE_LAYER_H__

#include <clutter/clutter.h>
#include <shumate/shumate-defines.h>
#include <shumate/shumate-bounding-box.h>

G_BEGIN_DECLS

#define SHUMATE_TYPE_LAYER shumate_layer_get_type ()

#define SHUMATE_LAYER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SHUMATE_TYPE_LAYER, ShumateLayer))

#define SHUMATE_LAYER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SHUMATE_TYPE_LAYER, ShumateLayerClass))

#define SHUMATE_IS_LAYER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SHUMATE_TYPE_LAYER))

#define SHUMATE_IS_LAYER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SHUMATE_TYPE_LAYER))

#define SHUMATE_LAYER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), SHUMATE_TYPE_LAYER, ShumateLayerClass))

typedef struct _ShumateLayer ShumateLayer;
typedef struct _ShumateLayerClass ShumateLayerClass;

/**
 * ShumateLayer:
 *
 * The #ShumateLayer structure contains only private data
 * and should be accessed using the provided API
 *
 * Since: 0.10
 */
struct _ShumateLayer
{
  ClutterActor parent;
};

struct _ShumateLayerClass
{
  ClutterActorClass parent_class;

  void (*set_view)(ShumateLayer *layer,
      ShumateView *view);
  ShumateBoundingBox * (*get_bounding_box)(ShumateLayer * layer);
};

GType shumate_layer_get_type (void);


void shumate_layer_set_view (ShumateLayer *layer,
    ShumateView *view);

ShumateBoundingBox *shumate_layer_get_bounding_box (ShumateLayer *layer);

G_END_DECLS

#endif /* __SHUMATE_LAYER_H__ */
