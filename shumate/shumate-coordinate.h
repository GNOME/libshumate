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

#ifndef SHUMATE_COORDINATE_H
#define SHUMATE_COORDINATE_H

#include <shumate/shumate-defines.h>
#include <shumate/shumate-location.h>

#include <glib-object.h>

G_BEGIN_DECLS

#define SHUMATE_TYPE_COORDINATE shumate_coordinate_get_type ()

#define SHUMATE_COORDINATE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SHUMATE_TYPE_COORDINATE, ShumateCoordinate))

#define SHUMATE_COORDINATE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SHUMATE_TYPE_COORDINATE, ShumateCoordinateClass))

#define SHUMATE_IS_COORDINATE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SHUMATE_TYPE_COORDINATE))

#define SHUMATE_IS_COORDINATE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SHUMATE_TYPE_COORDINATE))

#define SHUMATE_COORDINATE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), SHUMATE_TYPE_COORDINATE, ShumateCoordinateClass))

typedef struct _ShumateCoordinatePrivate ShumateCoordinatePrivate;

typedef struct _ShumateCoordinate ShumateCoordinate;
typedef struct _ShumateCoordinateClass ShumateCoordinateClass;


/**
 * ShumateCoordinate:
 *
 * The #ShumateCoordinate structure contains only private data
 * and should be accessed using the provided API
 *
 * Since: 0.10
 */
struct _ShumateCoordinate
{
  GInitiallyUnowned parent;

  ShumateCoordinatePrivate *priv;
};

struct _ShumateCoordinateClass
{
  GInitiallyUnownedClass parent_class;
};

GType shumate_coordinate_get_type (void);

ShumateCoordinate *shumate_coordinate_new (void);

ShumateCoordinate *shumate_coordinate_new_full (gdouble latitude,
    gdouble longitude);

G_END_DECLS

#endif
