/*
 * Copyright (C) 2008-2009 Pierre-Luc Beaudoin <pierre-luc@pierlux.com>
 * Copyright (C) 2010-2013 Jiri Techet <techet@gmail.com>
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

#include <shumate/shumate-defines.h>
#include <shumate/shumate-map-source.h>
#include <shumate/shumate-tile-cache.h>

G_BEGIN_DECLS

#define SHUMATE_TYPE_TILE_SOURCE shumate_tile_source_get_type ()

#define SHUMATE_TILE_SOURCE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SHUMATE_TYPE_TILE_SOURCE, ShumateTileSource))

#define SHUMATE_TILE_SOURCE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SHUMATE_TYPE_TILE_SOURCE, ShumateTileSourceClass))

#define SHUMATE_IS_TILE_SOURCE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SHUMATE_TYPE_TILE_SOURCE))

#define SHUMATE_IS_TILE_SOURCE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SHUMATE_TYPE_TILE_SOURCE))

#define SHUMATE_TILE_SOURCE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), SHUMATE_TYPE_TILE_SOURCE, ShumateTileSourceClass))

typedef struct _ShumateTileSourcePrivate ShumateTileSourcePrivate;

typedef struct _ShumateTileSource ShumateTileSource;
typedef struct _ShumateTileSourceClass ShumateTileSourceClass;

/**
 * ShumateTileSource:
 *
 * The #ShumateTileSource structure contains only private data
 * and should be accessed using the provided API
 *
 * Since: 0.6
 */
struct _ShumateTileSource
{
  ShumateMapSource parent_instance;

  ShumateTileSourcePrivate *priv;
};

struct _ShumateTileSourceClass
{
  ShumateMapSourceClass parent_class;
};

GType shumate_tile_source_get_type (void);

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
