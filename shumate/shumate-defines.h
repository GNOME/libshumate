/*
 * Copyright (C) 2008 Pierre-Luc Beaudoin <pierre-luc@pierlux.com>
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

#ifndef SHUMATE_DEFINES_H
#define SHUMATE_DEFINES_H

#define SHUMATE_API __attribute__((visibility ("default")))

/*
 * The ordinate y of the Mercator projection becomes infinite at the poles
 * and the map must be truncated at some latitude less than ninety degrees.
 *
 * Using a square aspect ratio for the map, the maximum latitude shown is
 * approximately 85.05113 degrees.
 */
#define SHUMATE_MIN_LATITUDE   -85.0511287798
#define SHUMATE_MAX_LATITUDE    85.0511287798
#define SHUMATE_MIN_LONGITUDE -180.0
#define SHUMATE_MAX_LONGITUDE  180.0

typedef struct _ShumateView ShumateView;
typedef struct _ShumateViewClass ShumateViewClass;

typedef struct _ShumateMapSource ShumateMapSource;

#endif
