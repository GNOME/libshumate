/* shumate-viewport.h: Viewport actor
 *
 * Copyright (C) 2008 OpenedHand
 * Copyright (C) 2011-2013 Jiri Techet <techet@gmail.com>
 * Copyright (C) 2019 Marcus Lundblad <ml@update.uu.se>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Written by: Chris Lord <chris@openedhand.com>
 */

#ifndef __SHUMATE_VIEWPORT_H__
#define __SHUMATE_VIEWPORT_H__

#include <glib-object.h>
#include <clutter/clutter.h>
#include "shumate-adjustment.h"

G_BEGIN_DECLS

#define SHUMATE_TYPE_VIEWPORT shumate_viewport_get_type ()

#define SHUMATE_VIEWPORT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SHUMATE_TYPE_VIEWPORT, ShumateViewport))

#define SHUMATE_IS_VIEWPORT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SHUMATE_TYPE_VIEWPORT))

#define SHUMATE_VIEWPORT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SHUMATE_TYPE_VIEWPORT, ShumateViewportClass))

#define SHUMATE_IS_VIEWPORT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SHUMATE_TYPE_VIEWPORT))

#define SHUMATE_VIEWPORT_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), SHUMATE_TYPE_VIEWPORT, ShumateViewportClass))

typedef struct _ShumateViewport ShumateViewport;
typedef struct _ShumateViewportPrivate ShumateViewportPrivate;
typedef struct _ShumateViewportClass ShumateViewportClass;

struct _ShumateViewport
{
  ClutterActor parent;

  ShumateViewportPrivate *priv;
};

struct _ShumateViewportClass
{
  ClutterActorClass parent_class;
};

GType shumate_viewport_get_type (void) G_GNUC_CONST;

ClutterActor *shumate_viewport_new (void);

void shumate_viewport_set_origin (ShumateViewport *viewport,
    gdouble x,
    gdouble y);

void shumate_viewport_get_origin (ShumateViewport *viewport,
    gdouble *x,
    gdouble *y);
void shumate_viewport_stop (ShumateViewport *viewport);

void shumate_viewport_get_adjustments (ShumateViewport *viewport,
    ShumateAdjustment **hadjustment,
    ShumateAdjustment **vadjustment);

void shumate_viewport_set_adjustments (ShumateViewport *viewport,
    ShumateAdjustment *hadjustment,
    ShumateAdjustment *vadjustment);

void shumate_viewport_set_child (ShumateViewport *viewport,
    ClutterActor *child);

void shumate_viewport_get_anchor (ShumateViewport *viewport,
    gint *x,
    gint *y);

void shumate_viewport_set_actor_position (ShumateViewport *viewport,
    ClutterActor *actor,
    gdouble x,
    gdouble y);

G_END_DECLS

#endif /* __SHUMATE_VIEWPORT_H__ */
