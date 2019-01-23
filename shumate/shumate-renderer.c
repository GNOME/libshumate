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
 * SECTION:shumate-renderer
 * @short_description: A base class of renderers
 *
 * A renderer is used to render tiles textures. A tile is rendered based on
 * the provided data - this can be arbitrary data the given renderer understands
 * (e.g. raw bitmap data, vector xml map representation and so on).
 */

#include "shumate-renderer.h"

G_DEFINE_TYPE (ShumateRenderer, shumate_renderer, G_TYPE_INITIALLY_UNOWNED)

static void
shumate_renderer_dispose (GObject *object)
{
  G_OBJECT_CLASS (shumate_renderer_parent_class)->dispose (object);
}


static void
shumate_renderer_finalize (GObject *object)
{
  G_OBJECT_CLASS (shumate_renderer_parent_class)->finalize (object);
}


static void
shumate_renderer_class_init (ShumateRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = shumate_renderer_finalize;
  object_class->dispose = shumate_renderer_dispose;

  klass->set_data = NULL;
  klass->render = NULL;
}


/**
 * shumate_renderer_set_data:
 * @renderer: a #ShumateRenderer
 * @data: data used for tile rendering
 * @size: size of the data in bytes
 *
 * Sets the data which is used to render tiles by the renderer.
 */
void
shumate_renderer_set_data (ShumateRenderer *renderer,
    const gchar *data,
    guint size)
{
  g_return_if_fail (SHUMATE_IS_RENDERER (renderer));

  SHUMATE_RENDERER_GET_CLASS (renderer)->set_data (renderer, data, size);
}


/**
 * shumate_renderer_render:
 * @renderer: a #ShumateRenderer
 * @tile: the tile to render
 *
 * Renders the texture for the provided tile and calls shumate_tile_set_content()
 * to set the content of the tile. When the rendering is finished, the renderer
 * emits the #ShumateTile::render-complete signal. The tile has to be displayed manually by
 * calling shumate_tile_display_content().
 */
void
shumate_renderer_render (ShumateRenderer *renderer,
    ShumateTile *tile)
{
  g_return_if_fail (SHUMATE_IS_RENDERER (renderer));

  SHUMATE_RENDERER_GET_CLASS (renderer)->render (renderer, tile);
}


static void
shumate_renderer_init (ShumateRenderer *self)
{
}
