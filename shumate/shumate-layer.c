/*
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

/**
 * ShumateLayer:
 *
 * Every layer (overlay that moves together with the map) has to inherit this
 * class and implement its virtual methods.
 *
 * You can use the same layer to display many types of maps.  In Shumate they
 * are called map sources.  You can change the [property@MapLayer:map-source]
 * property at any time to replace the current displayed map.
 */

#include "shumate-layer.h"

enum
{
  PROP_VIEWPORT = 1,
  N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

typedef struct {
  ShumateViewport *viewport;
} ShumateLayerPrivate;

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (ShumateLayer, shumate_layer, GTK_TYPE_WIDGET)

static void
shumate_layer_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  ShumateLayer *self = SHUMATE_LAYER (object);
  ShumateLayerPrivate *priv = shumate_layer_get_instance_private (self);

  switch (property_id)
    {
    case PROP_VIEWPORT:
      priv->viewport = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
shumate_layer_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  ShumateLayer *self = SHUMATE_LAYER (object);
  ShumateLayerPrivate *priv = shumate_layer_get_instance_private (self);

  switch (property_id)
    {
    case PROP_VIEWPORT:
      g_value_set_object (value, priv->viewport);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
shumate_layer_dispose (GObject *object)
{
  ShumateLayerPrivate *priv = shumate_layer_get_instance_private (SHUMATE_LAYER (object));

  g_clear_object (&priv->viewport);

  G_OBJECT_CLASS (shumate_layer_parent_class)->dispose (object);
}

static void
shumate_layer_constructed (GObject *object)
{
  ShumateLayerPrivate *priv = shumate_layer_get_instance_private (SHUMATE_LAYER (object));

  if (!priv->viewport)
    priv->viewport = shumate_viewport_new ();

  G_OBJECT_CLASS (shumate_layer_parent_class)->constructed (object);
}

static gboolean
shumate_layer_contains (GtkWidget *widget,
                        double     x,
                        double     y)
{
  /* This allows mouse events on the empty space between markers in this layer
   * to fall through to lower layers. */
  return FALSE;
}

static void
shumate_layer_class_init (ShumateLayerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->set_property = shumate_layer_set_property;
  object_class->get_property = shumate_layer_get_property;
  object_class->dispose = shumate_layer_dispose;
  object_class->constructed = shumate_layer_constructed;

  widget_class->contains = shumate_layer_contains;

  obj_properties[PROP_VIEWPORT] =
    g_param_spec_object ("viewport",
                         "Viewport",
                         "The viewport used to display the layer",
                         SHUMATE_TYPE_VIEWPORT,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class,
                                     N_PROPERTIES,
                                     obj_properties);

  gtk_widget_class_set_css_name (widget_class, "map-layer");
}

static void
shumate_layer_init (ShumateLayer *self)
{
  g_object_set (G_OBJECT (self),
                "hexpand", TRUE,
                "vexpand", TRUE,
                NULL);
}

/**
 * shumate_layer_get_viewport:
 * @self: a #ShumateLayer
 *
 * Gets the #ShumateViewport used by this layer.
 *
 * Returns: (transfer none): The #ShumateViewport.
 */
ShumateViewport *
shumate_layer_get_viewport (ShumateLayer *self)
{
  ShumateLayerPrivate *priv = shumate_layer_get_instance_private (self);

  g_return_val_if_fail (SHUMATE_IS_LAYER (self), NULL);

  return priv->viewport;
}
