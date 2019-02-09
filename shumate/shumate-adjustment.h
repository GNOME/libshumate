/* shumate-adjustment.h: Adjustment object
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
 * Written by: Chris Lord <chris@openedhand.com>, inspired by GtkAdjustment
 */

#ifndef __SHUMATE_ADJUSTMENT_H__
#define __SHUMATE_ADJUSTMENT_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define SHUMATE_TYPE_ADJUSTMENT shumate_adjustment_get_type ()

#define SHUMATE_ADJUSTMENT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SHUMATE_TYPE_ADJUSTMENT, ShumateAdjustment))

#define SHUMATE_IS_ADJUSTMENT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SHUMATE_TYPE_ADJUSTMENT))

#define SHUMATE_ADJUSTMENT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SHUMATE_TYPE_ADJUSTMENT, ShumateAdjustmentClass))

#define SHUMATE_IS_ADJUSTMENT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SHUMATE_TYPE_ADJUSTMENT))

#define SHUMATE_ADJUSTMENT_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), SHUMATE_TYPE_ADJUSTMENT, ShumateAdjustmentClass))

typedef struct _ShumateAdjustment ShumateAdjustment;
typedef struct _ShumateAdjustmentPrivate ShumateAdjustmentPrivate;
typedef struct _ShumateAdjustmentClass ShumateAdjustmentClass;

/**
 * ShumateAdjustment:
 *
 * Class for handling an interval between to values. The contents of
 * the #ShumateAdjustment are private and should be accessed using the
 * public API.
 */
struct _ShumateAdjustment
{
  /*< private >*/
  GObject parent_instance;

  ShumateAdjustmentPrivate *priv;
};

/**
 * ShumateAdjustmentClass:
 * @changed: Class handler for the ::changed signal.
 *
 * Base class for #ShumateAdjustment.
 */
struct _ShumateAdjustmentClass
{
  /*< private >*/
  GObjectClass parent_class;

  /*< public >*/
  void (*changed)(ShumateAdjustment *adjustment);
};

GType shumate_adjustment_get_type (void) G_GNUC_CONST;

ShumateAdjustment *shumate_adjustment_new (gdouble value,
    gdouble lower,
    gdouble upper,
    gdouble step_increment);
gdouble shumate_adjustment_get_value (ShumateAdjustment *adjustment);
void shumate_adjustment_set_value (ShumateAdjustment *adjustment,
    gdouble value);
void shumate_adjustment_set_values (ShumateAdjustment *adjustment,
    gdouble value,
    gdouble lower,
    gdouble upper,
    gdouble step_increment);
void shumate_adjustment_get_values (ShumateAdjustment *adjustment,
    gdouble *value,
    gdouble *lower,
    gdouble *upper,
    gdouble *step_increment);

void shumate_adjustment_interpolate (ShumateAdjustment *adjustment,
    gdouble value,
    guint n_frames,
    guint fps);

gboolean shumate_adjustment_clamp (ShumateAdjustment *adjustment,
    gboolean interpolate,
    guint n_frames,
    guint fps);
void shumate_adjustment_interpolate_stop (ShumateAdjustment *adjustment);

G_END_DECLS

#endif /* __SHUMATE_ADJUSTMENT_H__ */
