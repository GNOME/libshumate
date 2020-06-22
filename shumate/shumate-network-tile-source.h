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

#ifndef _SHUMATE_NETWORK_TILE_SOURCE_H_
#define _SHUMATE_NETWORK_TILE_SOURCE_H_

#include <shumate/shumate-tile-source.h>

G_BEGIN_DECLS

#define SHUMATE_TYPE_NETWORK_TILE_SOURCE shumate_network_tile_source_get_type ()
G_DECLARE_DERIVABLE_TYPE (ShumateNetworkTileSource, shumate_network_tile_source, SHUMATE, NETWORK_TILE_SOURCE, ShumateTileSource)

/**
 * ShumateNetworkTileSource:
 *
 * The #ShumateNetworkTileSource structure contains only private data
 * and should be accessed using the provided API
 */

struct _ShumateNetworkTileSourceClass
{
  ShumateTileSourceClass parent_class;
};

ShumateNetworkTileSource *shumate_network_tile_source_new_full (const gchar *id,
    const gchar *name,
    const gchar *license,
    const gchar *license_uri,
    guint min_zoom,
    guint max_zoom,
    guint tile_size,
    ShumateMapProjection projection,
    const gchar *uri_format);

const gchar *shumate_network_tile_source_get_uri_format (ShumateNetworkTileSource *tile_source);
void shumate_network_tile_source_set_uri_format (ShumateNetworkTileSource *tile_source,
    const gchar *uri_format);

gboolean shumate_network_tile_source_get_offline (ShumateNetworkTileSource *tile_source);
void shumate_network_tile_source_set_offline (ShumateNetworkTileSource *tile_source,
    gboolean offline);

const gchar *shumate_network_tile_source_get_proxy_uri (ShumateNetworkTileSource *tile_source);
void shumate_network_tile_source_set_proxy_uri (ShumateNetworkTileSource *tile_source,
    const gchar *proxy_uri);

gint shumate_network_tile_source_get_max_conns (ShumateNetworkTileSource *tile_source);
void shumate_network_tile_source_set_max_conns (ShumateNetworkTileSource *tile_source,
    gint max_conns);

void shumate_network_tile_source_set_user_agent (ShumateNetworkTileSource *tile_source,
    const gchar *user_agent);

G_END_DECLS

#endif /* _SHUMATE_NETWORK_TILE_SOURCE_H_ */
