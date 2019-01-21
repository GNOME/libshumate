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

#define SHUMATE_MAP_SOURCE_DESC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SHUMATE_TYPE_MAP_SOURCE_DESC, ShumateMapSourceDesc))

#define SHUMATE_MAP_SOURCE_DESC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SHUMATE_TYPE_MAP_SOURCE_DESC, ShumateMapSourceDescClass))

#define SHUMATE_IS_MAP_SOURCE_DESC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SHUMATE_TYPE_MAP_SOURCE_DESC))

#define SHUMATE_IS_MAP_SOURCE_DESC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SHUMATE_TYPE_MAP_SOURCE_DESC))

#define SHUMATE_MAP_SOURCE_DESC_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), SHUMATE_TYPE_MAP_SOURCE_DESC, ShumateMapSourceDescClass))

typedef struct _ShumateMapSourceDescPrivate ShumateMapSourceDescPrivate;

typedef struct _ShumateMapSourceDesc ShumateMapSourceDesc;
typedef struct _ShumateMapSourceDescClass ShumateMapSourceDescClass;

/**
 * ShumateMapSourceDesc:
 *
 * The #ShumateMapSourceDesc structure contains only private data
 * and should be accessed using the provided API
 *
 * Since: 0.10
 */
struct _ShumateMapSourceDesc
{
  GObject parent_instance;

  ShumateMapSourceDescPrivate *priv;
};

struct _ShumateMapSourceDescClass
{
  GObjectClass parent_class;
};

/**
 * ShumateMapSourceConstructor:
 * @desc: a #ShumateMapSourceDesc
 *
 * A #ShumateMapSource constructor.  It should return a ready to use
 * #ShumateMapSource.
 *
 * Returns: A fully constructed #ShumateMapSource ready to be used.
 *
 * Since: 0.10
 */
typedef ShumateMapSource* (*ShumateMapSourceConstructor) (ShumateMapSourceDesc *desc);

/**
 * SHUMATE_MAP_SOURCE_CONSTRUCTOR:
 *
 * Conversion macro to #ShumateMapSourceConstructor.
 *
 * Since: 0.10
 */
#define SHUMATE_MAP_SOURCE_CONSTRUCTOR (f) ((ShumateMapSourceConstructor) (f))

GType shumate_map_source_desc_get_type (void);

ShumateMapSourceDesc *shumate_map_source_desc_new_full (
    gchar *id,
    gchar *name,
    gchar *license,
    gchar *license_uri,
    guint min_zoom,
    guint max_zoom,
    guint tile_size,
    ShumateMapProjection projection,
    gchar *uri_format,
    ShumateMapSourceConstructor constructor,
    gpointer data);

const gchar *shumate_map_source_desc_get_id (ShumateMapSourceDesc *desc);
const gchar *shumate_map_source_desc_get_name (ShumateMapSourceDesc *desc);
const gchar *shumate_map_source_desc_get_license (ShumateMapSourceDesc *desc);
const gchar *shumate_map_source_desc_get_license_uri (ShumateMapSourceDesc *desc);
const gchar *shumate_map_source_desc_get_uri_format (ShumateMapSourceDesc *desc);
guint shumate_map_source_desc_get_min_zoom_level (ShumateMapSourceDesc *desc);
guint shumate_map_source_desc_get_max_zoom_level (ShumateMapSourceDesc *desc);
guint shumate_map_source_desc_get_tile_size (ShumateMapSourceDesc *desc);
ShumateMapProjection shumate_map_source_desc_get_projection (ShumateMapSourceDesc *desc);
gpointer shumate_map_source_desc_get_data (ShumateMapSourceDesc *desc);
ShumateMapSourceConstructor shumate_map_source_desc_get_constructor (ShumateMapSourceDesc *desc);

G_END_DECLS

#endif
