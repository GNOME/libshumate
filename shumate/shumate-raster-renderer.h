/*
 * Copyright (C) 2021 James Westman <james@jwestman.net>
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
 * License along with this library; if not, see <https://www.gnu.org/licenses/>.
 */


#pragma once

#include <shumate/shumate-data-source.h>
#include <shumate/shumate-map-source.h>

G_BEGIN_DECLS


#define SHUMATE_TYPE_RASTER_RENDERER (shumate_raster_renderer_get_type())
G_DECLARE_FINAL_TYPE (ShumateRasterRenderer, shumate_raster_renderer, SHUMATE, RASTER_RENDERER, ShumateMapSource)


ShumateRasterRenderer *shumate_raster_renderer_new (ShumateDataSource *data_source);
ShumateRasterRenderer *shumate_raster_renderer_new_from_url (const char *url_template);

ShumateRasterRenderer *shumate_raster_renderer_new_full (const char           *id,
                                                         const char           *name,
                                                         const char           *license,
                                                         const char           *license_uri,
                                                         guint                 min_zoom,
                                                         guint                 max_zoom,
                                                         guint                 tile_size,
                                                         ShumateMapProjection  projection,
                                                         ShumateDataSource    *data_source);
ShumateRasterRenderer *shumate_raster_renderer_new_full_from_url (const char           *id,
                                                                  const char           *name,
                                                                  const char           *license,
                                                                  const char           *license_uri,
                                                                  guint                 min_zoom,
                                                                  guint                 max_zoom,
                                                                  guint                 tile_size,
                                                                  ShumateMapProjection  projection,
                                                                  const char           *url_template);

G_END_DECLS
