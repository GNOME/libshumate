/* shumate-kinetic-scroll-view.h: Finger scrolling container actor
 *
 * Copyright (C) 2008 OpenedHand
 * Copyright (C) 2011-2013 Jiri Techet <techet@gmail.com>
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

#ifndef __SHUMATE_KINETIC_SCROLL_VIEW_H__
#define __SHUMATE_KINETIC_SCROLL_VIEW_H__

#include <glib-object.h>
#include <clutter/clutter.h>
#include <shumate/shumate-viewport.h>

G_BEGIN_DECLS

#define SHUMATE_TYPE_KINETIC_SCROLL_VIEW shumate_kinetic_scroll_view_get_type ()

#define SHUMATE_KINETIC_SCROLL_VIEW(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SHUMATE_TYPE_KINETIC_SCROLL_VIEW, ShumateKineticScrollView))

#define SHUMATE_IS_KINETIC_SCROLL_VIEW(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SHUMATE_TYPE_KINETIC_SCROLL_VIEW))

#define SHUMATE_KINETIC_SCROLL_VIEW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SHUMATE_TYPE_KINETIC_SCROLL_VIEW, ShumateKineticScrollViewClass))

#define SHUMATE_IS_KINETIC_SCROLL_VIEW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SHUMATE_TYPE_KINETIC_SCROLL_VIEW))

#define SHUMATE_KINETIC_SCROLL_VIEW_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), SHUMATE_TYPE_KINETIC_SCROLL_VIEW, ShumateKineticScrollViewClass))

typedef struct _ShumateKineticScrollView ShumateKineticScrollView;
typedef struct _ShumateKineticScrollViewPrivate ShumateKineticScrollViewPrivate;
typedef struct _ShumateKineticScrollViewClass ShumateKineticScrollViewClass;

struct _ShumateKineticScrollView
{
  /*< private >*/
  ClutterActor parent_instance;

  ShumateKineticScrollViewPrivate *priv;
};

struct _ShumateKineticScrollViewClass
{
  ClutterActorClass parent_class;
};

GType shumate_kinetic_scroll_view_get_type (void) G_GNUC_CONST;

ClutterActor *shumate_kinetic_scroll_view_new (gboolean kinetic,
    ShumateViewport *viewport);

void shumate_kinetic_scroll_view_stop (ShumateKineticScrollView *self);

G_END_DECLS

#endif /* __SHUMATE_KINETIC_SCROLL_VIEW_H__ */
