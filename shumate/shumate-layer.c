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
 * SECTION:shumate-layer
 * @short_description: Base class of libshumate layers
 *
 * Every layer (overlay that moves together with the map) has to inherit this
 * class and implement its virtual methods.
 */

#include "shumate-layer.h"

G_DEFINE_ABSTRACT_TYPE (ShumateLayer, shumate_layer, GTK_TYPE_WIDGET)

static void
shumate_layer_class_init (ShumateLayerClass *klass)
{
  klass->set_view = NULL;
  klass->get_bounding_box = NULL;
  gtk_widget_class_set_css_name (GTK_WIDGET_CLASS (klass), g_intern_static_string ("map-layer"));
}


/**
 * shumate_layer_set_view:
 * @layer: a #ShumateLayer
 * @view: a #ShumateView
 *
 * #ShumateView calls this method to pass a reference to itself to the layer
 * when the layer is added to the view. When the layer is removed from the
 * view, it passes NULL to the layer. Custom layers can implement this method
 * and perform the necessary initialization. This method should not be called
 * by user code.
 */
void
shumate_layer_set_view (ShumateLayer *layer,
    ShumateView *view)
{
  g_return_if_fail (SHUMATE_IS_LAYER (layer));

  SHUMATE_LAYER_GET_CLASS (layer)->set_view (layer, view);
}


/**
 * shumate_layer_get_bounding_box:
 * @layer: a #ShumateLayer
 *
 * Gets the bounding box occupied by the elements inside the layer.
 *
 * Returns: The bounding box.
 */
ShumateBoundingBox *
shumate_layer_get_bounding_box (ShumateLayer *layer)
{
  g_return_val_if_fail (SHUMATE_IS_LAYER (layer), NULL);

  return SHUMATE_LAYER_GET_CLASS (layer)->get_bounding_box (layer);
}


static void
shumate_layer_init (ShumateLayer *self)
{
  g_object_set (G_OBJECT (self),
                "hexpand", TRUE,
                "vexpand", TRUE,
                NULL);
}
