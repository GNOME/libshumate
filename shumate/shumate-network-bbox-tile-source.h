/*
 * Copyright (C) 2009 Simon Wenner <simon@wenner.ch>
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

#ifndef _SHUMATE_NETWORK_BBOX_TILE_SOURCE
#define _SHUMATE_NETWORK_BBOX_TILE_SOURCE

#include <glib-object.h>

#include <shumate/shumate-tile-source.h>
#include <shumate/shumate-bounding-box.h>

G_BEGIN_DECLS

#define SHUMATE_TYPE_NETWORK_BBOX_TILE_SOURCE shumate_network_bbox_tile_source_get_type ()

#define SHUMATE_NETWORK_BBOX_TILE_SOURCE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SHUMATE_TYPE_NETWORK_BBOX_TILE_SOURCE, ShumateNetworkBboxTileSource))

#define SHUMATE_NETWORK_BBOX_TILE_SOURCE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SHUMATE_TYPE_NETWORK_BBOX_TILE_SOURCE, ShumateNetworkBboxTileSourceClass))

#define SHUMATE_IS_NETWORK_BBOX_TILE_SOURCE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SHUMATE_TYPE_NETWORK_BBOX_TILE_SOURCE))

#define SHUMATE_IS_NETWORK_BBOX_TILE_SOURCE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SHUMATE_TYPE_NETWORK_BBOX_TILE_SOURCE))

#define SHUMATE_NETWORK_BBOX_TILE_SOURCE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), SHUMATE_TYPE_NETWORK_BBOX_TILE_SOURCE, ShumateNetworkBboxTileSourceClass))

typedef struct _ShumateNetworkBboxTileSourcePrivate ShumateNetworkBboxTileSourcePrivate;

typedef struct _ShumateNetworkBboxTileSource ShumateNetworkBboxTileSource;
typedef struct _ShumateNetworkBboxTileSourceClass ShumateNetworkBboxTileSourceClass;

/**
 * ShumateNetworkBboxTileSource:
 *
 * The #ShumateNetworkBboxTileSource structure contains only private data
 * and should be accessed using the provided API
 *
 * Since: 0.8
 */
struct _ShumateNetworkBboxTileSource
{
  ShumateTileSource parent;

  ShumateNetworkBboxTileSourcePrivate *priv;
};

struct _ShumateNetworkBboxTileSourceClass
{
  ShumateTileSourceClass parent_class;
};

GType shumate_network_bbox_tile_source_get_type (void);

ShumateNetworkBboxTileSource *shumate_network_bbox_tile_source_new_full (const gchar *id,
    const gchar *name,
    const gchar *license,
    const gchar *license_uri,
    guint min_zoom,
    guint max_zoom,
    guint tile_size,
    ShumateMapProjection projection,
    ShumateRenderer *renderer);

void shumate_network_bbox_tile_source_load_map_data (
    ShumateNetworkBboxTileSource *map_data_source,
    ShumateBoundingBox *bbox);

const gchar *shumate_network_bbox_tile_source_get_api_uri (
    ShumateNetworkBboxTileSource *map_data_source);

void shumate_network_bbox_tile_source_set_api_uri (
    ShumateNetworkBboxTileSource *map_data_source,
    const gchar *api_uri);

void shumate_network_bbox_tile_source_set_user_agent (
    ShumateNetworkBboxTileSource *map_data_source,
    const gchar *user_agent);

G_END_DECLS

#endif /* _SHUMATE_NETWORK_BBOX_TILE_SOURCE */
