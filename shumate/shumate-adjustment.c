/* shumate-adjustment.c: Adjustment object
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
 * Written by: Chris Lord <chris@openedhand.com>, inspired by GtkAdjustment
 */

#include "config.h"

#include <clutter/clutter.h>

#include "shumate-adjustment.h"
#include "shumate-marshal.h"
#include "shumate-private.h"

G_DEFINE_TYPE (ShumateAdjustment, shumate_adjustment, G_TYPE_OBJECT)

#define ADJUSTMENT_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), SHUMATE_TYPE_ADJUSTMENT, ShumateAdjustmentPrivate))

struct _ShumateAdjustmentPrivate
{
  gdouble lower;
  gdouble upper;
  gdouble value;
  gdouble step_increment;

  /* For interpolation */
  ClutterTimeline *interpolation;
  gdouble dx;
  gdouble old_position;
  gdouble new_position;
};

enum
{
  PROP_0,

  PROP_LOWER,
  PROP_UPPER,
  PROP_VALUE,
  PROP_STEP_INC,
};

enum
{
  CHANGED,

  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };

static void shumate_adjustment_set_lower (ShumateAdjustment *adjustment,
    gdouble lower);
static void shumate_adjustment_set_upper (ShumateAdjustment *adjustment,
    gdouble upper);
static void shumate_adjustment_set_step_increment (ShumateAdjustment *adjustment,
    gdouble step);

