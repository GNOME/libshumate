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

#include "shumate-vector-renderer-private.h"
#include "shumate-tile-downloader.h"
#include "shumate-tile-private.h"
#include "shumate-profiling-private.h"
#include "shumate-vector-reader.h"
#include "shumate-vector-reader-iter.h"

/**
 * ShumateVectorRenderer:
 *
 * A [class@MapSource] that renders tiles from a given vector data source.
 */

#ifdef SHUMATE_HAS_VECTOR_RENDERER
#include <json-glib/json-glib.h>
#include <cairo/cairo.h>

#include "vector/shumate-vector-render-scope-private.h"
#include "vector/shumate-vector-symbol-info-private.h"
#include "vector/shumate-vector-utils-private.h"
#include "vector/shumate-vector-layer-private.h"
#include "vector/shumate-vector-index-private.h"
#endif

struct _ShumateVectorRenderer
{
  ShumateMapSource parent_instance;

  char *source_name;
  ShumateDataSource *data_source;

  ShumateVectorSpriteSheet *sprites;
  GMutex sprites_mutex;

  GThreadPool *thread_pool;

  char *style_json;

  GPtrArray *layers;

#ifdef SHUMATE_HAS_VECTOR_RENDERER
  ShumateVectorIndexDescription *index_description;
#endif
};


static gboolean begin_render (ShumateVectorRenderer  *self,
                              GTask                  *task,
                              GBytes                 *tile_data,
                              ShumateGridPosition    *source_position);

static void shumate_vector_renderer_initable_iface_init (GInitableIface *iface);

G_DEFINE_TYPE_WITH_CODE (ShumateVectorRenderer, shumate_vector_renderer, SHUMATE_TYPE_MAP_SOURCE,
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, shumate_vector_renderer_initable_iface_init))

enum {
  PROP_0,
  PROP_STYLE_JSON,
  PROP_SPRITE_SHEET,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];


/**
 * shumate_vector_renderer_new:
 * @id: an ID for the map source
 * @style_json: a vector style
 * @error: return location for a #GError, or %NULL
 *
 * Creates a new [class@VectorRenderer] from the given JSON style.
 *
 * The stylesheet should contain a list of tile sources. Tiles will be
 * downloaded using [class@TileDownloader]s.
 *
 * See the [MapLibre Style Specification](https://maplibre.org/maplibre-gl-js-docs/style-spec/)
 * for details on @style_json, but be aware that libshumate does not support
 * every feature of the specification.
 *
 * Returns: (transfer full): a newly constructed [class@VectorRenderer], or %NULL if @error is set
 */
ShumateVectorRenderer *
shumate_vector_renderer_new (const char  *id,
                             const char  *style_json,
                             GError     **error)
{
  g_return_val_if_fail (id != NULL, NULL);
  g_return_val_if_fail (style_json != NULL, NULL);

  return g_initable_new (SHUMATE_TYPE_VECTOR_RENDERER, NULL, error,
                         "id", id,
                         "style-json", style_json,
                         NULL);
}


/**
 * shumate_vector_renderer_is_supported:
 *
 * Checks whether libshumate was compiled with vector tile support. If it was
 * not, vector renderers cannot be created or used.
 *
 * Returns: %TRUE if libshumate was compiled with `-Dvector_renderer=true` or
 * %FALSE if it was not
 */
gboolean
shumate_vector_renderer_is_supported (void)
{
#ifdef SHUMATE_HAS_VECTOR_RENDERER
  return TRUE;
#else
  return FALSE;
#endif
}


static void
shumate_vector_renderer_finalize (GObject *object)
{
  ShumateVectorRenderer *self = (ShumateVectorRenderer *)object;

  g_clear_pointer (&self->layers, g_ptr_array_unref);
  g_clear_pointer (&self->style_json, g_free);
  g_clear_pointer (&self->source_name, g_free);
  g_clear_object (&self->data_source);
  g_clear_object (&self->sprites);
#ifdef SHUMATE_HAS_VECTOR_RENDERER
  g_clear_pointer (&self->index_description, shumate_vector_index_description_free);
#endif

  if (self->thread_pool)
    g_thread_pool_free (self->thread_pool, FALSE, FALSE);

  g_mutex_clear (&self->sprites_mutex);

  G_OBJECT_CLASS (shumate_vector_renderer_parent_class)->finalize (object);
}

