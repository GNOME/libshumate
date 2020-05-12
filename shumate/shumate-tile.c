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
/**
 * SECTION:shumate-tile
 * @short_description: An object that represent map tiles
 *
 * This object represents map tiles. Tiles are loaded by #ShumateMapSource.
 */

#include "shumate-tile.h"

#include "shumate-enum-types.h"
#include "shumate-private.h"
#include "shumate-marshal.h"

#include <math.h>
#include <gdk/gdk.h>
#include <gio/gio.h>
#include <cairo-gobject.h>

typedef struct
{
  guint x; /* The x position on the map (in pixels) */
  guint y; /* The y position on the map (in pixels) */
  guint size; /* The tile's width and height (only support square tiles */
  guint zoom_level; /* The tile's zoom level */

  ShumateState state; /* The tile state: loading, validation, done */
  /* The tile actor that will be displayed after shumate_tile_display_content () */
  //ClutterActor *content_actor;
  gboolean fade_in;

  GDateTime *modified_time; /* The last modified time of the cache */
  gchar *etag; /* The HTTP ETag sent by the server */
  gboolean content_displayed;
  cairo_surface_t *surface;
} ShumateTilePrivate;

G_DEFINE_TYPE_WITH_PRIVATE (ShumateTile, shumate_tile, G_TYPE_OBJECT);

enum
{
  PROP_0,
  PROP_X,
  PROP_Y,
  PROP_ZOOM_LEVEL,
  PROP_SIZE,
  PROP_STATE,
  PROP_CONTENT,
  PROP_ETAG,
  PROP_FADE_IN,
  PROP_SURFACE
};

enum
{
  /* normal signals */
  RENDER_COMPLETE,
  LAST_SIGNAL
};

static guint shumate_tile_signals[LAST_SIGNAL] = { 0, };

