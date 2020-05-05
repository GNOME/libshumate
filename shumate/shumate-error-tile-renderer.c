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
 * SECTION:shumate-error-tile-renderer
 * @short_description: A renderer that renders error tiles independently of input data
 *
 * #ShumateErrorTileRenderer always renders error tiles (tiles that indicate that the real tile could
 * not be loaded) no matter what input data is used.
 */

#include "shumate-error-tile-renderer.h"

#include "shumate-cairo-importable.h"

#include <gdk/gdk.h>

typedef struct
{
  //ClutterContent *error_canvas;
  guint tile_size;
} ShumateErrorTileRendererPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (ShumateErrorTileRenderer, shumate_error_tile_renderer, SHUMATE_TYPE_RENDERER)

enum
{
  PROP_0,
  PROP_TILE_SIZE
};


static void set_data (ShumateRenderer *renderer,
    const guint8 *data,
    guint size);
static void render (ShumateRenderer *renderer,
    ShumateTile *tile);


static void
shumate_error_tile_renderer_get_property (GObject *object,
    guint property_id,
    GValue *value,
    GParamSpec *pspec)
{
  ShumateErrorTileRenderer *renderer = SHUMATE_ERROR_TILE_RENDERER (object);

  switch (property_id)
    {
    case PROP_TILE_SIZE:
      g_value_set_uint (value, shumate_error_tile_renderer_get_tile_size (renderer));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}


static void
shumate_error_tile_renderer_set_property (GObject *object,
    guint property_id,
    const GValue *value,
    GParamSpec *pspec)
{
  ShumateErrorTileRenderer *renderer = SHUMATE_ERROR_TILE_RENDERER (object);

  switch (property_id)
    {
    case PROP_TILE_SIZE:
      shumate_error_tile_renderer_set_tile_size (renderer, g_value_get_uint (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
shumate_error_tile_renderer_class_init (ShumateErrorTileRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ShumateRendererClass *renderer_class = SHUMATE_RENDERER_CLASS (klass);

  object_class->get_property = shumate_error_tile_renderer_get_property;
  object_class->set_property = shumate_error_tile_renderer_set_property;

  /**
   * ShumateErrorTileRenderer:tile-size:
   *
   * The size of the rendered tile.
   */
  g_object_class_install_property (object_class,
      PROP_TILE_SIZE,
      g_param_spec_uint ("tile-size",
          "Tile Size",
          "The size of the rendered tile",
          0,
          G_MAXINT,
          256,
          G_PARAM_READWRITE));

  renderer_class->set_data = set_data;
  renderer_class->render = render;
}


static void
shumate_error_tile_renderer_init (ShumateErrorTileRenderer *self)
{
}


/**
 * shumate_error_tile_renderer_new:
 * @tile_size: the size of the rendered error tile
 *
 * Constructor of a #ShumateErrorTileRenderer.
 *
 * Returns: a constructed #ShumateErrorTileRenderer
 */
ShumateErrorTileRenderer *
shumate_error_tile_renderer_new (guint tile_size)
{
  return g_object_new (SHUMATE_TYPE_ERROR_TILE_RENDERER,
      "tile-size", tile_size,
      NULL);
}


static void
set_data (ShumateRenderer *renderer, const guint8 *data, guint size)
{
  /* always render the error tile no matter what data is set */
}


/* static gboolean */
/* redraw_tile (ClutterCanvas *canvas, */
/*     cairo_t *cr, */
/*     gint w, */
/*     gint h, */
/*     ShumateTile *tile) */
/* { */
/*   cairo_pattern_t *pat; */
/*   gint size = w; */

/*   shumate_cairo_importable_set_surface (SHUMATE_CAIRO_IMPORTABLE (tile), cairo_get_target (cr)); */

  /* draw a linear gray to white pattern */
/*   pat = cairo_pattern_create_linear (size / 2.0, 0.0, size, size / 2.0); */
/*   cairo_pattern_add_color_stop_rgb (pat, 0, 0.686, 0.686, 0.686); */
/*   cairo_pattern_add_color_stop_rgb (pat, 1, 0.925, 0.925, 0.925); */
/*   cairo_set_source (cr, pat); */
/*   cairo_rectangle (cr, 0, 0, size, size); */
/*   cairo_fill (cr); */

/*   cairo_pattern_destroy (pat); */

  /* draw the red cross */
/*   cairo_set_source_rgb (cr, 0.424, 0.078, 0.078); */
/*   cairo_set_line_width (cr, 14.0); */
/*   cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND); */
/*   cairo_move_to (cr, 24, 24); */
/*   cairo_line_to (cr, 50, 50); */
/*   cairo_move_to (cr, 50, 24); */
/*   cairo_line_to (cr, 24, 50); */
/*   cairo_stroke (cr); */

/*   return TRUE; */
/* } */


static void
render (ShumateRenderer *renderer, ShumateTile *tile)
{
  ShumateErrorTileRenderer *error_renderer = SHUMATE_ERROR_TILE_RENDERER (renderer);

  g_return_if_fail (SHUMATE_IS_ERROR_TILE_RENDERER (renderer));
  g_return_if_fail (SHUMATE_IS_TILE (tile));

  //ClutterActor *actor;
  gpointer data = NULL;
  guint size = 0;
  gboolean error = FALSE;

  if (shumate_tile_get_state (tile) == SHUMATE_STATE_LOADED)
    {
      /* cache is just validating tile - don't generate error tile in this case - instead use what we have */
      g_signal_emit_by_name (tile, "render-complete", data, size, error);
      return;
    }

  size = shumate_error_tile_renderer_get_tile_size (error_renderer);

  /* if (!priv->error_canvas) */
  /*   { */
  /*     priv->error_canvas = clutter_canvas_new (); */
  /*     clutter_canvas_set_size (CLUTTER_CANVAS (priv->error_canvas), size, size); */
  /*     g_signal_connect (priv->error_canvas, "draw", G_CALLBACK (redraw_tile), tile); */
  /*     clutter_content_invalidate (priv->error_canvas); */
  /*   } */

  /* actor = clutter_actor_new (); */
  /* clutter_actor_set_size (actor, size, size); */
  /* clutter_actor_set_content (actor, priv->error_canvas); */
  /* has to be set for proper opacity */
  /* clutter_actor_set_offscreen_redirect (actor, CLUTTER_OFFSCREEN_REDIRECT_AUTOMATIC_FOR_OPACITY); */

  /* shumate_tile_set_content (tile, actor); */
  g_signal_emit_by_name (tile, "render-complete", data, size, error);
}


/**
 * shumate_error_tile_renderer_set_tile_size:
 * @renderer: a #ShumateErrorTileRenderer
 * @size: the size of the rendered error tiles
 *
 * Sets the size of the rendered error tile.
 */
void
shumate_error_tile_renderer_set_tile_size (ShumateErrorTileRenderer *renderer,
    guint size)
{
  ShumateErrorTileRenderer *error_renderer = SHUMATE_ERROR_TILE_RENDERER (renderer);
  ShumateErrorTileRendererPrivate *priv = shumate_error_tile_renderer_get_instance_private (error_renderer);

  g_return_if_fail (SHUMATE_IS_ERROR_TILE_RENDERER (renderer));

  priv->tile_size = size;

  g_object_notify (G_OBJECT (renderer), "tile-size");
}


/**
 * shumate_error_tile_renderer_get_tile_size:
 * @renderer: a #ShumateErrorTileRenderer
 *
 * Gets the size of the rendered error tiles.
 *
 * Returns: the size of the rendered error tiles
 */
guint
shumate_error_tile_renderer_get_tile_size (ShumateErrorTileRenderer *renderer)
{
  ShumateErrorTileRenderer *error_renderer = SHUMATE_ERROR_TILE_RENDERER (renderer);
  ShumateErrorTileRendererPrivate *priv = shumate_error_tile_renderer_get_instance_private (error_renderer);

  g_return_val_if_fail (SHUMATE_IS_ERROR_TILE_RENDERER (renderer), 0);

  return priv->tile_size;
}