static void
shumate_vector_renderer_get_property (GObject    *object,
                                      guint       prop_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  ShumateVectorRenderer *self = SHUMATE_VECTOR_RENDERER (object);

  switch (prop_id)
    {
    case PROP_STYLE_JSON:
      g_value_set_string (value, self->style_json);
      break;
    case PROP_SPRITE_SHEET:
      g_value_set_object (value, shumate_vector_renderer_get_sprite_sheet (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
shumate_vector_renderer_set_property (GObject      *object,
                                      guint         prop_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  ShumateVectorRenderer *self = SHUMATE_VECTOR_RENDERER (object);

  switch (prop_id)
    {
    case PROP_STYLE_JSON:
      /* Property is construct only, so it should only be set once */
      g_assert (self->style_json == NULL);
      self->style_json = g_strdup (g_value_get_string (value));
      break;
    case PROP_SPRITE_SHEET:
      shumate_vector_renderer_set_sprite_sheet (self, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void shumate_vector_renderer_fill_tile_async (ShumateMapSource    *map_source,
                                                     ShumateTile         *tile,
                                                     GCancellable        *cancellable,
                                                     GAsyncReadyCallback  callback,
                                                     gpointer             user_data);

static gboolean shumate_vector_renderer_fill_tile_finish (ShumateMapSource  *map_source,
                                                          GAsyncResult      *result,
                                                          GError           **error);

static void
shumate_vector_renderer_class_init (ShumateVectorRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ShumateMapSourceClass *map_source_class = SHUMATE_MAP_SOURCE_CLASS (klass);

  object_class->finalize = shumate_vector_renderer_finalize;
  object_class->get_property = shumate_vector_renderer_get_property;
  object_class->set_property = shumate_vector_renderer_set_property;

  map_source_class->fill_tile_async = shumate_vector_renderer_fill_tile_async;
  map_source_class->fill_tile_finish = shumate_vector_renderer_fill_tile_finish;

  /**
   * ShumateVectorRenderer:style-json:
   *
   * A map style, in [Mapbox Style Specification](https://docs.mapbox.com/mapbox-gl-js/style-spec/)
   * format.
   *
   * Note that not all features of the specification are supported.
   */
  properties[PROP_STYLE_JSON] =
    g_param_spec_string ("style-json",
                         "Style JSON",
                         "Style JSON",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * ShumateVectorRenderer:sprite-sheet:
   *
   * The sprite sheet used to render icons and textures.
   *
   * Since: 1.1
   */
  properties[PROP_SPRITE_SHEET] =
    g_param_spec_object ("sprite-sheet",
                         "sprite-sheet",
                         "sprite-sheet",
                         SHUMATE_TYPE_VECTOR_SPRITE_SHEET,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);
}


static gboolean
shumate_vector_renderer_initable_init (GInitable     *initable,
                                       GCancellable  *cancellable,
                                       GError       **error)
{
  SHUMATE_PROFILE_START ();

#ifdef SHUMATE_HAS_VECTOR_RENDERER
  ShumateVectorRenderer *self = (ShumateVectorRenderer *)initable;
  g_autoptr(JsonNode) node = NULL;
  JsonNode *layers_node;
  JsonNode *sources_node;
  JsonObject *object;
  const char *style_name;

  g_return_val_if_fail (SHUMATE_IS_VECTOR_RENDERER (self), FALSE);
  g_return_val_if_fail (self->style_json != NULL, FALSE);

  if (!(node = json_from_string (self->style_json, error)))
    return FALSE;

  if (!shumate_vector_json_get_object (node, &object, error))
    return FALSE;

  if (!shumate_vector_json_get_string_member (object, "name", &style_name, error))
    return FALSE;
  if (style_name != NULL)
    shumate_map_source_set_name (SHUMATE_MAP_SOURCE (self), style_name);

  if ((sources_node = json_object_get_member (object, "sources")) == NULL)
    {
      g_set_error (error,
                   SHUMATE_STYLE_ERROR,
                   SHUMATE_STYLE_ERROR_UNSUPPORTED,
                   "a data source is required");
      return FALSE;
    }
  else
    {
      JsonObject *sources;
      JsonObjectIter iter;
      const char *source_name;
      JsonNode *source_node;
      int minzoom = 30, maxzoom = 0;

      if (!shumate_vector_json_get_object (sources_node, &sources, error))
        return FALSE;

      if (json_object_get_size (sources) > 1)
        {
          /* TODO: Support multiple data sources */
          g_set_error (error,
                       SHUMATE_STYLE_ERROR,
                       SHUMATE_STYLE_ERROR_UNSUPPORTED,
                       "ShumateVectorRenderer does not currently support multiple data sources");
          return FALSE;
        }
      else if (json_object_get_size (object) == 0)
        {
          g_set_error (error,
                       SHUMATE_STYLE_ERROR,
                       SHUMATE_STYLE_ERROR_UNSUPPORTED,
                       "a data source is required");
          return FALSE;
        }

      json_object_iter_init (&iter, sources);
      while (json_object_iter_next (&iter, &source_name, &source_node))
        {
          JsonObject *source_object;
          const char *source_type;
          const char *url;
          JsonArray *tiles;
          const char *url_template;
          ShumateDataSource *data_source;

          if (!shumate_vector_json_get_object (source_node, &source_object, error))
            return FALSE;

          if (!shumate_vector_json_get_string_member (source_object, "type", &source_type, error)
              || !shumate_vector_json_get_string_member (source_object, "url", &url, error)
              || !shumate_vector_json_get_array_member (source_object, "tiles", &tiles, error))
            return FALSE;

          if (g_strcmp0 (source_type, "vector") != 0)
            {
              g_set_error (error,
                           SHUMATE_STYLE_ERROR,
                           SHUMATE_STYLE_ERROR_UNSUPPORTED,
                           "ShumateVectorRenderer currently only supports vector sources.");
              return FALSE;
            }

          if (url != NULL)
            {
              g_set_error (error,
                           SHUMATE_STYLE_ERROR,
                           SHUMATE_STYLE_ERROR_UNSUPPORTED,
                           "ShumateVectorRenderer does not currently support TileJSON links. "
                           "Please embed the TileJSON data directly into the style.");
              return FALSE;
            }

          if (tiles == NULL || json_array_get_length (tiles) == 0)
            {
              g_set_error (error,
                           SHUMATE_STYLE_ERROR,
                           SHUMATE_STYLE_ERROR_MALFORMED_STYLE,
                           "Expected 'tiles' array to contain at least one element");
              return FALSE;
            }

          if (!shumate_vector_json_get_string (json_array_get_element (tiles, 0),
                                               &url_template,
                                               error))
            return FALSE;

          minzoom = json_object_get_int_member_with_default (source_object, "minzoom", 0);
          maxzoom = json_object_get_int_member_with_default (source_object, "maxzoom", 30);

          data_source = SHUMATE_DATA_SOURCE (shumate_tile_downloader_new (url_template));
          shumate_data_source_set_min_zoom_level (data_source, minzoom);
          shumate_data_source_set_max_zoom_level (data_source, maxzoom);

          self->source_name = g_strdup (source_name);
          self->data_source = data_source;
        }

      maxzoom = MAX (maxzoom, 18);

      if (minzoom < maxzoom)
        {
          shumate_map_source_set_min_zoom_level (SHUMATE_MAP_SOURCE (self), minzoom);
          shumate_map_source_set_max_zoom_level (SHUMATE_MAP_SOURCE (self), maxzoom);
        }
    }

  self->layers = g_ptr_array_new_with_free_func (g_object_unref);
  if ((layers_node = json_object_get_member (object, "layers")))
    {
      JsonArray *layers;

      if (!shumate_vector_json_get_array (layers_node, &layers, error))
        return FALSE;

      for (int i = 0, n = json_array_get_length (layers); i < n; i ++)
        {
          JsonNode *layer_node = json_array_get_element (layers, i);
          JsonObject *layer_obj;
          const char *layer_id;
          ShumateVectorLayer *layer;
          ShumateVectorExpression *filter;

          if (!shumate_vector_json_get_object (layer_node, &layer_obj, error))
            return FALSE;

          if (!shumate_vector_json_get_string_member (layer_obj, "id", &layer_id, error))
            return FALSE;

          if (!(layer = shumate_vector_layer_create_from_json (layer_obj, error)))
            {
              g_prefix_error (error, "layer '%s': ", layer_id);
              return FALSE;
            }

          g_ptr_array_add (self->layers, layer);
          filter = shumate_vector_layer_get_filter (layer);
          if (filter != NULL)
            shumate_vector_expression_collect_indexes (filter, shumate_vector_layer_get_source_layer (layer), self->index_description);
        }
    }

  /* According to the style spec, this is not configurable for vector tiles */
  shumate_map_source_set_tile_size (SHUMATE_MAP_SOURCE (self), 512);

  return TRUE;
#else
  g_set_error (error,
               SHUMATE_STYLE_ERROR,
               SHUMATE_STYLE_ERROR_SUPPORT_OMITTED,
               "Libshumate was compiled without support for vector tiles, so a "
               "ShumateVectorRenderer may not be constructed. You can fix this "
               "by compiling libshumate with `-Dvector_renderer=true` or by "
               "checking `shumate_vector_renderer_is_supported ()` before trying "
               "to construct a ShumateVectorRenderer.");
  return FALSE;
#endif
}


static void
shumate_vector_renderer_initable_iface_init (GInitableIface *iface)
{
  iface->init = shumate_vector_renderer_initable_init;
}


/* An item in the thread pool queue to render a tile from received data. */
typedef struct {
  GTask *task;
  GCancellable *cancellable;
  gulong cancellable_handle;
  GBytes *data;
  ShumateGridPosition source_position;

  GdkPaintable *paintable;
  GPtrArray *symbols;
} RenderJob;


static void
render_job_unref (RenderJob *job)
{
  if (job->cancellable_handle != 0)
    g_cancellable_disconnect (g_task_get_cancellable (job->task), job->cancellable_handle);
  g_clear_object (&job->cancellable);
  g_clear_object (&job->task);
  g_clear_pointer (&job->data, g_bytes_unref);
  g_clear_object (&job->paintable);
  g_clear_pointer (&job->symbols, g_ptr_array_unref);
  g_free (job);
}

/* The data associated with a shumate_vector_renderer_fill_tile_async() task. */
typedef struct {
  ShumateTile *tile;
  RenderJob *current_job;
  ShumateDataSourceRequest *req;
  guint8 completed : 1;
} TaskData;

static void
task_data_free (TaskData *data)
{
  g_clear_object (&data->tile);
  g_clear_object (&data->req);
  g_free (data);
}


static void
shumate_vector_renderer_init (ShumateVectorRenderer *self)
{
  g_mutex_init (&self->sprites_mutex);
#ifdef SHUMATE_HAS_VECTOR_RENDERER
  self->index_description = shumate_vector_index_description_new ();
#endif
}


/**
 * shumate_vector_renderer_get_style_json:
 * @self: a [class@VectorStyle]
 *
 * Gets the JSON string from which this vector style was loaded.
 *
 * Returns: (nullable): the style JSON, or %NULL if none is set
 */
const char *
shumate_vector_renderer_get_style_json (ShumateVectorRenderer *self)
{
  g_return_val_if_fail (SHUMATE_IS_VECTOR_RENDERER (self), NULL);

  return self->style_json;
}


/**
 * shumate_vector_renderer_set_sprite_sheet_data:
 * @self: a [class@VectorRenderer]
 * @sprites_pixbuf: a [class@GdkPixbuf.Pixbuf]
 * @sprites_json: a JSON string
 * @error: return location for a #GError, or %NULL
 *
 * Sets the sprite sheet used by the style JSON to render icons and textures.
 *
 * The existing [property@VectorRenderer:sprite-sheet] property will be replaced
 * with a new instance of [class@VectorSpriteSheet].
 *
 * Returns: whether the sprite sheet was loaded successfully
 *
 * Deprecated: 1.1: Use the methods of [property@VectorRenderer:sprite-sheet] instead.
 */
gboolean
shumate_vector_renderer_set_sprite_sheet_data (ShumateVectorRenderer  *self,
                                               GdkPixbuf              *sprites_pixbuf,
                                               const char             *sprites_json,
                                               GError                **error)
{
  g_autoptr(ShumateVectorSpriteSheet) sprites = NULL;
  g_autoptr(GdkTexture) texture = NULL;

  g_return_val_if_fail (SHUMATE_IS_VECTOR_RENDERER (self), FALSE);
  g_return_val_if_fail (GDK_IS_PIXBUF (sprites_pixbuf), FALSE);
  g_return_val_if_fail (sprites_json != NULL, FALSE);

  sprites = shumate_vector_sprite_sheet_new ();

  texture = gdk_texture_new_for_pixbuf (sprites_pixbuf);
  if (!shumate_vector_sprite_sheet_add_page (sprites, texture, sprites_json, 1, error))
    return FALSE;

  shumate_vector_renderer_set_sprite_sheet (self, sprites);
  return TRUE;
}


/**
 * shumate_vector_renderer_get_sprite_sheet:
 * @self: a [class@VectorRenderer]
 *
 * Gets the sprite sheet used to render icons and textures.
 *
 * Returns: (transfer none): the [class@VectorSpriteSheet]
 *
 * Since: 1.1
 */
ShumateVectorSpriteSheet *
shumate_vector_renderer_get_sprite_sheet (ShumateVectorRenderer *self)
{
  g_autoptr(GMutexLocker) locker = NULL;

  g_return_val_if_fail (SHUMATE_IS_VECTOR_RENDERER (self), FALSE);

  locker = g_mutex_locker_new (&self->sprites_mutex);

  if (self->sprites == NULL)
    self->sprites = shumate_vector_sprite_sheet_new ();

  return self->sprites;
}


/**
 * shumate_vector_renderer_set_sprite_sheet:
 * @self: a [class@VectorRenderer]
 * @sprites: a [class@VectorSpriteSheet]
 *
 * Sets the sprite sheet used to render icons and textures.
 *
 * Since: 1.1
 */
void
shumate_vector_renderer_set_sprite_sheet (ShumateVectorRenderer    *self,
                                          ShumateVectorSpriteSheet *sprites)
{
  g_autoptr(GMutexLocker) locker = NULL;

  g_return_if_fail (SHUMATE_IS_VECTOR_RENDERER (self));
  g_return_if_fail (SHUMATE_IS_VECTOR_SPRITE_SHEET (sprites));

  locker = g_mutex_locker_new (&self->sprites_mutex);

  if (g_set_object (&self->sprites, sprites))
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SPRITE_SHEET]);
}


/**
 * shumate_vector_renderer_set_data_source:
 * @self: a [class@VectorRenderer]
 * @name: the name of the data source
 * @data_source: a [class@DataSource]
 *
 * Adds a data source to the renderer.
 *
 * Currently, [class@VectorRenderer] only supports one data source
 * and throws an error if the style does not contain exactly one
 * data source. However, support for multiple sources may be added
 * in the future, so this method accepts a name parameter. If the
 * name does not match the one expected by the style, this method
 * will have no effect.
 *
 * Since: 1.2
 */
void
shumate_vector_renderer_set_data_source (ShumateVectorRenderer *self,
                                         const char            *name,
                                         ShumateDataSource     *data_source)
{
  g_return_if_fail (SHUMATE_IS_VECTOR_RENDERER (self));
  g_return_if_fail (name != NULL);
  g_return_if_fail (SHUMATE_IS_DATA_SOURCE (data_source));

  if (g_strcmp0 (name, self->source_name) == 0)
    g_set_object (&self->data_source, data_source);
}


#ifdef SHUMATE_HAS_VECTOR_RENDERER
static GdkTexture *
texture_new_for_surface (cairo_surface_t *surface)
{
  g_autoptr(GBytes) bytes = NULL;
  GdkTexture *texture;

  g_return_val_if_fail (cairo_surface_get_type (surface) == CAIRO_SURFACE_TYPE_IMAGE, NULL);
  g_return_val_if_fail (cairo_image_surface_get_width (surface) > 0, NULL);
  g_return_val_if_fail (cairo_image_surface_get_height (surface) > 0, NULL);

  bytes = g_bytes_new_with_free_func (cairo_image_surface_get_data (surface),
                                      (gsize) cairo_image_surface_get_height (surface)
                                      * (gsize) cairo_image_surface_get_stride (surface),
                                      (GDestroyNotify) cairo_surface_destroy,
                                      cairo_surface_reference (surface));

  texture = gdk_memory_texture_new (cairo_image_surface_get_width (surface),
                                    cairo_image_surface_get_height (surface),
                                    GDK_MEMORY_B8G8R8A8_PREMULTIPLIED,
                                    bytes,
                                    cairo_image_surface_get_stride (surface));

  return texture;
}
#endif


static void
get_source_coordinates (ShumateVectorRenderer *self,
                        int                   *x,
                        int                   *y,
                        int                   *zoom_level)
{
  /* Figure out which tile from the data source should be used to render the
   * given tile (which will be different if we're overzooming). */

  int maxzoom = shumate_data_source_get_max_zoom_level (self->data_source);
  if (*zoom_level > maxzoom)
    {
      *x >>= (*zoom_level - maxzoom);
      *y >>= (*zoom_level - maxzoom);
      *zoom_level = maxzoom;
    }
}

static void
on_request_notify (ShumateDataSourceRequest *req,
                   GParamSpec               *pspec,
                   gpointer                  user_data)
{
  GTask *task = user_data;
  GBytes *data;
  ShumateVectorRenderer *self = g_task_get_source_object (task);

  if ((data = shumate_data_source_request_get_data (req)))
    {
      begin_render (
        self,
        task,
        data,
        &SHUMATE_GRID_POSITION_INIT (
          shumate_data_source_request_get_x (req),
          shumate_data_source_request_get_y (req),
          shumate_data_source_request_get_zoom_level (req)
        )
      );
    }
}


static void
return_from_task (GTask *task, ShumateDataSourceRequest *req)
{
  GError *error;
  TaskData *data = g_task_get_task_data (task);

  shumate_tile_set_state (data->tile, SHUMATE_STATE_DONE);

  if ((error = shumate_data_source_request_get_error (req)))
    g_task_return_error (task, g_error_copy (error));
  else
    g_task_return_boolean (task, TRUE);
}

static void
on_request_notify_completed (ShumateDataSourceRequest *req,
                             GParamSpec               *pspec,
                             gpointer                  user_data)
{
  g_autoptr(GTask) task = user_data;
  TaskData *data = g_task_get_task_data (task);

  if (data->current_job != NULL)
    data->completed = TRUE;
  else
    return_from_task (task, req);
}

static void
shumate_vector_renderer_fill_tile_async (ShumateMapSource    *map_source,
                                         ShumateTile         *tile,
                                         GCancellable        *cancellable,
                                         GAsyncReadyCallback  callback,
                                         gpointer             user_data)
{
  ShumateVectorRenderer *self = (ShumateVectorRenderer *)map_source;
  g_autoptr(GTask) task = NULL;
  int x, y, zoom_level;
  g_autoptr(ShumateDataSourceRequest) req = NULL;
  GBytes *data;
  TaskData *task_data;

  g_return_if_fail (SHUMATE_IS_VECTOR_RENDERER (self));
  g_return_if_fail (SHUMATE_IS_TILE (tile));
  g_return_if_fail (!cancellable || G_IS_CANCELLABLE (cancellable));

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_source_tag (task, shumate_vector_renderer_fill_tile_async);

  task_data = g_new0 (TaskData, 1);
  task_data->tile = g_object_ref (tile);
  g_task_set_task_data (task, task_data, (GDestroyNotify)task_data_free);

  x = shumate_tile_get_x (tile);
  y = shumate_tile_get_y (tile);
  zoom_level = shumate_tile_get_zoom_level (tile);

  get_source_coordinates (self, &x, &y, &zoom_level);

  req = shumate_data_source_start_request (self->data_source, x, y, zoom_level, cancellable);
  task_data->req = g_object_ref (req);

  if ((data = shumate_data_source_request_get_data (req)))
    {
      begin_render (
        self,
        task,
        data,
        &SHUMATE_GRID_POSITION_INIT (x, y, zoom_level)
      );
    }

  if (shumate_data_source_request_is_completed (req))
    on_request_notify_completed (req, NULL, g_steal_pointer (&task));
  else
    {
      g_signal_connect_object (req, "notify::data", (GCallback)on_request_notify, task, 0);
      g_signal_connect_object (req, "notify::completed", (GCallback)on_request_notify_completed, g_object_ref (task), 0);
    }
}

static gboolean
shumate_vector_renderer_fill_tile_finish (ShumateMapSource  *map_source,
                                          GAsyncResult      *result,
                                          GError           **error)
{
  ShumateVectorRenderer *self = (ShumateVectorRenderer *)map_source;

  g_return_val_if_fail (SHUMATE_IS_VECTOR_RENDERER (self), FALSE);
  g_return_val_if_fail (g_task_is_valid (result, self), FALSE);

  return g_task_propagate_boolean (G_TASK (result), error);
}

void
shumate_vector_renderer_render (ShumateVectorRenderer  *self,
                                ShumateTile            *tile,
                                GBytes                 *tile_data,
                                ShumateGridPosition    *source_position,
                                GdkPaintable          **paintable,
                                GPtrArray             **symbols)
{
  SHUMATE_PROFILE_START ();

#ifdef SHUMATE_HAS_VECTOR_RENDERER
  ShumateVectorRenderScope scope;
  cairo_surface_t *surface;
  g_autoptr(GPtrArray) symbol_list = g_ptr_array_new_with_free_func ((GDestroyNotify)shumate_vector_symbol_info_unref);
  int texture_size;
  g_autofree char *profile_desc = NULL;
  g_autoptr(ShumateVectorSpriteSheet) sprites = NULL;
  g_autoptr(ShumateVectorReader) reader = NULL;

  g_assert (SHUMATE_IS_VECTOR_RENDERER (self));
  g_assert (SHUMATE_IS_TILE (tile));

  g_mutex_lock (&self->sprites_mutex);
  if (self->sprites == NULL)
    self->sprites = shumate_vector_sprite_sheet_new ();
  sprites = g_object_ref (self->sprites);
  g_mutex_unlock (&self->sprites_mutex);

  texture_size = shumate_tile_get_size (tile);
  scope.scale_factor = shumate_tile_get_scale_factor (tile);
  scope.target_size = texture_size;
  scope.tile_x = shumate_tile_get_x (tile);
  scope.tile_y = shumate_tile_get_y (tile);
  scope.zoom_level = shumate_tile_get_zoom_level (tile);
  scope.symbols = symbol_list;
  scope.sprites = sprites;
  scope.index = NULL;
  scope.index_description = self->index_description;

  if (scope.zoom_level > source_position->zoom)
    {
      float s = 1 << ((int)scope.zoom_level - source_position->zoom);
      scope.overzoom_x = (scope.tile_x - (source_position->x << ((int)scope.zoom_level - source_position->zoom))) / s;
      scope.overzoom_y = (scope.tile_y - (source_position->y << ((int)scope.zoom_level - source_position->zoom))) / s;
      scope.overzoom_scale = s;
    }
  else
    {
      scope.overzoom_x = 0;
      scope.overzoom_y = 0;
      scope.overzoom_scale = 1;
    }

  surface = cairo_image_surface_create (
    CAIRO_FORMAT_ARGB32,
    texture_size * scope.scale_factor,
    texture_size * scope.scale_factor
  );
  scope.cr = cairo_create (surface);
  cairo_scale (scope.cr, scope.scale_factor, scope.scale_factor);

  reader = shumate_vector_reader_new (tile_data);
  scope.reader = shumate_vector_reader_iterate (reader);

  if (scope.reader != NULL)
    for (scope.layer_idx = 0; scope.layer_idx < self->layers->len; scope.layer_idx ++)
      shumate_vector_layer_render ((ShumateVectorLayer *)self->layers->pdata[scope.layer_idx], &scope);

  *paintable = GDK_PAINTABLE (texture_new_for_surface (surface));
  *symbols = g_ptr_array_ref (scope.symbols);

  cairo_destroy (scope.cr);
  cairo_surface_destroy (surface);
  g_clear_object (&scope.reader);

  g_clear_pointer (&scope.index, shumate_vector_index_free);

  profile_desc = g_strdup_printf ("(%d, %d) @ %f", scope.tile_x, scope.tile_y, scope.zoom_level);
  SHUMATE_PROFILE_END (profile_desc);

#else
  g_return_if_reached ();
#endif
}

static gboolean
render_job_finish (RenderJob *job)
{
  TaskData *data = g_task_get_task_data (job->task);

  if (!g_cancellable_is_cancelled (job->cancellable))
    {
      shumate_tile_set_paintable (data->tile, job->paintable);
      shumate_tile_set_symbols (data->tile, job->symbols);
    }

  if (data->current_job == job)
    {
      data->current_job = NULL;

      if (data->completed)
        return_from_task (job->task, data->req);
    }

  render_job_unref (job);
  return G_SOURCE_REMOVE;
}

static void
thread_func (RenderJob *job)
{
  ShumateVectorRenderer *self = g_task_get_source_object (job->task);
  TaskData *data = g_task_get_task_data (job->task);

  if (!g_cancellable_is_cancelled (job->cancellable))
    {
      shumate_vector_renderer_render (
        self,
        data->tile,
        job->data,
        &job->source_position,
        &job->paintable,
        &job->symbols
      );
    }

  g_idle_add ((GSourceFunc)render_job_finish, job);
}

static void
chain_cancel (GCancellable *source,
              GCancellable *dest)
{
  g_cancellable_cancel (dest);
}

static gboolean
begin_render (ShumateVectorRenderer  *self,
              GTask                  *task,
              GBytes                 *tile_data,
              ShumateGridPosition    *source_position)
{
  g_autoptr(GError) error = NULL;
  RenderJob *job = NULL;
  TaskData *data = (TaskData *)g_task_get_task_data (task);

  if (data->current_job != NULL)
    g_cancellable_cancel (data->current_job->cancellable);

  job = g_new0 (RenderJob, 1);
  job->cancellable = g_cancellable_new ();
  job->task = g_object_ref (task);
  job->data = g_bytes_ref (tile_data);
  job->source_position = *source_position;
  data->current_job = job;

  /* If the input cancellable is cancelled, stop the render job. */
  if (g_task_get_cancellable (task) != NULL)
    {
      job->cancellable_handle = g_cancellable_connect (
        g_task_get_cancellable (task),
        G_CALLBACK (chain_cancel),
        job, NULL
      );
    }

  if (self->thread_pool == NULL)
    {
      self->thread_pool = g_thread_pool_new_full (
        (GFunc)thread_func,
        NULL,
        (GDestroyNotify)render_job_unref,
        g_get_num_processors () - 1,
        FALSE,
        &error
      );
      if (self->thread_pool == NULL)
        {
          g_critical ("Failed to create thread pool: %s", error->message);
          return FALSE;
        }
    }

  if (!g_thread_pool_push (self->thread_pool, job, &error))
    {
      g_critical ("Failed to push job to thread pool: %s", error->message);
      return FALSE;
    }

  return TRUE;
}


/**
 * shumate_style_error_quark:
 *
 * Returns: a #GQuark
 */
G_DEFINE_QUARK (shumate-style-error-quark, shumate_style_error);
