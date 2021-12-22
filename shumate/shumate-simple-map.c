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

#include "shumate-simple-map.h"
#include "shumate-map.h"
#include "shumate-map-layer.h"

struct _ShumateSimpleMap
{
  GtkWidget parent_instance;

  ShumateMapSource *map_source;
  GList *overlay_layers;

  ShumateMap *map;
  ShumateMapLayer *map_layer;

  ShumateLicense *license;
  ShumateScale *scale;
  ShumateCompass *compass;

  GtkBox *buttons;
  GtkBox *zoom_buttons;
};

static void buildable_interface_init (GtkBuildableIface *iface);

G_DEFINE_TYPE_WITH_CODE (ShumateSimpleMap, shumate_simple_map, GTK_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_BUILDABLE, buildable_interface_init))

enum {
  PROP_0,
  PROP_MAP_SOURCE,
  PROP_VIEWPORT,
  PROP_COMPASS,
  PROP_LICENSE,
  PROP_SCALE,
  PROP_SHOW_ZOOM_BUTTONS,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];

ShumateSimpleMap *
shumate_simple_map_new (void)
{
  return g_object_new (SHUMATE_TYPE_SIMPLE_MAP, NULL);
}


static void
shumate_simple_map_finalize (GObject *object)
{
  ShumateSimpleMap *self = (ShumateSimpleMap *)object;

  g_clear_object (&self->map_source);
  g_clear_list (&self->overlay_layers, NULL);

  G_OBJECT_CLASS (shumate_simple_map_parent_class)->finalize (object);
}


