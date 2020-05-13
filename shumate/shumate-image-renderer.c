/*
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

/**
 * SECTION:shumate-image-renderer
 * @short_description: A renderer that renders tiles from binary image data
 *
 * #ShumateImageRenderer renders tiles from binary image data. The rendering
 * is performed using #GdkPixbufLoader so the set of supported image
 * formats is equal to the set of formats supported by #GdkPixbufLoader.
 */

#include "shumate-image-renderer.h"

#include <gdk/gdk.h>

typedef struct
{
  gchar *data;
  guint size;
} ShumateImageRendererPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (ShumateImageRenderer, shumate_image_renderer, SHUMATE_TYPE_RENDERER)

typedef struct _RendererData RendererData;

struct _RendererData
{
  ShumateRenderer *renderer;
  ShumateTile *tile;
  gchar *data;
  guint size;
};

static void set_data (ShumateRenderer *renderer,
    const guint8 *data,
    guint size);
static void render (ShumateRenderer *renderer,
    ShumateTile *tile);

static void
shumate_image_renderer_finalize (GObject *object)
{
  ShumateImageRenderer *image_renderer = SHUMATE_IMAGE_RENDERER (object);
  ShumateImageRendererPrivate *priv = shumate_image_renderer_get_instance_private (image_renderer);

  g_clear_pointer (&priv->data, g_free);

  G_OBJECT_CLASS (shumate_image_renderer_parent_class)->finalize (object);
}


static void
shumate_image_renderer_class_init (ShumateImageRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ShumateRendererClass *renderer_class = SHUMATE_RENDERER_CLASS (klass);

  object_class->finalize = shumate_image_renderer_finalize;

  renderer_class->set_data = set_data;
  renderer_class->render = render;
}


static void
shumate_image_renderer_init (ShumateImageRenderer *self)
{
}


/**
 * shumate_image_renderer_new:
 *
 * Constructor of #ShumateImageRenderer.
 *
 * Returns: a constructed #ShumateImageRenderer object
 */
ShumateImageRenderer *
shumate_image_renderer_new (void)
{
  return g_object_new (SHUMATE_TYPE_IMAGE_RENDERER, NULL);
}


static void
set_data (ShumateRenderer *renderer, const guint8 *data, guint size)
{
  ShumateImageRenderer *image_renderer = SHUMATE_IMAGE_RENDERER (renderer);
  ShumateImageRendererPrivate *priv = shumate_image_renderer_get_instance_private (image_renderer);

  if (priv->data)
    g_free (priv->data);

  priv->data = g_memdup (data, size);
  priv->size = size;
}


/* static gboolean */
/* image_tile_draw_cb (ClutterCanvas   *canvas, */
/*     cairo_t *cr, */
/*     gint width, */
/*     gint height, */
/*     ShumateTile *tile) */
/* { */
/*   cairo_surface_t *surface; */

/*   surface = shumate_cairo_exportable_get_surface (SHUMATE_CAIRO_EXPORTABLE (tile)); */

  /* Clear the drawing area */
/*   cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR); */
/*   cairo_paint (cr); */
/*   cairo_set_operator (cr, CAIRO_OPERATOR_OVER); */

/*   cairo_set_source_surface (cr, surface, 0, 0); */
/*   cairo_paint(cr); */

/*   return FALSE; */
/* } */


static void
image_rendered_cb (GInputStream *stream, GAsyncResult *res, RendererData *data)
{
  ShumateTile *tile = data->tile;
  gboolean error = TRUE;
  g_autoptr(GdkPixbuf) pixbuf = NULL;
  g_autoptr(GdkTexture) texture = NULL;

  pixbuf = gdk_pixbuf_new_from_stream_finish (res, NULL);
  if (!pixbuf)
    {
      g_warning ("NULL pixbuf");
      goto finish;
    }

  texture = gdk_texture_new_for_pixbuf (pixbuf);
  shumate_tile_set_texture (tile, texture);

  error = FALSE;

finish:

  g_signal_emit_by_name (tile, "render-complete", data->data, data->size, error);

  g_object_unref (data->renderer);
  g_free (data->data);
  g_slice_free (RendererData, data);

  g_object_unref (tile);
  g_object_unref (stream);
}


static void
render (ShumateRenderer *renderer, ShumateTile *tile)
{
  ShumateImageRenderer *image_renderer = SHUMATE_IMAGE_RENDERER (renderer);
  ShumateImageRendererPrivate *priv = shumate_image_renderer_get_instance_private (image_renderer);
  GInputStream *stream;

  if (!priv->data || priv->size == 0)
    {
      g_signal_emit_by_name (tile, "render-complete", priv->data, priv->size, TRUE);
      return;
    }

  RendererData *data;

  data = g_slice_new (RendererData);
  data->tile = g_object_ref (tile);
  data->renderer = g_object_ref (renderer);
  data->data = priv->data;
  data->size = priv->size;

  stream = g_memory_input_stream_new_from_data (priv->data, priv->size, NULL);
  gdk_pixbuf_new_from_stream_async (stream, NULL, (GAsyncReadyCallback)image_rendered_cb, data);
  priv->data = NULL;
}
