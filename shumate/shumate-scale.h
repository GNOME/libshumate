/*
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

#ifndef SHUMATE_SCALE_H
#define SHUMATE_SCALE_H

#include <shumate/shumate-defines.h>

#include <glib-object.h>

G_BEGIN_DECLS

#define SHUMATE_TYPE_SCALE shumate_scale_get_type ()

#define SHUMATE_SCALE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SHUMATE_TYPE_SCALE, ShumateScale))

#define SHUMATE_SCALE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SHUMATE_TYPE_SCALE, ShumateScaleClass))

#define SHUMATE_IS_SCALE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SHUMATE_TYPE_SCALE))

#define SHUMATE_IS_SCALE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SHUMATE_TYPE_SCALE))

#define SHUMATE_SCALE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), SHUMATE_TYPE_SCALE, ShumateScaleClass))

typedef struct _ShumateScalePrivate ShumateScalePrivate;

typedef struct _ShumateScale ShumateScale;
typedef struct _ShumateScaleClass ShumateScaleClass;

/**
 * ShumateUnit:
 * @SHUMATE_UNIT_KM: kilometers
 * @SHUMATE_UNIT_MILES: miles
 *
 * Units used by the scale.
 */
typedef enum
{
  SHUMATE_UNIT_KM,
  SHUMATE_UNIT_MILES,
} ShumateUnit;

/**
 * ShumateScale:
 *
 * The #ShumateScale structure contains only private data
 * and should be accessed using the provided API
 */
struct _ShumateScale
{
  GObject parent;

  ShumateScalePrivate *priv;
};

struct _ShumateScaleClass
{
  GObjectClass parent_class;
};

GType shumate_scale_get_type (void);

ShumateScale *shumate_scale_new (void);


void shumate_scale_set_max_width (ShumateScale *scale,
    guint value);
void shumate_scale_set_unit (ShumateScale *scale,
    ShumateUnit unit);

guint shumate_scale_get_max_width (ShumateScale *scale);
ShumateUnit shumate_scale_get_unit (ShumateScale *scale);

void shumate_scale_connect_view (ShumateScale *scale,
    ShumateView *view);
void shumate_scale_disconnect_view (ShumateScale *scale);

G_END_DECLS

#endif
