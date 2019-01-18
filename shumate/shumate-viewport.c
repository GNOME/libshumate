/* shumate-viewport.c: Viewport actor
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

#include "config.h"

#include <clutter/clutter.h>

#include "shumate-viewport.h"
#include "shumate-private.h"

G_DEFINE_TYPE (ShumateViewport, shumate_viewport, CLUTTER_TYPE_ACTOR)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), SHUMATE_TYPE_VIEWPORT, ShumateViewportPrivate))

struct _ShumateViewportPrivate
{
  gdouble x;
  gdouble y;

  gint anchor_x;
  gint anchor_y;

  ShumateAdjustment *hadjustment;
  ShumateAdjustment *vadjustment;

  ClutterActor *child;
};

enum
{
  PROP_0,

  PROP_X_ORIGIN,
  PROP_Y_ORIGIN,
  PROP_HADJUST,
  PROP_VADJUST,
};

enum
{
  /* normal signals */
  RELOCATED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };


static void
shumate_viewport_get_property (GObject *object,
    guint prop_id,
    GValue *value,
    GParamSpec *pspec)
{
  ShumateAdjustment *adjustment;

  ShumateViewportPrivate *priv = SHUMATE_VIEWPORT (object)->priv;

  switch (prop_id)
    {
    case PROP_X_ORIGIN:
      g_value_set_int (value, priv->x);
      break;

    case PROP_Y_ORIGIN:
      g_value_set_int (value, priv->y);
      break;

    case PROP_HADJUST:
      shumate_viewport_get_adjustments (SHUMATE_VIEWPORT (object), &adjustment, NULL);
      g_value_set_object (value, adjustment);
      break;

    case PROP_VADJUST:
      shumate_viewport_get_adjustments (SHUMATE_VIEWPORT (object), NULL, &adjustment);
      g_value_set_object (value, adjustment);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


static void
shumate_viewport_set_property (GObject *object,
    guint prop_id,
    const GValue *value,
    GParamSpec *pspec)
{
  ShumateViewport *viewport = SHUMATE_VIEWPORT (object);
  ShumateViewportPrivate *priv = viewport->priv;

  switch (prop_id)
    {
    case PROP_X_ORIGIN:
      shumate_viewport_set_origin (viewport,
          g_value_get_int (value),
          priv->y);
      break;

    case PROP_Y_ORIGIN:
      shumate_viewport_set_origin (viewport,
          priv->x,
          g_value_get_int (value));
      break;

    case PROP_HADJUST:
      shumate_viewport_set_adjustments (SHUMATE_VIEWPORT (object),
          g_value_get_object (value),
          priv->vadjustment);
      break;

    case PROP_VADJUST:
      shumate_viewport_set_adjustments (SHUMATE_VIEWPORT (object),
          priv->hadjustment,
          g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


void
shumate_viewport_stop (ShumateViewport *viewport)
{
  ShumateViewportPrivate *priv = SHUMATE_VIEWPORT (viewport)->priv;

  if (priv->hadjustment)
    shumate_adjustment_interpolate_stop (priv->hadjustment);

  if (priv->vadjustment)
    shumate_adjustment_interpolate_stop (priv->vadjustment);
}


static void
shumate_viewport_dispose (GObject *gobject)
{
  ShumateViewportPrivate *priv = SHUMATE_VIEWPORT (gobject)->priv;

  if (priv->hadjustment)
    {
      shumate_adjustment_interpolate_stop (priv->hadjustment);
      g_object_unref (priv->hadjustment);
      priv->hadjustment = NULL;
    }

  if (priv->vadjustment)
    {
      shumate_adjustment_interpolate_stop (priv->vadjustment);
      g_object_unref (priv->vadjustment);
      priv->vadjustment = NULL;
    }

  G_OBJECT_CLASS (shumate_viewport_parent_class)->dispose (gobject);
}


static void
shumate_viewport_class_init (ShumateViewportClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (ShumateViewportPrivate));

  gobject_class->get_property = shumate_viewport_get_property;
  gobject_class->set_property = shumate_viewport_set_property;
  gobject_class->dispose = shumate_viewport_dispose;

  g_object_class_install_property (gobject_class,
      PROP_X_ORIGIN,
      g_param_spec_int ("x-origin",
          "X Origin",
          "Origin's X coordinate in pixels",
          -G_MAXINT, G_MAXINT,
          0,
          G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
      PROP_Y_ORIGIN,
      g_param_spec_int ("y-origin",
          "Y Origin",
          "Origin's Y coordinate in pixels",
          -G_MAXINT, G_MAXINT,
          0,
          G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
      PROP_HADJUST,
      g_param_spec_object ("hadjustment",
          "ShumateAdjustment",
          "Horizontal adjustment",
          SHUMATE_TYPE_ADJUSTMENT,
          G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
      PROP_VADJUST,
      g_param_spec_object ("vadjustment",
          "ShumateAdjustment",
          "Vertical adjustment",
          SHUMATE_TYPE_ADJUSTMENT,
          G_PARAM_READWRITE));

  signals[RELOCATED] =
    g_signal_new ("relocated",
        G_OBJECT_CLASS_TYPE (gobject_class),
        G_SIGNAL_RUN_LAST,
        0, NULL, NULL,
        g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE,
        0);
}


static void
hadjustment_value_notify_cb (ShumateAdjustment *adjustment,
    GParamSpec *pspec,
    ShumateViewport *viewport)
{
  ShumateViewportPrivate *priv = viewport->priv;
  gdouble value;

  value = shumate_adjustment_get_value (adjustment);

  if (priv->x != value)
    shumate_viewport_set_origin (viewport, value, priv->y);
}


static void
vadjustment_value_notify_cb (ShumateAdjustment *adjustment, GParamSpec *arg1,
    ShumateViewport *viewport)
{
  ShumateViewportPrivate *priv = viewport->priv;
  gdouble value;

  value = shumate_adjustment_get_value (adjustment);

  if (priv->y != value)
    shumate_viewport_set_origin (viewport, priv->x, value);
}


void
shumate_viewport_set_adjustments (ShumateViewport *viewport,
    ShumateAdjustment *hadjustment,
    ShumateAdjustment *vadjustment)
{
  ShumateViewportPrivate *priv = SHUMATE_VIEWPORT (viewport)->priv;

  if (hadjustment != priv->hadjustment)
    {
      if (priv->hadjustment)
        {
          g_signal_handlers_disconnect_by_func (priv->hadjustment,
              hadjustment_value_notify_cb,
              viewport);
          g_object_unref (priv->hadjustment);
        }

      if (hadjustment)
        {
          g_object_ref (hadjustment);
          g_signal_connect (hadjustment, "notify::value",
              G_CALLBACK (hadjustment_value_notify_cb),
              viewport);
        }

      priv->hadjustment = hadjustment;
    }

  if (vadjustment != priv->vadjustment)
    {
      if (priv->vadjustment)
        {
          g_signal_handlers_disconnect_by_func (priv->vadjustment,
              vadjustment_value_notify_cb,
              viewport);
          g_object_unref (priv->vadjustment);
        }

      if (vadjustment)
        {
          g_object_ref (vadjustment);
          g_signal_connect (vadjustment, "notify::value",
              G_CALLBACK (vadjustment_value_notify_cb),
              viewport);
        }

      priv->vadjustment = vadjustment;
    }
}


void
shumate_viewport_get_adjustments (ShumateViewport *viewport,
    ShumateAdjustment **hadjustment,
    ShumateAdjustment **vadjustment)
{
  ShumateViewportPrivate *priv;

  g_return_if_fail (SHUMATE_IS_VIEWPORT (viewport));

  priv = ((ShumateViewport *) viewport)->priv;

  if (hadjustment)
    {
      if (priv->hadjustment)
        *hadjustment = priv->hadjustment;
      else
        {
          ShumateAdjustment *adjustment;
          guint width;

          width = clutter_actor_get_width (CLUTTER_ACTOR (viewport));

          adjustment = shumate_adjustment_new (priv->x,
                0,
                width,
                1);
          shumate_viewport_set_adjustments (viewport,
              adjustment,
              priv->vadjustment);
          *hadjustment = adjustment;
        }
    }

  if (vadjustment)
    {
      if (priv->vadjustment)
        *vadjustment = priv->vadjustment;
      else
        {
          ShumateAdjustment *adjustment;
          guint height;

          height = clutter_actor_get_height (CLUTTER_ACTOR (viewport));

          adjustment = shumate_adjustment_new (priv->y,
                0,
                height,
                1);
          shumate_viewport_set_adjustments (viewport,
              priv->hadjustment,
              adjustment);
          *vadjustment = adjustment;
        }
    }
}


static void
shumate_viewport_init (ShumateViewport *self)
{
  self->priv = GET_PRIVATE (self);

  self->priv->anchor_x = 0;
  self->priv->anchor_y = 0;
}


ClutterActor *
shumate_viewport_new (void)
{
  return g_object_new (SHUMATE_TYPE_VIEWPORT, NULL);
}


#define ANCHOR_LIMIT G_MAXINT16

void
shumate_viewport_set_origin (ShumateViewport *viewport,
    gdouble x,
    gdouble y)
{
  g_return_if_fail (SHUMATE_IS_VIEWPORT (viewport));

  ShumateViewportPrivate *priv = viewport->priv;
  gboolean relocated;

  if (x == priv->x && y == priv->y)
    return;

  relocated = (ABS (priv->anchor_x - x) > ANCHOR_LIMIT || ABS (priv->anchor_y - y) > ANCHOR_LIMIT);
  if (relocated)
    {
      priv->anchor_x = x - ANCHOR_LIMIT / 2;
      priv->anchor_y = y - ANCHOR_LIMIT / 2;
    }

  if (priv->child)
    clutter_actor_set_position (priv->child, -x + priv->anchor_x, -y + priv->anchor_y);

  g_object_freeze_notify (G_OBJECT (viewport));

  if (priv->hadjustment && priv->vadjustment)
    {
      g_object_freeze_notify (G_OBJECT (priv->hadjustment));
      g_object_freeze_notify (G_OBJECT (priv->vadjustment));

      if (x != priv->x)
        {
          priv->x = x;
          g_object_notify (G_OBJECT (viewport), "x-origin");

          shumate_adjustment_set_value (priv->hadjustment, x);
        }

      if (y != priv->y)
        {
          priv->y = y;
          g_object_notify (G_OBJECT (viewport), "y-origin");

          shumate_adjustment_set_value (priv->vadjustment, y);
        }

      g_object_thaw_notify (G_OBJECT (priv->hadjustment));
      g_object_thaw_notify (G_OBJECT (priv->vadjustment));
    }

  g_object_thaw_notify (G_OBJECT (viewport));

  if (relocated)
    g_signal_emit_by_name (viewport, "relocated", NULL);
}


void
shumate_viewport_get_origin (ShumateViewport *viewport,
    gdouble *x,
    gdouble *y)
{
  g_return_if_fail (SHUMATE_IS_VIEWPORT (viewport));

  ShumateViewportPrivate *priv = viewport->priv;

  if (x)
    *x = priv->x;

  if (y)
    *y = priv->y;
}


void
shumate_viewport_get_anchor (ShumateViewport *viewport,
    gint *x,
    gint *y)
{
  g_return_if_fail (SHUMATE_IS_VIEWPORT (viewport));

  ShumateViewportPrivate *priv = viewport->priv;

  if (x)
    *x = priv->anchor_x;

  if (y)
    *y = priv->anchor_y;
}


void
shumate_viewport_set_child (ShumateViewport *viewport, ClutterActor *child)
{
  g_return_if_fail (SHUMATE_IS_VIEWPORT (viewport));

  ShumateViewportPrivate *priv = viewport->priv;

  clutter_actor_remove_all_children (CLUTTER_ACTOR (viewport));
  clutter_actor_add_child (CLUTTER_ACTOR (viewport), child);
  priv->child = child;
}


void
shumate_viewport_set_actor_position (ShumateViewport *viewport,
    ClutterActor *actor,
    gdouble x,
    gdouble y)
{
  ShumateViewportPrivate *priv = viewport->priv;

  clutter_actor_set_position (actor, x - priv->anchor_x, y - priv->anchor_y);
}
