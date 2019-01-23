/*
 * Copyright (C) 2009 Simon Wenner <simon@wenner.ch>
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

#ifndef SHUMATE_BOUNDING_BOX_H
#define SHUMATE_BOUNDING_BOX_H

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _ShumateBoundingBox ShumateBoundingBox;

#define SHUMATE_BOUNDING_BOX(obj) ((ShumateBoundingBox *) (obj))

/**
 * ShumateBoundingBox:
 * @left: left coordinate
 * @top: top coordinate
 * @right: right coordinate
 * @bottom: bottom coordinate
 *
 * Defines the area of a ShumateMapDataSource that contains data.
 */
struct _ShumateBoundingBox
{
  /*< public >*/
  gdouble left;
  gdouble top;
  gdouble right;
  gdouble bottom;
};

GType shumate_bounding_box_get_type (void) G_GNUC_CONST;
#define SHUMATE_TYPE_BOUNDING_BOX (shumate_bounding_box_get_type ())

ShumateBoundingBox *shumate_bounding_box_new (void);

ShumateBoundingBox *shumate_bounding_box_copy (const ShumateBoundingBox *bbox);

void shumate_bounding_box_free (ShumateBoundingBox *bbox);

void shumate_bounding_box_get_center (ShumateBoundingBox *bbox,
    gdouble *latitude,
    gdouble *longitude);

void shumate_bounding_box_compose (ShumateBoundingBox *bbox,
    ShumateBoundingBox *other);

void shumate_bounding_box_extend (ShumateBoundingBox *bbox,
    gdouble latitude,
    gdouble longitude);

gboolean shumate_bounding_box_is_valid (ShumateBoundingBox *bbox);

gboolean shumate_bounding_box_covers(ShumateBoundingBox *bbox,
    gdouble latitude,
    gdouble longitude);

G_END_DECLS

#endif
