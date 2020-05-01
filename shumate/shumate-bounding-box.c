/*
 * Copyright (C) 2009 Simon Wenner <simon@wenner.ch>
 * Copyright (C) 2019 Marcus Lundblad <ml@update.uu.se>
 *
 * This file is inspired by clutter-color.c which is
 * Copyright (C) 2006 OpenedHand, and has the same license.
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

/**
 * SECTION:shumate-bounding-box
 * @short_description: A basic struct to describe a bounding box
 *
 * A basic struct to describe a bounding box.
 *
 */

#include "shumate-bounding-box.h"
#include "shumate-defines.h"

G_DEFINE_BOXED_TYPE (ShumateBoundingBox, shumate_bounding_box, shumate_bounding_box_copy, shumate_bounding_box_free)

/**
 * shumate_bounding_box_new:
 *
 * Creates a newly allocated #ShumateBoundingBox to be freed
 * with shumate_bounding_box_free().
 *
 * Returns: a #ShumateBoundingBox
 */
ShumateBoundingBox *
shumate_bounding_box_new (void)
{
  ShumateBoundingBox *bbox;

  bbox = g_slice_new (ShumateBoundingBox);

  bbox->left = SHUMATE_MAX_LONGITUDE;
  bbox->right = SHUMATE_MIN_LONGITUDE;
  bbox->bottom = SHUMATE_MAX_LATITUDE;
  bbox->top = SHUMATE_MIN_LATITUDE;

  return bbox;
}


/**
 * shumate_bounding_box_copy:
 * @bbox: a #ShumateBoundingBox
 *
 * Makes a copy of the bounding box structure. The result must be
 * freed using shumate_bounding_box_free().
 *
 * Returns: an allocated copy of @bbox.
 */
ShumateBoundingBox *
shumate_bounding_box_copy (const ShumateBoundingBox *bbox)
{
  if (G_LIKELY (bbox != NULL))
    return g_slice_dup (ShumateBoundingBox, bbox);

  return NULL;
}


/**
 * shumate_bounding_box_free:
 * @bbox: a #ShumateBoundingBox
 *
 * Frees a bounding box structure created with shumate_bounding_box_new() or
 * shumate_bounding_box_copy().
 */
void
shumate_bounding_box_free (ShumateBoundingBox *bbox)
{
  if (G_UNLIKELY (bbox == NULL))
    return;

  g_slice_free (ShumateBoundingBox, bbox);
}


/**
 * shumate_bounding_box_get_center:
 * @bbox: a #ShumateBoundingBox
 * @latitude: (out): the latitude of the box center
 * @longitude: (out): the longitude of the box center
 *
 * Gets the center's latitude and longitude of the box to @latitude and @longitude.
 */
void
shumate_bounding_box_get_center (ShumateBoundingBox *bbox,
    gdouble *latitude,
    gdouble *longitude)
{
  g_return_if_fail (SHUMATE_BOUNDING_BOX (bbox));

  *longitude = (bbox->right + bbox->left) / 2.0;
  *latitude = (bbox->top + bbox->bottom) / 2.0;
}


/**
 * shumate_bounding_box_compose:
 * @bbox: a #ShumateBoundingBox
 * @other: a #ShumateBoundingBox
 *
 * Sets bbox equal to the bounding box containing both @bbox and @other.
 */
void
shumate_bounding_box_compose (ShumateBoundingBox *bbox,
    ShumateBoundingBox *other)
{
  g_return_if_fail (SHUMATE_BOUNDING_BOX (bbox));

  if (other->left < bbox->left)
    bbox->left = other->left;

  if (other->right > bbox->right)
    bbox->right = other->right;

  if (other->top > bbox->top)
    bbox->top = other->top;

  if (other->bottom < bbox->bottom)
    bbox->bottom = other->bottom;
}


/**
 * shumate_bounding_box_extend:
 * @bbox: a #ShumateBoundingBox
 * @latitude: the latitude of the point
 * @longitude: the longitude of the point
 *
 * Extend the bounding box so it contains a point with @latitude and @longitude.
 * Do nothing if the point is already inside the bounding box.
 */
void
shumate_bounding_box_extend (ShumateBoundingBox *bbox,
    gdouble latitude, gdouble longitude)
{
  g_return_if_fail (SHUMATE_BOUNDING_BOX (bbox));

  if (longitude < bbox->left)
    bbox->left = longitude;

  if (latitude < bbox->bottom)
    bbox->bottom = latitude;

  if (longitude > bbox->right)
    bbox->right = longitude;

  if (latitude > bbox->top)
    bbox->top = latitude;
}


/**
 * shumate_bounding_box_is_valid:
 * @bbox: a #ShumateBoundingBox
 *
 * Checks whether @bbox represents a valid bounding box on the map.
 *
 * Returns: TRUE when the bounding box is valid, FALSE otherwise.
 */
gboolean
shumate_bounding_box_is_valid (ShumateBoundingBox *bbox)
{
  g_return_val_if_fail (SHUMATE_BOUNDING_BOX (bbox), FALSE);

  return (bbox->left < bbox->right) && (bbox->bottom < bbox->top) &&
         (bbox->left >= SHUMATE_MIN_LONGITUDE) && (bbox->left <= SHUMATE_MAX_LONGITUDE) &&
         (bbox->right >= SHUMATE_MIN_LONGITUDE) && (bbox->right <= SHUMATE_MAX_LONGITUDE) &&
         (bbox->bottom >= SHUMATE_MIN_LATITUDE) && (bbox->bottom <= SHUMATE_MAX_LATITUDE) &&
         (bbox->top >= SHUMATE_MIN_LATITUDE) && (bbox->top <= SHUMATE_MAX_LATITUDE);
}

/**
 * shumate_bounding_box_covers:
 * @bbox: a #ShumateBoundingBox
 * @latitude: the latitude of the point
 * @longitude: the longitude of the point
 *
 * Checks whether @bbox covers the given coordinates.
 *
 * Returns: TRUE when the bounding box covers given coordinates, FALSE otherwise.
 */
gboolean
shumate_bounding_box_covers(ShumateBoundingBox *bbox,
    gdouble latitude,
    gdouble longitude)
{
  g_return_val_if_fail (SHUMATE_BOUNDING_BOX (bbox), FALSE);

  return ((latitude >= bbox->bottom && latitude <= bbox->top) &&
          (longitude >= bbox->left && longitude <= bbox->right));
}