static void
shumate_simple_map_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  ShumateSimpleMap *self = SHUMATE_SIMPLE_MAP (object);

  switch (prop_id)
    {
    case PROP_MAP_SOURCE:
      g_value_set_object (value, shumate_simple_map_get_map_source (self));
      break;

    case PROP_VIEWPORT:
      g_value_set_object (value, shumate_simple_map_get_viewport (self));
      break;

    case PROP_COMPASS:
      g_value_set_object (value, shumate_simple_map_get_compass (self));
      break;

    case PROP_SCALE:
      g_value_set_object (value, shumate_simple_map_get_scale (self));
      break;

    case PROP_LICENSE:
      g_value_set_object (value, shumate_simple_map_get_license (self));
      break;

    case PROP_SHOW_ZOOM_BUTTONS:
      g_value_set_boolean (value, shumate_simple_map_get_show_zoom_buttons (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}


static void
shumate_simple_map_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  ShumateSimpleMap *self = SHUMATE_SIMPLE_MAP (object);

  switch (prop_id)
    {
    case PROP_MAP_SOURCE:
      shumate_simple_map_set_map_source (self, g_value_get_object (value));
      break;

    case PROP_SHOW_ZOOM_BUTTONS:
      shumate_simple_map_set_show_zoom_buttons (self, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}


static void
shumate_simple_map_dispose (GObject *object)
{
  GtkWidget *child;

  while ((child = gtk_widget_get_first_child (GTK_WIDGET (object))))
    gtk_widget_unparent (child);

  G_OBJECT_CLASS (shumate_simple_map_parent_class)->dispose (object);
}


static void
on_zoom_in_clicked (ShumateSimpleMap *self,
                    GtkButton        *button)
{
  shumate_viewport_zoom_in (shumate_map_get_viewport (self->map));
}


static void
on_zoom_out_clicked (ShumateSimpleMap *self,
                     GtkButton        *button)
{
  shumate_viewport_zoom_out (shumate_map_get_viewport (self->map));
}


static GObject *
shumate_simple_map_get_internal_child (GtkBuildable *buildable,
                                       GtkBuilder *builder,
                                       const char *childname)
{
  ShumateSimpleMap *self = SHUMATE_SIMPLE_MAP (buildable);

  if (!g_strcmp0 (childname, "compass"))
    return G_OBJECT (self->compass);
  else if (!g_strcmp0 (childname, "license"))
    return G_OBJECT (self->license);
  else if (!g_strcmp0 (childname, "scale"))
    return G_OBJECT (self->scale);
  else if (!g_strcmp0 (childname, "map"))
    return G_OBJECT (self->map);
  else
    return NULL;
}


static void
shumate_simple_map_class_init (ShumateSimpleMapClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = shumate_simple_map_dispose;
  object_class->finalize = shumate_simple_map_finalize;
  object_class->get_property = shumate_simple_map_get_property;
  object_class->set_property = shumate_simple_map_set_property;

  properties[PROP_VIEWPORT] =
    g_param_spec_object ("viewport",
                         "Viewport",
                         "Viewport",
                         SHUMATE_TYPE_VIEWPORT,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  properties[PROP_MAP_SOURCE] =
    g_param_spec_object ("map-source",
                         "Map source",
                         "Map source",
                         SHUMATE_TYPE_MAP_SOURCE,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  properties[PROP_COMPASS] =
    g_param_spec_object ("compass",
                         "Compass",
                         "Compass",
                         SHUMATE_TYPE_COMPASS,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  properties[PROP_LICENSE] =
    g_param_spec_object ("license",
                         "License",
                         "License",
                         SHUMATE_TYPE_LICENSE,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  properties[PROP_SCALE] =
    g_param_spec_object ("scale",
                         "Scale",
                         "Scale",
                         SHUMATE_TYPE_SCALE,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  properties[PROP_SHOW_ZOOM_BUTTONS] =
    g_param_spec_boolean ("show-zoom-buttons",
                          "Show zoom buttons",
                          "Show zoom buttons",
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, N_PROPS, properties);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/shumate/shumate-simple-map.ui");
  gtk_widget_class_bind_template_child (widget_class, ShumateSimpleMap, map);
  gtk_widget_class_bind_template_child (widget_class, ShumateSimpleMap, license);
  gtk_widget_class_bind_template_child (widget_class, ShumateSimpleMap, scale);
  gtk_widget_class_bind_template_child (widget_class, ShumateSimpleMap, compass);
  gtk_widget_class_bind_template_child (widget_class, ShumateSimpleMap, zoom_buttons);
  gtk_widget_class_bind_template_callback (widget_class, on_zoom_in_clicked);
  gtk_widget_class_bind_template_callback (widget_class, on_zoom_out_clicked);
  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
}

static void
buildable_interface_init (GtkBuildableIface *iface)
{
  iface->get_internal_child = shumate_simple_map_get_internal_child;
}

static void
shumate_simple_map_init (ShumateSimpleMap *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}


/**
 * shumate_simple_map_get_viewport:
 * @self: a [class@Map]
 *
 * Gets the map's viewport, needed for constructing map layers that will be added
 * to it.
 *
 * Returns: (transfer none): a [class@Viewport]
 */
ShumateViewport *
shumate_simple_map_get_viewport (ShumateSimpleMap *self)
{
  g_return_val_if_fail (SHUMATE_IS_SIMPLE_MAP (self), NULL);
  return shumate_map_get_viewport (self->map);
}


/**
 * shumate_simple_map_get_map_source:
 * @self: a [class@SimpleMap]
 *
 * Gets the map source for the current base layer.
 *
 * Returns: (transfer none): a [class@MapSource]
 */
ShumateMapSource *
shumate_simple_map_get_map_source (ShumateSimpleMap *self)
{
  g_return_val_if_fail (SHUMATE_IS_SIMPLE_MAP (self), NULL);
  return self->map_source;
}


/**
 * shumate_simple_map_set_map_source:
 * @self: a [class@SimpleMap]
 * @map_source: (nullable): a [class@MapSource]
 *
 * Sets the source for the base map.
 */
void
shumate_simple_map_set_map_source (ShumateSimpleMap *self,
                                   ShumateMapSource *map_source)
{
  ShumateViewport *viewport;
  ShumateMapLayer *map_layer;

  g_return_if_fail (SHUMATE_IS_SIMPLE_MAP (self));
  g_return_if_fail (map_source == NULL || SHUMATE_IS_MAP_SOURCE (map_source));

  viewport = shumate_map_get_viewport (self->map);

  if (self->map_source == map_source)
    return;

  if (self->map_source) {
    shumate_license_remove_map_source (self->license, self->map_source);
  }

  g_set_object (&self->map_source, map_source);

  shumate_viewport_set_reference_map_source (viewport, map_source);
  shumate_map_set_map_source (self->map, map_source);

  map_layer = shumate_map_layer_new (map_source, viewport);
  shumate_map_insert_layer_behind (self->map, SHUMATE_LAYER (map_layer), SHUMATE_LAYER (self->map_layer));
  if (self->map_layer) {
    shumate_map_remove_layer (self->map, SHUMATE_LAYER (self->map_layer));
  }
  self->map_layer = map_layer;

  shumate_license_append_map_source (self->license, map_source);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_MAP_SOURCE]);
}


/**
 * shumate_simple_map_add_overlay_layer:
 * @self: a [class@SimpleMap]
 * @layer: a [class@Layer] to add
 *
 * Adds a map layer as an overlay on top of the base map.
 */
void
shumate_simple_map_add_overlay_layer (ShumateSimpleMap *self,
                                      ShumateLayer     *layer)
{
  g_return_if_fail (SHUMATE_IS_SIMPLE_MAP (self));
  g_return_if_fail (SHUMATE_IS_LAYER (layer));

  self->overlay_layers = g_list_append (self->overlay_layers, layer);
  shumate_map_add_layer (self->map, layer);
}


/**
 * shumate_simple_map_insert_overlay_layer_above:
 * @self: a [class@SimpleMap]
 * @layer: a [class@Layer] to insert
 *
 * Inserts a map layer as an overlay on top of the base map. The layer will
 * appear above @sibling, or at the bottom (but still above the base map)
 * if @sibling is %NULL.
 */
void
shumate_simple_map_insert_overlay_layer_above (ShumateSimpleMap *self,
                                               ShumateLayer     *layer,
                                               ShumateLayer     *sibling)
{
  gint idx;

  g_return_if_fail (SHUMATE_IS_SIMPLE_MAP (self));
  g_return_if_fail (SHUMATE_IS_LAYER (layer));
  g_return_if_fail (sibling == NULL || SHUMATE_IS_LAYER (sibling));

  if (sibling == NULL)
    idx = 0;
  else
    idx = g_list_index (self->overlay_layers, sibling) + 1;

  self->overlay_layers = g_list_insert (self->overlay_layers, layer, idx);
  shumate_map_insert_layer_above (self->map, layer, sibling);
}


/**
 * shumate_simple_map_insert_overlay_layer_behind:
 * @self: a [class@SimpleMap]
 * @layer: a [class@Layer] to insert
 *
 * Inserts a map layer as an overlay on top of the base map. The layer will
 * appear just below @sibling, or above everything else if @sibling is %NULL.
 */
void
shumate_simple_map_insert_overlay_layer_behind (ShumateSimpleMap *self,
                                                ShumateLayer     *layer,
                                                ShumateLayer     *sibling)
{
  GList *sibling_link;

  g_return_if_fail (SHUMATE_IS_SIMPLE_MAP (self));
  g_return_if_fail (SHUMATE_IS_LAYER (layer));
  g_return_if_fail (sibling == NULL || SHUMATE_IS_LAYER (sibling));

  sibling_link = g_list_find (self->overlay_layers, sibling);
  self->overlay_layers = g_list_insert_before (self->overlay_layers, sibling_link, layer);
  shumate_map_insert_layer_behind (self->map, layer, sibling);
}


/**
 * shumate_simple_map_remove_overlay_layer:
 * @self: a [class@SimpleMap]
 * @layer: a [class@Layer] that was added to the map previously
 *
 * Removes a layer from the map.
 */
void
shumate_simple_map_remove_overlay_layer (ShumateSimpleMap *self,
                                         ShumateLayer     *layer)
{
  g_return_if_fail (SHUMATE_IS_SIMPLE_MAP (self));
  g_return_if_fail (SHUMATE_IS_LAYER (layer));

  self->overlay_layers = g_list_remove (self->overlay_layers, layer);
  shumate_map_remove_layer (self->map, layer);
}


/**
 * shumate_simple_map_get_compass:
 * @self: a [class@SimpleMap]
 *
 * Gets the compass widget for the map.
 *
 * Returns: (transfer none): a [class@Compass]
 */
ShumateCompass *
shumate_simple_map_get_compass (ShumateSimpleMap *self)
{
  g_return_val_if_fail (SHUMATE_IS_SIMPLE_MAP (self), NULL);

  return self->compass;
}


/**
 * shumate_simple_map_get_license:
 * @self: a [class@SimpleMap]
 *
 * Gets the license widget for the map.
 *
 * Returns: (transfer none): a [class@License]
 */
ShumateLicense *
shumate_simple_map_get_license (ShumateSimpleMap *self)
{
  g_return_val_if_fail (SHUMATE_IS_SIMPLE_MAP (self), NULL);

  return self->license;
}


/**
 * shumate_simple_map_get_scale:
 * @self: a [class@SimpleMap]
 *
 * Gets the scale widget for the map.
 *
 * Returns: (transfer none): a [class@Scale]
 */
ShumateScale *
shumate_simple_map_get_scale (ShumateSimpleMap *self)
{
  g_return_val_if_fail (SHUMATE_IS_SIMPLE_MAP (self), NULL);
  return self->scale;
}


/**
 * shumate_simple_map_get_show_zoom_buttons:
 * @self: a [class@SimpleMap]
 *
 * Gets whether or not the zoom buttons are shown.
 *
 * Returns: %TRUE if the zoom buttons are visible, otherwise %FALSE
 */
gboolean
shumate_simple_map_get_show_zoom_buttons (ShumateSimpleMap *self)
{
  g_return_val_if_fail (SHUMATE_IS_SIMPLE_MAP (self), FALSE);
  return gtk_widget_is_visible (GTK_WIDGET (self->zoom_buttons));
}


/**
 * shumate_simple_map_set_show_zoom_buttons:
 * @self: a [class@SimpleMap]
 * @show_zoom_buttons: %TRUE to show the zoom buttons, %FALSE to hide them
 *
 * Sets whether or not the zoom buttons are shown.
 */
void
shumate_simple_map_set_show_zoom_buttons (ShumateSimpleMap *self,
                                          gboolean          show_zoom_buttons)
{
  g_return_if_fail (SHUMATE_IS_SIMPLE_MAP (self));
  gtk_widget_set_visible (GTK_WIDGET (self->zoom_buttons), show_zoom_buttons);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SHOW_ZOOM_BUTTONS]);
}
