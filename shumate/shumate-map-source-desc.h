/*
 * Copyright (C) 2009 Pierre-Luc Beaudoin <pierre-luc@pierlux.com>
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

#ifndef SHUMATE_MAP_SOURCE_DESC_H
#define SHUMATE_MAP_SOURCE_DESC_H

#include <glib-object.h>
#include "shumate-tile-source.h"

G_BEGIN_DECLS

#define SHUMATE_TYPE_MAP_SOURCE_DESC shumate_map_source_desc_get_type ()
G_DECLARE_FINAL_TYPE (ShumateMapSourceDesc, shumate_map_source_desc, SHUMATE, MAP_SOURCE_DESC, GObject)

/**
 * ShumateMapSourceDesc:
 *
 * The #ShumateMapSourceDesc structure contains only private data
 * and should be accessed using the provided API
 */

/**
 * ShumateMapSourceConstructor:
 * @desc: a #ShumateMapSourceDesc
 *
 * A #ShumateMapSource constructor.  It should return a ready to use
 * #ShumateMapSource.
 *
 * Returns: A fully constructed #ShumateMapSource ready to be used.
 */
typedef ShumateMapSource* (*ShumateMapSourceConstructor) (ShumateMapSourceDesc *desc);

/**
 * SHUMATE_MAP_SOURCE_CONSTRUCTOR:
 *
 * Conversion macro to #ShumateMapSourceConstructor.
 */
#define SHUMATE_MAP_SOURCE_CONSTRUCTOR (f) ((ShumateMapSourceConstructor) (f))

ShumateMapSourceDesc *shumate_map_source_desc_new_full (
    char *id,
    char *name,
    char *license,
    char *license_uri,
    guint min_zoom,
    guint max_zoom,
    guint tile_size,
    ShumateMapProjection projection,
    char *uri_format,
    ShumateMapSourceConstructor constructor,
    gpointer data);

const char *shumate_map_source_desc_get_id (ShumateMapSourceDesc *desc);
const char *shumate_map_source_desc_get_name (ShumateMapSourceDesc *desc);
const char *shumate_map_source_desc_get_license (ShumateMapSourceDesc *desc);
const char *shumate_map_source_desc_get_license_uri (ShumateMapSourceDesc *desc);
const char *shumate_map_source_desc_get_uri_format (ShumateMapSourceDesc *desc);
guint shumate_map_source_desc_get_min_zoom_level (ShumateMapSourceDesc *desc);
guint shumate_map_source_desc_get_max_zoom_level (ShumateMapSourceDesc *desc);
guint shumate_map_source_desc_get_tile_size (ShumateMapSourceDesc *desc);
ShumateMapProjection shumate_map_source_desc_get_projection (ShumateMapSourceDesc *desc);
gpointer shumate_map_source_desc_get_data (ShumateMapSourceDesc *desc);
ShumateMapSourceConstructor shumate_map_source_desc_get_constructor (ShumateMapSourceDesc *desc);

G_END_DECLS

#endif