static void
shumate_adjustment_get_property (GObject *object,
    guint prop_id,
    GValue *value,
    GParamSpec *pspec)
{
  ShumateAdjustmentPrivate *priv = SHUMATE_ADJUSTMENT (object)->priv;

  switch (prop_id)
    {
    case PROP_LOWER:
      g_value_set_double (value, priv->lower);
      break;

    case PROP_UPPER:
      g_value_set_double (value, priv->upper);
      break;

    case PROP_VALUE:
      g_value_set_double (value, priv->value);
      break;

    case PROP_STEP_INC:
      g_value_set_double (value, priv->step_increment);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


static void
shumate_adjustment_set_property (GObject *object,
    guint prop_id,
    const GValue *value,
    GParamSpec *pspec)
{
  ShumateAdjustment *adj = SHUMATE_ADJUSTMENT (object);

  switch (prop_id)
    {
    case PROP_LOWER:
      shumate_adjustment_set_lower (adj, g_value_get_double (value));
      break;

    case PROP_UPPER:
      shumate_adjustment_set_upper (adj, g_value_get_double (value));
      break;

    case PROP_VALUE:
      shumate_adjustment_set_value (adj, g_value_get_double (value));
      break;

    case PROP_STEP_INC:
      shumate_adjustment_set_step_increment (adj, g_value_get_double (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}


static void
stop_interpolation (ShumateAdjustment *adjustment)
{
  ShumateAdjustmentPrivate *priv = adjustment->priv;

  if (priv->interpolation)
    {
      clutter_timeline_stop (priv->interpolation);
      g_object_unref (priv->interpolation);
      priv->interpolation = NULL;
    }
}


void
shumate_adjustment_interpolate_stop (ShumateAdjustment *adjustment)
{
  stop_interpolation (adjustment);
}


static void
shumate_adjustment_dispose (GObject *object)
{
  stop_interpolation (SHUMATE_ADJUSTMENT (object));

  G_OBJECT_CLASS (shumate_adjustment_parent_class)->dispose (object);
}


static void
shumate_adjustment_class_init (ShumateAdjustmentClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (ShumateAdjustmentPrivate));

  object_class->get_property = shumate_adjustment_get_property;
  object_class->set_property = shumate_adjustment_set_property;
  object_class->dispose = shumate_adjustment_dispose;

  g_object_class_install_property (object_class,
      PROP_LOWER,
      g_param_spec_double ("lower",
          "Lower",
          "Lower bound",
          -G_MAXDOUBLE,
          G_MAXDOUBLE,
          0.0,
          SHUMATE_PARAM_READWRITE));
  g_object_class_install_property (object_class,
      PROP_UPPER,
      g_param_spec_double ("upper",
          "Upper",
          "Upper bound",
          -G_MAXDOUBLE,
          G_MAXDOUBLE,
          0.0,
          SHUMATE_PARAM_READWRITE));
  g_object_class_install_property (object_class,
      PROP_VALUE,
      g_param_spec_double ("value",
          "Value",
          "Current value",
          -G_MAXDOUBLE,
          G_MAXDOUBLE,
          0.0,
          SHUMATE_PARAM_READWRITE));
  g_object_class_install_property (object_class,
      PROP_STEP_INC,
      g_param_spec_double ("step-increment",
          "Step Increment",
          "Step increment",
          -G_MAXDOUBLE,
          G_MAXDOUBLE,
          0.0,
          SHUMATE_PARAM_READWRITE));

  signals[CHANGED] =
    g_signal_new ("changed",
        G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET (ShumateAdjustmentClass, changed),
        NULL, NULL,
        g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0);
}


static void
shumate_adjustment_init (ShumateAdjustment *self)
{
  self->priv = ADJUSTMENT_PRIVATE (self);
}


ShumateAdjustment *
shumate_adjustment_new (gdouble value,
    gdouble lower,
    gdouble upper,
    gdouble step_increment)
{
  return g_object_new (SHUMATE_TYPE_ADJUSTMENT,
      "value", value,
      "lower", lower,
      "upper", upper,
      "step-increment", step_increment,
      NULL);
}


gdouble
shumate_adjustment_get_value (ShumateAdjustment *adjustment)
{
  g_return_val_if_fail (SHUMATE_IS_ADJUSTMENT (adjustment), 0.0);

  return adjustment->priv->value;
}


void
shumate_adjustment_set_value (ShumateAdjustment *adjustment,
    double value)
{
  ShumateAdjustmentPrivate *priv;

  g_return_if_fail (SHUMATE_IS_ADJUSTMENT (adjustment));

  priv = adjustment->priv;

  stop_interpolation (adjustment);

  value = CLAMP (value, priv->lower, priv->upper);

  if (priv->value != value)
    {
      priv->value = value;
      g_object_notify (G_OBJECT (adjustment), "value");
    }
}


static void
shumate_adjustment_clamp_page (ShumateAdjustment *adjustment,
    double lower,
    double upper)
{
  gboolean changed;
  ShumateAdjustmentPrivate *priv;

  g_return_if_fail (SHUMATE_IS_ADJUSTMENT (adjustment));

  priv = adjustment->priv;

  stop_interpolation (adjustment);

  lower = CLAMP (lower, priv->lower, priv->upper);
  upper = CLAMP (upper, priv->lower, priv->upper);

  changed = FALSE;

  if (priv->value > upper)
    {
      priv->value = upper;
      changed = TRUE;
    }

  if (priv->value < lower)
    {
      priv->value = lower;
      changed = TRUE;
    }

  if (changed)
    g_object_notify (G_OBJECT (adjustment), "value");
}


static void
shumate_adjustment_set_lower (ShumateAdjustment *adjustment,
    gdouble lower)
{
  ShumateAdjustmentPrivate *priv = adjustment->priv;

  if (priv->lower != lower)
    {
      priv->lower = lower;

      g_signal_emit (adjustment, signals[CHANGED], 0);

      g_object_notify (G_OBJECT (adjustment), "lower");

      shumate_adjustment_clamp_page (adjustment, priv->lower, priv->upper);
    }
}


static void
shumate_adjustment_set_upper (ShumateAdjustment *adjustment,
    gdouble upper)
{
  ShumateAdjustmentPrivate *priv = adjustment->priv;

  if (priv->upper != upper)
    {
      priv->upper = upper;

      g_signal_emit (adjustment, signals[CHANGED], 0);

      g_object_notify (G_OBJECT (adjustment), "upper");

      shumate_adjustment_clamp_page (adjustment, priv->lower, priv->upper);
    }
}


static void
shumate_adjustment_set_step_increment (ShumateAdjustment *adjustment,
    gdouble step)
{
  ShumateAdjustmentPrivate *priv = adjustment->priv;

  if (priv->step_increment != step)
    {
      priv->step_increment = step;

      g_signal_emit (adjustment, signals[CHANGED], 0);

      g_object_notify (G_OBJECT (adjustment), "step-increment");
    }
}


void
shumate_adjustment_set_values (ShumateAdjustment *adjustment,
    gdouble value,
    gdouble lower,
    gdouble upper,
    gdouble step_increment)
{
  ShumateAdjustmentPrivate *priv;
  gboolean emit_changed = FALSE;

  g_return_if_fail (SHUMATE_IS_ADJUSTMENT (adjustment));

  priv = adjustment->priv;

  stop_interpolation (adjustment);

  g_object_freeze_notify (G_OBJECT (adjustment));

  if (priv->lower != lower)
    {
      priv->lower = lower;
      emit_changed = TRUE;

      g_object_notify (G_OBJECT (adjustment), "lower");
    }

  if (priv->upper != upper)
    {
      priv->upper = upper;
      emit_changed = TRUE;

      g_object_notify (G_OBJECT (adjustment), "upper");
    }

  if (priv->step_increment != step_increment)
    {
      priv->step_increment = step_increment;
      emit_changed = TRUE;

      g_object_notify (G_OBJECT (adjustment), "step-increment");
    }

  shumate_adjustment_set_value (adjustment, value);

  if (emit_changed)
    g_signal_emit (G_OBJECT (adjustment), signals[CHANGED], 0);

  g_object_thaw_notify (G_OBJECT (adjustment));
}


void
shumate_adjustment_get_values (ShumateAdjustment *adjustment,
    gdouble *value,
    gdouble *lower,
    gdouble *upper,
    gdouble *step_increment)
{
  ShumateAdjustmentPrivate *priv;

  g_return_if_fail (SHUMATE_IS_ADJUSTMENT (adjustment));

  priv = adjustment->priv;

  if (lower)
    *lower = priv->lower;

  if (upper)
    *upper = priv->upper;

  if (value)
    *value = shumate_adjustment_get_value (adjustment);

  if (step_increment)
    *step_increment = priv->step_increment;
}


static void
interpolation_new_frame_cb (ClutterTimeline *timeline,
    gint frame_num,
    ShumateAdjustment *adjustment)
{
  ShumateAdjustmentPrivate *priv = adjustment->priv;

  priv->interpolation = NULL;
  shumate_adjustment_set_value (adjustment,
      priv->old_position +
      frame_num * priv->dx);
  priv->interpolation = timeline;
}


static void
interpolation_completed_cb (ClutterTimeline *timeline,
    ShumateAdjustment *adjustment)
{
  ShumateAdjustmentPrivate *priv = adjustment->priv;

  stop_interpolation (adjustment);
  shumate_adjustment_set_value (adjustment, priv->new_position);
}


void
shumate_adjustment_interpolate (ShumateAdjustment *adjustment,
    gdouble value,
    guint n_frames,
    guint fps)
{
  ShumateAdjustmentPrivate *priv = adjustment->priv;

  stop_interpolation (adjustment);

  if (n_frames <= 1)
    {
      shumate_adjustment_set_value (adjustment, value);
      return;
    }

  priv->old_position = priv->value;
  priv->new_position = value;

  priv->dx = (priv->new_position - priv->old_position) * n_frames;
  priv->interpolation = clutter_timeline_new (((gdouble) n_frames / fps) * 1000);

  g_signal_connect (priv->interpolation,
      "new-frame",
      G_CALLBACK (interpolation_new_frame_cb),
      adjustment);
  g_signal_connect (priv->interpolation,
      "completed",
      G_CALLBACK (interpolation_completed_cb),
      adjustment);

  clutter_timeline_start (priv->interpolation);
}


gboolean
shumate_adjustment_clamp (ShumateAdjustment *adjustment,
    gboolean interpolate,
    guint n_frames,
    guint fps)
{
  ShumateAdjustmentPrivate *priv = adjustment->priv;
  gdouble dest = priv->value;

  dest = CLAMP (dest, priv->lower, priv->upper);

  if (dest != priv->value)
    {
      if (interpolate)
        shumate_adjustment_interpolate (adjustment,
            dest,
            n_frames,
            fps);
      else
        shumate_adjustment_set_value (adjustment, dest);

      return TRUE;
    }

  return FALSE;
}
