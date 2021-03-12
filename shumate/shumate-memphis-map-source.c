/*
 * Copyright 2021 Collabora Ltd. (https://www.collabora.com)
 * Copyright 2021 Corentin Noël <corentin.noel@collabora.com>
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

#include "shumate-memphis-map-source.h"
#include <memphis/memphis.h>

struct _ShumateMemphisMapSource
{
  ShumateMapSource parent_instance;

  MemphisRuleSet *rules;
  MemphisMap *map;
  MemphisRenderer *renderer;
};

G_DEFINE_TYPE (ShumateMemphisMapSource, shumate_memphis_map_source, SHUMATE_TYPE_MAP_SOURCE)

enum {
  PROP_0,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];

static const char *
shumate_memphis_map_source_get_id (ShumateMapSource *map_source)
{
  g_return_val_if_fail (SHUMATE_IS_MEMPHIS_MAP_SOURCE (map_source), NULL);

  return "memphis";
}

static const char *
shumate_memphis_map_source_get_name (ShumateMapSource *map_source)
{
  g_return_val_if_fail (SHUMATE_IS_MEMPHIS_MAP_SOURCE (map_source), NULL);

  return "Memphis";
}

static const char *
shumate_memphis_map_source_get_license (ShumateMapSource *map_source)
{
  g_return_val_if_fail (SHUMATE_IS_MEMPHIS_MAP_SOURCE (map_source), NULL);

  return "NONE";
}

static const char *
shumate_memphis_map_source_get_license_uri (ShumateMapSource *map_source)
{
  g_return_val_if_fail (SHUMATE_IS_MEMPHIS_MAP_SOURCE (map_source), NULL);

  return "NONE";
}

static guint
shumate_memphis_map_source_get_min_zoom_level (ShumateMapSource *map_source)
{
  g_return_val_if_fail (SHUMATE_IS_MEMPHIS_MAP_SOURCE (map_source), 0);

  return 12;
}

static guint
shumate_memphis_map_source_get_max_zoom_level (ShumateMapSource *map_source)
{
  g_return_val_if_fail (SHUMATE_IS_MEMPHIS_MAP_SOURCE (map_source), 0);

  return 14;
}

static guint
shumate_memphis_map_source_get_tile_size (ShumateMapSource *map_source)
{
  g_return_val_if_fail (SHUMATE_IS_MEMPHIS_MAP_SOURCE (map_source), 0);

  return 512;
}

static ShumateMapProjection
shumate_memphis_map_source_get_projection (ShumateMapSource *map_source)
{
  g_return_val_if_fail (SHUMATE_IS_MEMPHIS_MAP_SOURCE (map_source), SHUMATE_MAP_PROJECTION_MERCATOR);

  return SHUMATE_MAP_PROJECTION_MERCATOR;
}

static void
shumate_memphis_map_source_fill_tile (ShumateMapSource *map_source,
                                      ShumateTile      *tile,
                                      GCancellable     *cancellable)
{
  ShumateMemphisMapSource *self = (ShumateMemphisMapSource *)map_source;
  g_autoptr(GdkTexture) texture = NULL;
  g_autoptr(GBytes) bytes = NULL;
  int width = gtk_widget_get_width (GTK_WIDGET (tile));
  int height = gtk_widget_get_width (GTK_WIDGET (tile));
  int x = shumate_tile_get_x (tile);
  int y = shumate_tile_get_y (tile);
  int zoom_level = shumate_tile_get_zoom_level (tile);
  cairo_surface_t *surface;
  cairo_t *cr;

  if (!memphis_renderer_tile_has_data (self->renderer, x, y, zoom_level)) {
    g_critical ("Unable to render tile at: %d %d %d", x, y, zoom_level);
  }

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 512, 512);
  cr =  cairo_create(surface);
  memphis_renderer_draw_tile (self->renderer, cr, x, y, zoom_level);
  cairo_destroy(cr);

  cairo_surface_flush(surface);
  int stride = cairo_image_surface_get_stride (surface);
  bytes = g_bytes_new (cairo_image_surface_get_data (surface), cairo_image_surface_get_width (surface) * stride);
  texture = gdk_memory_texture_new (width, height, GDK_MEMORY_DEFAULT, bytes, stride);
  cairo_surface_destroy(surface);

  shumate_tile_set_texture (tile, texture);
  shumate_tile_set_fade_in (tile, TRUE);
  shumate_tile_set_state (tile, SHUMATE_STATE_DONE);
}

static void
shumate_memphis_map_source_dispose (GObject *object)
{
  ShumateMemphisMapSource *self = (ShumateMemphisMapSource *)object;

  g_clear_object (&self->renderer);
  g_clear_object (&self->map);
  g_clear_object (&self->rules);

  G_OBJECT_CLASS (shumate_memphis_map_source_parent_class)->dispose (object);
}

static void
shumate_memphis_map_source_finalize (GObject *object)
{
  ShumateMemphisMapSource *self = (ShumateMemphisMapSource *)object;

  G_OBJECT_CLASS (shumate_memphis_map_source_parent_class)->finalize (object);
}

static void
shumate_memphis_map_source_get_property (GObject    *object,
                                         guint       prop_id,
                                         GValue     *value,
                                         GParamSpec *pspec)
{
  ShumateMemphisMapSource *self = SHUMATE_MEMPHIS_MAP_SOURCE (object);

  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
shumate_memphis_map_source_set_property (GObject      *object,
                                         guint         prop_id,
                                         const GValue *value,
                                         GParamSpec   *pspec)
{
  ShumateMemphisMapSource *self = SHUMATE_MEMPHIS_MAP_SOURCE (object);

  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
shumate_memphis_map_source_class_init (ShumateMemphisMapSourceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ShumateMapSourceClass *map_source_class = SHUMATE_MAP_SOURCE_CLASS (klass);

  object_class->dispose = shumate_memphis_map_source_dispose;
  object_class->finalize = shumate_memphis_map_source_finalize;
  object_class->get_property = shumate_memphis_map_source_get_property;
  object_class->set_property = shumate_memphis_map_source_set_property;

  map_source_class->get_id = shumate_memphis_map_source_get_id;
  map_source_class->get_name = shumate_memphis_map_source_get_name;
  map_source_class->get_license = shumate_memphis_map_source_get_license;
  map_source_class->get_license_uri = shumate_memphis_map_source_get_license_uri;
  map_source_class->get_min_zoom_level = shumate_memphis_map_source_get_min_zoom_level;
  map_source_class->get_max_zoom_level = shumate_memphis_map_source_get_max_zoom_level;
  map_source_class->get_tile_size = shumate_memphis_map_source_get_tile_size;
  map_source_class->get_projection = shumate_memphis_map_source_get_projection;
  map_source_class->fill_tile = shumate_memphis_map_source_fill_tile;
}

static gchar *map_file  = "map.osm";
static gchar *rule_file = "default-rules.xml";

static void
shumate_memphis_map_source_init (ShumateMemphisMapSource *self)
{
  g_autoptr(GError) error = NULL;

  self->rules = memphis_rule_set_new ();
  if (!memphis_rule_set_load_from_file (self->rules, rule_file, &error)) {
    g_critical ("%s", error->message);
    g_clear_error (&error);
    return;
  }

  self->map = memphis_map_new ();
  if (!memphis_map_load_from_file (self->map, map_file, &error)) {
    g_critical ("%s", error->message);
    g_clear_error (&error);
    return;
  }

  self->renderer = memphis_renderer_new_full (self->rules, self->map);
  memphis_renderer_set_resolution (self->renderer, 512);
}

/**
 * shumate_memphis_map_source_new:
 *
 * Create a new #ShumateMemphisMapSource.
 *
 * Returns: (transfer full): a newly created #ShumateMemphisMapSource
 */
ShumateMemphisMapSource *
shumate_memphis_map_source_new (void)
{
  return g_object_new (SHUMATE_TYPE_MEMPHIS_MAP_SOURCE, NULL);
}
