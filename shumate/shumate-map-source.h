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

#ifndef _SHUMATE_MAP_SOURCE_H_
#define _SHUMATE_MAP_SOURCE_H_

#include <shumate/shumate-defines.h>
#include <shumate/shumate-tile.h>
#include <shumate/shumate-renderer.h>

G_BEGIN_DECLS

#define SHUMATE_TYPE_MAP_SOURCE shumate_map_source_get_type ()

#define SHUMATE_MAP_SOURCE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SHUMATE_TYPE_MAP_SOURCE, ShumateMapSource))

#define SHUMATE_MAP_SOURCE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SHUMATE_TYPE_MAP_SOURCE, ShumateMapSourceClass))

#define SHUMATE_IS_MAP_SOURCE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SHUMATE_TYPE_MAP_SOURCE))

#define SHUMATE_IS_MAP_SOURCE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SHUMATE_TYPE_MAP_SOURCE))

#define SHUMATE_MAP_SOURCE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), SHUMATE_TYPE_MAP_SOURCE, ShumateMapSourceClass))

typedef struct _ShumateMapSourcePrivate ShumateMapSourcePrivate;

typedef struct _ShumateMapSourceClass ShumateMapSourceClass;

/**
 * ShumateMapProjection:
 * @SHUMATE_MAP_PROJECTION_MERCATOR: Currently the only supported projection
 *
 * Projections supported by the library.
 */
typedef enum
{
  SHUMATE_MAP_PROJECTION_MERCATOR
} ShumateMapProjection;

/**
 * ShumateMapSource:
 *
 * The #ShumateMapSource structure contains only private data
 * and should be accessed using the provided API
 *
 * Since: 0.4
 */
struct _ShumateMapSource
{
  GInitiallyUnowned parent_instance;

  ShumateMapSourcePrivate *priv;
};

struct _ShumateMapSourceClass
{
  GInitiallyUnownedClass parent_class;

  const gchar * (*get_id)(ShumateMapSource *map_source);
  const gchar * (*get_name)(ShumateMapSource *map_source);
  const gchar * (*get_license)(ShumateMapSource *map_source);
  const gchar * (*get_license_uri)(ShumateMapSource *map_source);
  guint (*get_min_zoom_level)(ShumateMapSource *map_source);
  guint (*get_max_zoom_level)(ShumateMapSource *map_source);
  guint (*get_tile_size)(ShumateMapSource *map_source);
  ShumateMapProjection (*get_projection)(ShumateMapSource *map_source);

  void (*fill_tile)(ShumateMapSource *map_source,
      ShumateTile *tile);
};

GType shumate_map_source_get_type (void);

ShumateMapSource *shumate_map_source_get_next_source (ShumateMapSource *map_source);
void shumate_map_source_set_next_source (ShumateMapSource *map_source,
    ShumateMapSource *next_source);

ShumateRenderer *shumate_map_source_get_renderer (ShumateMapSource *map_source);
void shumate_map_source_set_renderer (ShumateMapSource *map_source,
    ShumateRenderer *renderer);

const gchar *shumate_map_source_get_id (ShumateMapSource *map_source);
const gchar *shumate_map_source_get_name (ShumateMapSource *map_source);
const gchar *shumate_map_source_get_license (ShumateMapSource *map_source);
const gchar *shumate_map_source_get_license_uri (ShumateMapSource *map_source);
guint shumate_map_source_get_min_zoom_level (ShumateMapSource *map_source);
guint shumate_map_source_get_max_zoom_level (ShumateMapSource *map_source);
guint shumate_map_source_get_tile_size (ShumateMapSource *map_source);
ShumateMapProjection shumate_map_source_get_projection (ShumateMapSource *map_source);

gdouble shumate_map_source_get_x (ShumateMapSource *map_source,
    guint zoom_level,
    gdouble longitude);
gdouble shumate_map_source_get_y (ShumateMapSource *map_source,
    guint zoom_level,
    gdouble latitude);
gdouble shumate_map_source_get_longitude (ShumateMapSource *map_source,
    guint zoom_level,
    gdouble x);
gdouble shumate_map_source_get_latitude (ShumateMapSource *map_source,
    guint zoom_level,
    gdouble y);
guint shumate_map_source_get_row_count (ShumateMapSource *map_source,
    guint zoom_level);
guint shumate_map_source_get_column_count (ShumateMapSource *map_source,
    guint zoom_level);
gdouble shumate_map_source_get_meters_per_pixel (ShumateMapSource *map_source,
    guint zoom_level,
    gdouble latitude,
    gdouble longitude);

void shumate_map_source_fill_tile (ShumateMapSource *map_source,
    ShumateTile *tile);

G_END_DECLS

#endif /* _SHUMATE_MAP_SOURCE_H_ */
