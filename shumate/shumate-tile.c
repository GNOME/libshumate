/*
 * Copyright (C) 2008-2009 Pierre-Luc Beaudoin <pierre-luc@pierlux.com>
 * Copyright (C) 2010-2013 Jiri Techet <techet@gmail.com>
 * Copyright (C) 2019 Marcus Lundblad <ml@update.uu.se>
 * Copyright (C) 2020 Collabora, Ltd. (https://www.collabora.com)
 * Copyright (C) 2020 Corentin Noël <corentin.noel@collabora.com>
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
 * ShumateTile:
 *
 * An object that represents map tiles. Tiles are loaded by a [class@MapSource].
 */

#include "shumate-tile.h"

#include "shumate-enum-types.h"
#include "shumate-marshal.h"

#include <math.h>
#include <gdk/gdk.h>
#include <gio/gio.h>

typedef struct
{
  guint x; /* The x position on the map (in pixels) */
  guint y; /* The y position on the map (in pixels) */
  guint size; /* The tile's width and height (only support square tiles */
  guint zoom_level; /* The tile's zoom level */

  ShumateState state; /* The tile state: loading, validation, done */
  gboolean fade_in;

  GdkTexture *texture;
  GPtrArray *symbols;
} ShumateTilePrivate;

G_DEFINE_TYPE_WITH_PRIVATE (ShumateTile, shumate_tile, GTK_TYPE_WIDGET);

enum
{
  PROP_X = 1,
  PROP_Y,
  PROP_ZOOM_LEVEL,
  PROP_SIZE,
  PROP_STATE,
  PROP_FADE_IN,
  PROP_TEXTURE,
  N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static void
shumate_tile_snapshot (GtkWidget   *widget,
                       GtkSnapshot *snapshot)
{
  ShumateTile *self = SHUMATE_TILE (widget);
  ShumateTilePrivate *priv = shumate_tile_get_instance_private (self);
  GdkTexture *texture = priv->texture;

  if (texture)
    {
      gtk_snapshot_append_texture (snapshot,
                                   texture,
                                   &GRAPHENE_RECT_INIT(
                                     0, 0,
                                     gtk_widget_get_width (widget),
                                     gtk_widget_get_height (widget)
                                   ));
    }
}

static GtkSizeRequestMode 
shumate_tile_get_request_mode (GtkWidget *widget)
{
  return GTK_SIZE_REQUEST_CONSTANT_SIZE;
}

static void
shumate_tile_measure (GtkWidget      *widget,
                      GtkOrientation  orientation,
                      int             for_size,
                      int            *minimum,
                      int            *natural,
                      int            *minimum_baseline,
                      int            *natural_baseline)
{
  ShumateTile *self = SHUMATE_TILE (widget);
  ShumateTilePrivate *priv = shumate_tile_get_instance_private (self);

  if (minimum)
    *minimum = 0;

  if (natural)
    *natural = priv->size;
}

static void
shumate_tile_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
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

    case PROP_FADE_IN:
      g_value_set_boolean (value, shumate_tile_get_fade_in (self));
      break;

