/*
 * Copyright (C) 2008-2009 Pierre-Luc Beaudoin <pierre-luc@pierlux.com>
 * Copyright (C) 2010-2013 Jiri Techet <techet@gmail.com>
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

#ifndef _SHUMATE_TILE_SOURCE_H_
#define _SHUMATE_TILE_SOURCE_H_

#include <shumate/shumate-map-source.h>
#include <shumate/shumate-tile-cache.h>

G_BEGIN_DECLS

#define SHUMATE_TYPE_TILE_SOURCE shumate_tile_source_get_type ()
G_DECLARE_DERIVABLE_TYPE (ShumateTileSource, shumate_tile_source, SHUMATE, TILE_SOURCE, ShumateMapSource)

/**
 * ShumateTileSource:
 *
 * The #ShumateTileSource structure contains only private data
 * and should be accessed using the provided API
 */

struct _ShumateTileSourceClass
{
  ShumateMapSourceClass parent_class;
};

ShumateTileCache *shumate_tile_source_get_cache (ShumateTileSource *tile_source);
void shumate_tile_source_set_cache (ShumateTileSource *tile_source,
    ShumateTileCache *cache);

void shumate_tile_source_set_id (ShumateTileSource *tile_source,
    const gchar *id);
void shumate_tile_source_set_name (ShumateTileSource *tile_source,
    const gchar *name);
void shumate_tile_source_set_license (ShumateTileSource *tile_source,
    const gchar *license);
void shumate_tile_source_set_license_uri (ShumateTileSource *tile_source,
    const gchar *license_uri);

void shumate_tile_source_set_min_zoom_level (ShumateTileSource *tile_source,
    guint zoom_level);
void shumate_tile_source_set_max_zoom_level (ShumateTileSource *tile_source,
    guint zoom_level);
void shumate_tile_source_set_tile_size (ShumateTileSource *tile_source,
    guint tile_size);
void shumate_tile_source_set_projection (ShumateTileSource *tile_source,
    ShumateMapProjection projection);

G_END_DECLS

#endif /* _SHUMATE_TILE_SOURCE_H_ */
