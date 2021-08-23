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

#pragma once

#include <glib-object.h>
#include <json-glib/json-glib.h>
#include <cairo/cairo.h>

G_BEGIN_DECLS

#define SHUMATE_TYPE_VECTOR_LAYER (shumate_vector_layer_get_type())

G_DECLARE_DERIVABLE_TYPE (ShumateVectorLayer, shumate_vector_layer, SHUMATE, VECTOR_LAYER, GObject)

struct _ShumateVectorLayerClass
{
  GObjectClass parent_class;

  void (*render) (ShumateVectorLayer *self, cairo_t *cr);
};

ShumateVectorLayer *shumate_vector_layer_create_from_json (JsonObject *object, GError **error);

void shumate_vector_layer_render (ShumateVectorLayer *self, cairo_t *cr);

G_END_DECLS