    case PROP_TEXTURE:
      g_value_set_object (value, shumate_tile_get_texture (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}


static void
shumate_tile_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
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

    case PROP_FADE_IN:
      shumate_tile_set_fade_in (self, g_value_get_boolean (value));
      break;

    case PROP_TEXTURE:
      shumate_tile_set_texture (self, g_value_get_object (value));
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

  g_clear_object (&priv->texture);
  g_clear_pointer (&priv->symbols, g_ptr_array_unref);

  G_OBJECT_CLASS (shumate_tile_parent_class)->dispose (object);
}


static void
shumate_tile_class_init (ShumateTileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = shumate_tile_get_property;
  object_class->set_property = shumate_tile_set_property;
  object_class->dispose = shumate_tile_dispose;

  widget_class->snapshot = shumate_tile_snapshot;
  widget_class->measure = shumate_tile_measure;
  widget_class->get_request_mode = shumate_tile_get_request_mode;
  
  /**
   * ShumateTile:x:
   *
   * The x position of the tile
   */
  obj_properties[PROP_X] =
    g_param_spec_uint ("x",
                       "x",
                       "The X position of the tile",
                       0,
                       G_MAXUINT,
                       0,
                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * ShumateTile:y:
   *
   * The y position of the tile
   */
  obj_properties[PROP_Y] =
    g_param_spec_uint ("y",
                       "y",
                       "The Y position of the tile",
                       0,
                       G_MAXUINT,
                       0,
                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * ShumateTile:zoom-level:
   *
   * The zoom level of the tile
   */
  obj_properties[PROP_ZOOM_LEVEL] =
    g_param_spec_uint ("zoom-level",
                       "Zoom Level",
                       "The zoom level of the tile",
                       0,
                       G_MAXUINT,
                       0,
                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * ShumateTile:size:
   *
   * The size of the tile in pixels
   */
  obj_properties[PROP_SIZE] =
    g_param_spec_uint ("size",
                       "Size",
                       "The size of the tile",
                       0,
                       G_MAXUINT,
                       256,
                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * ShumateTile:state:
   *
   * The state of the tile
   */
  obj_properties[PROP_STATE] =
    g_param_spec_enum ("state",
                       "State",
                       "The state of the tile",
                       SHUMATE_TYPE_STATE,
                       SHUMATE_STATE_NONE,
                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * ShumateTile:fade-in:
   *
   * Specifies whether the tile should fade in when loading
   */
  obj_properties[PROP_FADE_IN] =
    g_param_spec_boolean ("fade-in",
                          "Fade In",
                          "Tile should fade in",
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * ShumateTile:texture:
   *
   * The #GdkTexture backing the tile
   */
  obj_properties[PROP_TEXTURE] =
    g_param_spec_object ("texture",
                         "Texture",
                         "Gdk Texture representation",
                         GDK_TYPE_TEXTURE,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class,
                                     N_PROPERTIES,
                                     obj_properties);

  gtk_widget_class_set_css_name (widget_class, "map-tile");
}


static void
shumate_tile_init (ShumateTile *self)
{
  ShumateTilePrivate *priv = shumate_tile_get_instance_private (self);

  priv->state = SHUMATE_STATE_NONE;
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
 * shumate_tile_set_x:
 * @self: the #ShumateTile
 * @x: the position
 *
 * Sets the tile's x position
 */
void
shumate_tile_set_x (ShumateTile *self,
                    guint        x)
{
  ShumateTilePrivate *priv = shumate_tile_get_instance_private (self);

  g_return_if_fail (SHUMATE_TILE (self));

  if (priv->x == x)
    return;

  priv->x = x;
  g_object_notify_by_pspec (G_OBJECT (self), obj_properties[PROP_X]);
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
 * shumate_tile_set_y:
 * @self: the #ShumateTile
 * @y: the position
 *
 * Sets the tile's y position
 */
void
shumate_tile_set_y (ShumateTile *self,
                    guint        y)
{
  ShumateTilePrivate *priv = shumate_tile_get_instance_private (self);

  g_return_if_fail (SHUMATE_TILE (self));

  if (priv->y == y)
    return;

  priv->y = y;
  g_object_notify_by_pspec (G_OBJECT (self), obj_properties[PROP_Y]);
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
 * shumate_tile_set_zoom_level:
 * @self: the #ShumateTile
 * @zoom_level: the zoom level
 *
 * Sets the tile's zoom level
 */
void
shumate_tile_set_zoom_level (ShumateTile *self,
                             guint        zoom_level)
{
  ShumateTilePrivate *priv = shumate_tile_get_instance_private (self);

  g_return_if_fail (SHUMATE_TILE (self));

  if (priv->zoom_level == zoom_level)
    return;

  priv->zoom_level = zoom_level;
  g_object_notify_by_pspec (G_OBJECT (self), obj_properties[PROP_ZOOM_LEVEL]);
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
 * shumate_tile_set_size:
 * @self: the #ShumateTile
 * @size: the size in pixels
 *
 * Sets the tile's size
 */
void
shumate_tile_set_size (ShumateTile *self,
                       guint        size)
{
  ShumateTilePrivate *priv = shumate_tile_get_instance_private (self);

  g_return_if_fail (SHUMATE_TILE (self));

  if (priv->size == size)
    return;

  priv->size = size;
  g_object_notify_by_pspec (G_OBJECT (self), obj_properties[PROP_SIZE]);
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
  g_object_notify_by_pspec (G_OBJECT (self), obj_properties[PROP_STATE]);
}


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
                          gboolean     fade_in)
{
  ShumateTilePrivate *priv = shumate_tile_get_instance_private (self);

  g_return_if_fail (SHUMATE_TILE (self));

  if (priv->fade_in == fade_in)
    return;

  priv->fade_in = fade_in;
  g_object_notify_by_pspec (G_OBJECT (self), obj_properties[PROP_FADE_IN]);
}

/**
 * shumate_tile_get_texture:
 * @self: the #ShumateTile
 *
 * Get the #GdkTexture representing this tile.
 *
 * Returns: (transfer none) (nullable): A #GdkTexture
 */
GdkTexture *
shumate_tile_get_texture (ShumateTile *self)
{
  ShumateTilePrivate *priv = shumate_tile_get_instance_private (self);

  g_return_val_if_fail (SHUMATE_TILE (self), NULL);

  return priv->texture;
}

/**
 * shumate_tile_set_texture:
 * @self: the #ShumateTile
 * @texture: a #GdkTexture
 *
 * Sets the #GdkTexture representing this tile.
 */
void
shumate_tile_set_texture (ShumateTile *self,
                          GdkTexture  *texture)
{
  ShumateTilePrivate *priv = shumate_tile_get_instance_private (self);

  g_return_if_fail (SHUMATE_TILE (self));

  if (g_set_object (&priv->texture, texture))
    {
      g_object_notify_by_pspec (G_OBJECT (self), obj_properties[PROP_TEXTURE]);
      gtk_widget_queue_draw (GTK_WIDGET (self));
    }
}


void
shumate_tile_set_symbols (ShumateTile *self, GPtrArray *symbols)
{
  ShumateTilePrivate *priv = shumate_tile_get_instance_private (self);

  g_return_if_fail (SHUMATE_IS_TILE (self));

  g_clear_pointer (&priv->symbols, g_ptr_array_unref);
  if (symbols != NULL)
    priv->symbols = g_ptr_array_ref (symbols);
}


GPtrArray *
shumate_tile_get_symbols (ShumateTile *self)
{
  ShumateTilePrivate *priv = shumate_tile_get_instance_private (self);

  g_return_val_if_fail (SHUMATE_IS_TILE (self), NULL);

  return priv->symbols;
}