static void
shumate_tile_get_property (GObject *object,
    guint property_id,
    GValue *value,
    GParamSpec *pspec)
{
  ShumateTile *self = SHUMATE_TILE (object);

  switch (property_id)
    {
    case PROP_X:
      g_value_set_uint (value, shumate_tile_get_x (self));
      break;

    case PROP_Y:
      g_value_set_uint (value, shumate_tile_get_y (self));
      break;

    case PROP_ZOOM_LEVEL:
      g_value_set_uint (value, shumate_tile_get_zoom_level (self));
      break;

    case PROP_SIZE:
      g_value_set_uint (value, shumate_tile_get_size (self));
      break;

    case PROP_STATE:
      g_value_set_enum (value, shumate_tile_get_state (self));
      break;

    //case PROP_CONTENT:
    //  g_value_set_object (value, shumate_tile_get_content (self));
    //  break;

    case PROP_ETAG:
      g_value_set_string (value, shumate_tile_get_etag (self));
      break;

    case PROP_FADE_IN:
      g_value_set_boolean (value, shumate_tile_get_fade_in (self));
      break;

    case PROP_SURFACE:
      g_value_set_boxed (value, shumate_tile_get_surface (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}


static void
shumate_tile_set_property (GObject *object,
    guint property_id,
    const GValue *value,
    GParamSpec *pspec)
{
  ShumateTile *self = SHUMATE_TILE (object);

  switch (property_id)
    {
    case PROP_X:
      shumate_tile_set_x (self, g_value_get_uint (value));
      break;

    case PROP_Y:
      shumate_tile_set_y (self, g_value_get_uint (value));
      break;

    case PROP_ZOOM_LEVEL:
      shumate_tile_set_zoom_level (self, g_value_get_uint (value));
      break;

    case PROP_SIZE:
      shumate_tile_set_size (self, g_value_get_uint (value));
      break;

    case PROP_STATE:
      shumate_tile_set_state (self, g_value_get_enum (value));
      break;

    //case PROP_CONTENT:
    //  shumate_tile_set_content (self, g_value_get_object (value));
    //  break;

    case PROP_ETAG:
      shumate_tile_set_etag (self, g_value_get_string (value));
      break;

    case PROP_FADE_IN:
      shumate_tile_set_fade_in (self, g_value_get_boolean (value));
      break;

    case PROP_SURFACE:
      shumate_tile_set_surface (self, g_value_get_boxed (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}


static void
shumate_tile_dispose (GObject *object)
{
  ShumateTile *self = SHUMATE_TILE (object);
  ShumateTilePrivate *priv = shumate_tile_get_instance_private (self);

  //if (!priv->content_displayed && priv->content_actor)
  //  {
  //    clutter_actor_destroy (priv->content_actor);
  //    priv->content_actor = NULL;
  //  }

  g_clear_pointer (&priv->surface, cairo_surface_destroy);
  G_OBJECT_CLASS (shumate_tile_parent_class)->dispose (object);
}


static void
shumate_tile_finalize (GObject *object)
{
  ShumateTile *self = SHUMATE_TILE (object);
  ShumateTilePrivate *priv = shumate_tile_get_instance_private (self);

  g_date_time_unref (priv->modified_time);
  g_free (priv->etag);

  G_OBJECT_CLASS (shumate_tile_parent_class)->finalize (object);
}


static void
shumate_tile_class_init (ShumateTileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = shumate_tile_get_property;
  object_class->set_property = shumate_tile_set_property;
  object_class->dispose = shumate_tile_dispose;
  object_class->finalize = shumate_tile_finalize;

  /**
   * ShumateTile:x:
   *
   * The x position of the tile
   */
  g_object_class_install_property (object_class,
      PROP_X,
      g_param_spec_uint ("x",
          "x",
          "The X position of the tile",
          0,
          G_MAXINT,
          0,
          G_PARAM_READWRITE));

  /**
   * ShumateTile:y:
   *
   * The y position of the tile
   */
  g_object_class_install_property (object_class,
      PROP_Y,
      g_param_spec_uint ("y",
          "y",
          "The Y position of the tile",
          0,
          G_MAXINT,
          0,
          G_PARAM_READWRITE));

  /**
   * ShumateTile:zoom-level:
   *
   * The zoom level of the tile
   */
  g_object_class_install_property (object_class,
      PROP_ZOOM_LEVEL,
      g_param_spec_uint ("zoom-level",
          "Zoom Level",
          "The zoom level of the tile",
          0,
          G_MAXINT,
          0,
          G_PARAM_READWRITE));

  /**
   * ShumateTile:size:
   *
   * The size of the tile in pixels
   */
  g_object_class_install_property (object_class,
      PROP_SIZE,
      g_param_spec_uint ("size",
          "Size",
          "The size of the tile",
          0,
          G_MAXINT,
          256,
          G_PARAM_READWRITE));

  /**
   * ShumateTile:state:
   *
   * The state of the tile
   */
  g_object_class_install_property (object_class,
      PROP_STATE,
      g_param_spec_enum ("state",
          "State",
          "The state of the tile",
          SHUMATE_TYPE_STATE,
          SHUMATE_STATE_NONE,
          G_PARAM_READWRITE));

  /**
   * ShumateTile:content:
   *
   * The #ClutterActor with the specific image content.  When changing this
   * property, the new actor will be faded in.
   */
  //g_object_class_install_property (object_class,
  //    PROP_CONTENT,
  //    g_param_spec_object ("content",
  //        "Content",
  //        "The tile's content",
  //        CLUTTER_TYPE_ACTOR,
  //        G_PARAM_READWRITE));

  /**
   * ShumateTile:etag:
   *
   * The tile's ETag. This information is sent by some web servers as a mean
   * to identify if a tile has changed.  This information is saved in the cache
   * and sent in GET queries.
   */
  g_object_class_install_property (object_class,
      PROP_ETAG,
      g_param_spec_string ("etag",
          "Entity Tag",
          "The entity tag of the tile",
          NULL,
          G_PARAM_READWRITE));

  /**
   * ShumateTile:fade-in:
   *
   * Specifies whether the tile should fade in when loading
   */
  g_object_class_install_property (object_class,
      PROP_FADE_IN,
      g_param_spec_boolean ("fade-in",
          "Fade In",
          "Tile should fade in",
          FALSE,
          G_PARAM_READWRITE));

  /**
   * ShumateTile:surface:
   *
   * The Cairo surface backing the tile
   */
  g_object_class_install_property (object_class,
      PROP_SURFACE,
      g_param_spec_boxed ("surface",
          "Surface",
          "Cairo surface representaion",
          CAIRO_GOBJECT_TYPE_SURFACE,
          G_PARAM_READWRITE));

  /**
   * ShumateTile::render-complete:
   * @self: a #ShumateTile
   * @data: the result of the rendering
   * @size: size of data
   * @error: TRUE if there was an error during rendering
   *
   * The #ShumateTile::render-complete signal is emitted when rendering of the tile is
   * completed by the renderer.
   */
  shumate_tile_signals[RENDER_COMPLETE] =
    g_signal_new ("render-complete",
        G_OBJECT_CLASS_TYPE (object_class),
        G_SIGNAL_RUN_LAST,
        0,
        NULL,
        NULL,
        _shumate_marshal_VOID__POINTER_UINT_BOOLEAN,
        G_TYPE_NONE,
        3,
        G_TYPE_POINTER, G_TYPE_UINT, G_TYPE_BOOLEAN);
}


static void
shumate_tile_init (ShumateTile *self)
{
  ShumateTilePrivate *priv = shumate_tile_get_instance_private (self);

  priv->state = SHUMATE_STATE_NONE;
  priv->x = 0;
  priv->y = 0;
  priv->zoom_level = 0;
  priv->size = 0;
  priv->modified_time = NULL;
  priv->etag = NULL;
  priv->fade_in = FALSE;
  priv->content_displayed = FALSE;

  //priv->content_actor = NULL;
}

/**
 * shumate_tile_new:
 *
 * Creates an instance of #ShumateTile.
 *
 * Returns: a new #ShumateTile
 */
ShumateTile *
shumate_tile_new (void)
{
  return g_object_new (SHUMATE_TYPE_TILE, NULL);
}


/**
 * shumate_tile_get_x:
 * @self: the #ShumateTile
 *
 * Gets the tile's x position.
 *
 * Returns: the tile's x position
 */
guint
shumate_tile_get_x (ShumateTile *self)
{
  ShumateTilePrivate *priv = shumate_tile_get_instance_private (self);

  g_return_val_if_fail (SHUMATE_TILE (self), 0);

  return priv->x;
}


/**
 * shumate_tile_get_y:
 * @self: the #ShumateTile
 *
 * Gets the tile's y position.
 *
 * Returns: the tile's y position
 */
guint
shumate_tile_get_y (ShumateTile *self)
{
  ShumateTilePrivate *priv = shumate_tile_get_instance_private (self);

  g_return_val_if_fail (SHUMATE_TILE (self), 0);

  return priv->y;
}


/**
 * shumate_tile_get_zoom_level:
 * @self: the #ShumateTile
 *
 * Gets the tile's zoom level.
 *
 * Returns: the tile's zoom level
 */
guint
shumate_tile_get_zoom_level (ShumateTile *self)
{
  ShumateTilePrivate *priv = shumate_tile_get_instance_private (self);

  g_return_val_if_fail (SHUMATE_TILE (self), 0);

  return priv->zoom_level;
}


/**
 * shumate_tile_get_size:
 * @self: the #ShumateTile
 *
 * Gets the tile's size.
 *
 * Returns: the tile's size in pixels
 */
guint
shumate_tile_get_size (ShumateTile *self)
{
  ShumateTilePrivate *priv = shumate_tile_get_instance_private (self);

  g_return_val_if_fail (SHUMATE_TILE (self), 0);

  return priv->size;
}


/**
 * shumate_tile_get_state:
 * @self: the #ShumateTile
 *
 * Gets the current state of tile loading.
 *
 * Returns: the tile's #ShumateState
 */
ShumateState
shumate_tile_get_state (ShumateTile *self)
{
  ShumateTilePrivate *priv = shumate_tile_get_instance_private (self);

  g_return_val_if_fail (SHUMATE_TILE (self), SHUMATE_STATE_NONE);

  return priv->state;
}


/**
 * shumate_tile_set_x:
 * @self: the #ShumateTile
 * @x: the position
 *
 * Sets the tile's x position
 */
void
shumate_tile_set_x (ShumateTile *self,
    guint x)
{
  ShumateTilePrivate *priv = shumate_tile_get_instance_private (self);

  g_return_if_fail (SHUMATE_TILE (self));

  priv->x = x;

  g_object_notify (G_OBJECT (self), "x");
}


/**
 * shumate_tile_set_y:
 * @self: the #ShumateTile
 * @y: the position
 *
 * Sets the tile's y position
 */
void
shumate_tile_set_y (ShumateTile *self,
    guint y)
{
  ShumateTilePrivate *priv = shumate_tile_get_instance_private (self);

  g_return_if_fail (SHUMATE_TILE (self));

  priv->y = y;

  g_object_notify (G_OBJECT (self), "y");
}


/**
 * shumate_tile_set_zoom_level:
 * @self: the #ShumateTile
 * @zoom_level: the zoom level
 *
 * Sets the tile's zoom level
 */
void
shumate_tile_set_zoom_level (ShumateTile *self,
    guint zoom_level)
{
  ShumateTilePrivate *priv = shumate_tile_get_instance_private (self);

  g_return_if_fail (SHUMATE_TILE (self));

  priv->zoom_level = zoom_level;

  g_object_notify (G_OBJECT (self), "zoom-level");
}


/**
 * shumate_tile_set_size:
 * @self: the #ShumateTile
 * @size: the size in pixels
 *
 * Sets the tile's size
 */
void
shumate_tile_set_size (ShumateTile *self,
    guint size)
{
  ShumateTilePrivate *priv = shumate_tile_get_instance_private (self);

  g_return_if_fail (SHUMATE_TILE (self));

  priv->size = size;

  g_object_notify (G_OBJECT (self), "size");
}


/**
 * shumate_tile_set_state:
 * @self: the #ShumateTile
 * @state: a #ShumateState
 *
 * Sets the tile's #ShumateState
 */
void
shumate_tile_set_state (ShumateTile *self,
    ShumateState state)
{
  ShumateTilePrivate *priv = shumate_tile_get_instance_private (self);

  g_return_if_fail (SHUMATE_TILE (self));

  if (state == priv->state)
    return;

  priv->state = state;
  g_object_notify (G_OBJECT (self), "state");
}


/**
 * shumate_tile_new_full:
 * @x: the x position
 * @y: the y position
 * @size: the size in pixels
 * @zoom_level: the zoom level
 *
 * Creates an instance of #ShumateTile.
 *
 * Returns: a #ShumateTile
 */
ShumateTile *
shumate_tile_new_full (guint x,
    guint y,
    guint size,
    guint zoom_level)
{
  return g_object_new (SHUMATE_TYPE_TILE,
      "x", x,
      "y", y,
      "zoom-level", zoom_level,
      "size", size,
      NULL);
}


/**
 * shumate_tile_get_modified_time:
 * @self: the #ShumateTile
 *
 * Gets the tile's last modified time.
 *
 * Returns: (transfer none): the tile's last modified time
 */
GDateTime *
shumate_tile_get_modified_time (ShumateTile *self)
{
  ShumateTilePrivate *priv = shumate_tile_get_instance_private (self);

  g_return_val_if_fail (SHUMATE_TILE (self), NULL);

  return priv->modified_time;
}


/**
 * shumate_tile_set_modified_time:
 * @self: the #ShumateTile
 * @modified_time: a #GDateTime, the value will be copied
 *
 * Sets the tile's modified time
 */
void
shumate_tile_set_modified_time (ShumateTile *self,
    GDateTime *modified_time)
{
  ShumateTilePrivate *priv = shumate_tile_get_instance_private (self);

  g_return_if_fail (SHUMATE_TILE (self));
  g_return_if_fail (modified_time != NULL);

  g_date_time_unref (priv->modified_time);
  priv->modified_time = g_date_time_ref (modified_time);
}


/**
 * shumate_tile_get_etag:
 * @self: the #ShumateTile
 *
 * Gets the tile's ETag.
 *
 * Returns: the tile's ETag
 */
const gchar *
shumate_tile_get_etag (ShumateTile *self)
{
  ShumateTilePrivate *priv = shumate_tile_get_instance_private (self);

  g_return_val_if_fail (SHUMATE_TILE (self), "");

  return priv->etag;
}


/**
 * shumate_tile_set_etag:
 * @self: the #ShumateTile
 * @etag: the tile's ETag as sent by the server
 *
 * Sets the tile's ETag
 */
void
shumate_tile_set_etag (ShumateTile *self,
    const gchar *etag)
{
  ShumateTilePrivate *priv = shumate_tile_get_instance_private (self);

  g_return_if_fail (SHUMATE_TILE (self));

  g_free (priv->etag);
  priv->etag = g_strdup (etag);
  g_object_notify (G_OBJECT (self), "etag");
}


/**
 * shumate_tile_set_content:
 * @self: the #ShumateTile
 * @actor: the new content
 *
 * Sets the tile's content. To also disppay the tile, you have to call
 * shumate_tile_display_content() in addition.
 */
/*
void
shumate_tile_set_content (ShumateTile *self,
    ClutterActor *actor)
{
  g_return_if_fail (SHUMATE_TILE (self));
  g_return_if_fail (CLUTTER_ACTOR (actor));

  ShumateTilePrivate *priv = self->priv;

  if (!priv->content_displayed && priv->content_actor)
    clutter_actor_destroy (priv->content_actor);

  priv->content_actor = g_object_ref_sink (actor);
  priv->content_displayed = FALSE;

  g_object_notify (G_OBJECT (self), "content");
}
*/


/*
static void
fade_in_completed (ClutterActor *actor,
    const gchar *transition_name,
    gboolean is_finished,
    ShumateTile *self)
{
  if (clutter_actor_get_n_children (CLUTTER_ACTOR (self)) > 1)
    clutter_actor_destroy (clutter_actor_get_first_child (CLUTTER_ACTOR (self)));

  g_signal_handlers_disconnect_by_func (actor, fade_in_completed, self);
}
 */


/**
 * shumate_tile_display_content:
 * @self: the #ShumateTile
 *
 * Displays the tile's content.
 */
void
shumate_tile_display_content (ShumateTile *self)
{
  //ShumateTilePrivate *priv = shumate_tile_get_instance_private (self);

  g_return_if_fail (SHUMATE_TILE (self));

  /*
  if (!priv->content_actor || priv->content_displayed)
    return;

  clutter_actor_add_child (CLUTTER_ACTOR (self), priv->content_actor);
  g_object_unref (priv->content_actor);
  priv->content_displayed = TRUE;

  clutter_actor_set_opacity (priv->content_actor, 0);
  clutter_actor_save_easing_state (priv->content_actor);
  if (priv->fade_in)
    {
      clutter_actor_set_easing_mode (priv->content_actor, CLUTTER_EASE_IN_CUBIC);
      clutter_actor_set_easing_duration (priv->content_actor, 500);
    }
  else
    {
      clutter_actor_set_easing_mode (priv->content_actor, CLUTTER_LINEAR);
      clutter_actor_set_easing_duration (priv->content_actor, 150);
    }
  clutter_actor_set_opacity (priv->content_actor, 255);
  clutter_actor_restore_easing_state (priv->content_actor);

  g_signal_connect (priv->content_actor, "transition-stopped::opacity", G_CALLBACK (fade_in_completed), self);
   */
}


/**
 * shumate_tile_get_content:
 * @self: the #ShumateTile
 *
 * Gets the tile's content actor.
 *
 * Returns: (transfer none): the tile's content, this actor will change each time the tile's content changes.
 * You should not unref this content, it is owned by the tile.
 */
/*
ClutterActor *
shumate_tile_get_content (ShumateTile *self)
{
  g_return_val_if_fail (SHUMATE_TILE (self), NULL);

  return self->priv->content_actor;
}
 */


/**
 * shumate_tile_get_fade_in:
 * @self: the #ShumateTile
 *
 * Checks whether the tile should fade in.
 *
 * Returns: the return value determines whether the tile should fade in when loading.
 */
gboolean
shumate_tile_get_fade_in (ShumateTile *self)
{
  ShumateTilePrivate *priv = shumate_tile_get_instance_private (self);

  g_return_val_if_fail (SHUMATE_TILE (self), FALSE);

  return priv->fade_in;
}


/**
 * shumate_tile_set_fade_in:
 * @self: the #ShumateTile
 * @fade_in: determines whether the tile should fade in when loading
 *
 * Sets the flag determining whether the tile should fade in when loading
 */
void
shumate_tile_set_fade_in (ShumateTile *self,
    gboolean fade_in)
{
  ShumateTilePrivate *priv = shumate_tile_get_instance_private (self);

  g_return_if_fail (SHUMATE_TILE (self));

  priv->fade_in = fade_in;

  g_object_notify (G_OBJECT (self), "surface");
}

/**
 * shumate_tile_get_surface:
 * @self: the #ShumateTile
 *
 * Returns: (transfer none): A #cairo_surface_t
 */
cairo_surface_t *
shumate_tile_get_surface (ShumateTile *self)
{
  ShumateTilePrivate *priv = shumate_tile_get_instance_private (self);

  g_return_val_if_fail (SHUMATE_TILE (self), NULL);

  return priv->surface;
}

/**
 * shumate_tile_set_surface:
 * @self: the #ShumateTile
 * @surface: a #cairo_surface_t
 *
 */
void
shumate_tile_set_surface (ShumateTile *self,
    cairo_surface_t *surface)
{
  ShumateTilePrivate *priv = shumate_tile_get_instance_private (self);

  g_return_if_fail (SHUMATE_TILE (self));

  if (priv->surface == surface)
    return;

  g_clear_pointer (&priv->surface, cairo_surface_destroy);
  if (surface)
    priv->surface = cairo_surface_reference (surface);

  g_object_notify (G_OBJECT (self), "surface");
}
