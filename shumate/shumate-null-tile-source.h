/*
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

#ifndef _SHUMATE_NULL_TILE_SOURCE
#define _SHUMATE_NULL_TILE_SOURCE
#include <glib-object.h>

#include <shumate/shumate-tile-source.h>

G_BEGIN_DECLS

#define SHUMATE_TYPE_NULL_TILE_SOURCE shumate_null_tile_source_get_type ()

#define SHUMATE_NULL_TILE_SOURCE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SHUMATE_TYPE_NULL_TILE_SOURCE, ShumateNullTileSource))

#define SHUMATE_NULL_TILE_SOURCE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SHUMATE_TYPE_NULL_TILE_SOURCE, ShumateNullTileSourceClass))

#define SHUMATE_IS_NULL_TILE_SOURCE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SHUMATE_TYPE_NULL_TILE_SOURCE))

#define SHUMATE_IS_NULL_TILE_SOURCE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SHUMATE_TYPE_NULL_TILE_SOURCE))

#define SHUMATE_NULL_TILE_SOURCE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), SHUMATE_TYPE_NULL_TILE_SOURCE, ShumateNullTileSourceClass))

typedef struct _ShumateNullTileSourcePrivate ShumateNullTileSourcePrivate;

/**
 * ShumateNullTileSource:
 *
 * The #ShumateNullTileSource structure contains only private data
 * and should be accessed using the provided API
 *
 * Since: 0.8
 */
typedef struct _ShumateNullTileSource ShumateNullTileSource;
typedef struct _ShumateNullTileSourceClass ShumateNullTileSourceClass;

struct _ShumateNullTileSource
{
  ShumateTileSource parent;
};

struct _ShumateNullTileSourceClass
{
  ShumateTileSourceClass parent_class;
};

GType shumate_null_tile_source_get_type (void);

ShumateNullTileSource *shumate_null_tile_source_new_full (ShumateRenderer *renderer);


G_END_DECLS

#endif /* _SHUMATE_NULL_TILE_SOURCE */
