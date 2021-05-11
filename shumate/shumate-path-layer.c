/*
 * Copyright (C) 2008-2009 Pierre-Luc Beaudoin <pierre-luc@pierlux.com>
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
 * SECTION:shumate-path-layer
 * @short_description: A layer displaying line path between inserted #ShumateLocation
 * objects
 *
 * This layer shows a connection between inserted objects implementing the
 * #ShumateLocation interface. This means that both #ShumateMarker
 * objects and #ShumateCoordinate objects can be inserted into the layer.
 * Of course, custom objects implementing the #ShumateLocation interface
 * can be used as well.
 */

#include "config.h"

#include "shumate-path-layer.h"

#include "shumate-enum-types.h"
#include "shumate-view.h"

#include <cairo/cairo-gobject.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <glib.h>

enum
{
  PROP_CLOSED_PATH = 1,
  PROP_STROKE_WIDTH,
  PROP_STROKE_COLOR,
  PROP_FILL,
  PROP_FILL_COLOR,
  PROP_STROKE,
  PROP_OUTLINE_WIDTH,
  PROP_OUTLINE_COLOR,
  N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static GdkRGBA DEFAULT_FILL_COLOR = { 0.8, 0.0, 0.0, 0.67 };
static GdkRGBA DEFAULT_STROKE_COLOR = { 0.64, 0.0, 0.0, 1.0 };
static GdkRGBA DEFAULT_OUTLINE_COLOR = { 1.0, 0.8, 0.8, 1.0 };

typedef struct
{
  gboolean closed_path;
  GdkRGBA *stroke_color;
  gboolean fill;
  GdkRGBA *fill_color;
  gboolean stroke;
  double stroke_width;
  GdkRGBA *outline_color;
  double outline_width;
  GArray *dashes; /* double */

  GList *nodes; /* ShumateLocation */
} ShumatePathLayerPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (ShumatePathLayer, shumate_path_layer, SHUMATE_TYPE_LAYER);

static void
on_view_longitude_changed (ShumatePathLayer *self,
                           GParamSpec       *pspec,
                           ShumateViewport  *view)
{
  g_assert (SHUMATE_IS_PATH_LAYER (self));

  gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
on_view_latitude_changed (ShumatePathLayer *self,
                          GParamSpec       *pspec,
                          ShumateViewport  *view)
{
  g_assert (SHUMATE_IS_PATH_LAYER (self));

  gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
on_view_zoom_level_changed (ShumatePathLayer *self,
                            GParamSpec       *pspec,
                            ShumateViewport  *view)
{
  g_assert (SHUMATE_IS_PATH_LAYER (self));

  gtk_widget_queue_draw (GTK_WIDGET (self));
}


static void
shumate_path_layer_get_property (GObject *object,
    guint property_id,
    G_GNUC_UNUSED GValue *value,
    GParamSpec *pspec)
{
  ShumatePathLayer *self = SHUMATE_PATH_LAYER (object);
  ShumatePathLayerPrivate *priv = shumate_path_layer_get_instance_private (self);

  switch (property_id)
    {
    case PROP_CLOSED_PATH:
      g_value_set_boolean (value, priv->closed_path);
      break;

    case PROP_FILL:
      g_value_set_boolean (value, priv->fill);
      break;

    case PROP_STROKE:
      g_value_set_boolean (value, priv->stroke);
      break;

    case PROP_FILL_COLOR:
      g_value_set_boxed (value, priv->fill_color);
      break;

    case PROP_STROKE_COLOR:
      g_value_set_boxed (value, priv->stroke_color);
      break;

    case PROP_STROKE_WIDTH:
      g_value_set_double (value, priv->stroke_width);
      break;

    case PROP_OUTLINE_COLOR:
      g_value_set_boxed (value, priv->outline_color);
      break;

    case PROP_OUTLINE_WIDTH:
      g_value_set_double (value, priv->outline_width);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}


static void
shumate_path_layer_set_property (GObject *object,
    guint property_id,
    G_GNUC_UNUSED const GValue *value,
    GParamSpec *pspec)
{
  switch (property_id)
    {
    case PROP_CLOSED_PATH:
      shumate_path_layer_set_closed (SHUMATE_PATH_LAYER (object),
          g_value_get_boolean (value));
      break;

    case PROP_FILL:
      shumate_path_layer_set_fill (SHUMATE_PATH_LAYER (object),
          g_value_get_boolean (value));
      break;

    case PROP_STROKE:
      shumate_path_layer_set_stroke (SHUMATE_PATH_LAYER (object),
          g_value_get_boolean (value));
      break;

    case PROP_FILL_COLOR:
      shumate_path_layer_set_fill_color (SHUMATE_PATH_LAYER (object),
          g_value_get_boxed (value));
      break;

    case PROP_STROKE_COLOR:
      shumate_path_layer_set_stroke_color (SHUMATE_PATH_LAYER (object),
          g_value_get_boxed (value));
      break;

    case PROP_STROKE_WIDTH:
      shumate_path_layer_set_stroke_width (SHUMATE_PATH_LAYER (object),
          g_value_get_double (value));
      break;

    case PROP_OUTLINE_COLOR:
      shumate_path_layer_set_outline_color (SHUMATE_PATH_LAYER (object),
          g_value_get_boxed (value));
      break;

    case PROP_OUTLINE_WIDTH:
      shumate_path_layer_set_outline_width (SHUMATE_PATH_LAYER (object),
          g_value_get_double (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}


static void
shumate_path_layer_dispose (GObject *object)
{
  ShumatePathLayer *self = SHUMATE_PATH_LAYER (object);
  ShumatePathLayerPrivate *priv = shumate_path_layer_get_instance_private (self);

  if (priv->nodes)
    shumate_path_layer_remove_all (SHUMATE_PATH_LAYER (object));

  G_OBJECT_CLASS (shumate_path_layer_parent_class)->dispose (object);
}

static void
shumate_path_layer_constructed (GObject *object)
{
  ShumatePathLayer *self = SHUMATE_PATH_LAYER (object);
  ShumateViewport *viewport;

  G_OBJECT_CLASS (shumate_path_layer_parent_class)->constructed (object);

  viewport = shumate_layer_get_viewport (SHUMATE_LAYER (self));
  g_signal_connect_swapped (viewport, "notify::longitude", G_CALLBACK (on_view_longitude_changed), self);
  g_signal_connect_swapped (viewport, "notify::latitude", G_CALLBACK (on_view_latitude_changed), self);
  g_signal_connect_swapped (viewport, "notify::zoom-level", G_CALLBACK (on_view_zoom_level_changed), self);

}


static void
shumate_path_layer_finalize (GObject *object)
{
  ShumatePathLayer *self = SHUMATE_PATH_LAYER (object);
  ShumatePathLayerPrivate *priv = shumate_path_layer_get_instance_private (self);

  g_clear_pointer (&priv->stroke_color, gdk_rgba_free);
  g_clear_pointer (&priv->outline_color, gdk_rgba_free);
  g_clear_pointer (&priv->fill_color, gdk_rgba_free);
  g_clear_pointer (&priv->dashes, g_array_unref);

  G_OBJECT_CLASS (shumate_path_layer_parent_class)->finalize (object);
}

static void
shumate_path_layer_snapshot (GtkWidget   *widget,
                             GtkSnapshot *snapshot)
{
  ShumatePathLayer *self = (ShumatePathLayer *)widget;
  ShumatePathLayerPrivate *priv = shumate_path_layer_get_instance_private (self);
  ShumateViewport *viewport;
  int width, height;
  cairo_t *cr;
  GList *elem;

  width = gtk_widget_get_allocated_width (widget);
  height = gtk_widget_get_allocated_height (widget);
  viewport = shumate_layer_get_viewport (SHUMATE_LAYER (self));

  if (!gtk_widget_get_visible (widget) || width <= 0 || height <= 0)
    return;

  cr = gtk_snapshot_append_cairo (snapshot, &GRAPHENE_RECT_INIT(0, 0, width, height));

  cairo_set_line_join (cr, CAIRO_LINE_JOIN_BEVEL);

  for (elem = priv->nodes; elem != NULL; elem = elem->next)
    {
      ShumateLocation *location = SHUMATE_LOCATION (elem->data);
      double x, y;

      x = shumate_viewport_longitude_to_widget_x (viewport, widget, shumate_location_get_longitude (location));
      y = shumate_viewport_latitude_to_widget_y (viewport, widget, shumate_location_get_latitude (location));

      cairo_line_to (cr, x, y);
    }

  if (priv->closed_path)
    cairo_close_path (cr);

  gdk_cairo_set_source_rgba (cr, priv->fill_color);

  if (priv->fill)
    cairo_fill_preserve (cr);

  if (priv->stroke)
    {
      /* width of the backgroud-colored part of the stroke,
       * will be reduced by the outline, when that is set (non-zero)
       */
      double inner_width = priv->stroke_width - 2 * priv->outline_width;

      cairo_set_dash (cr, (const double *) priv->dashes->data, priv->dashes->len, 0);

      if (priv->outline_width > 0)
        {
          gdk_cairo_set_source_rgba (cr, priv->outline_color);
          cairo_set_line_width (cr, priv->stroke_width);
          cairo_stroke_preserve (cr);
        }

      gdk_cairo_set_source_rgba (cr, priv->stroke_color);
      cairo_set_line_width (cr, inner_width);
      cairo_stroke (cr);
    }

  cairo_destroy (cr);
}


static void
shumate_path_layer_class_init (ShumatePathLayerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = shumate_path_layer_finalize;
  object_class->dispose = shumate_path_layer_dispose;
  object_class->constructed = shumate_path_layer_constructed;
  object_class->get_property = shumate_path_layer_get_property;
  object_class->set_property = shumate_path_layer_set_property;

  widget_class->snapshot = shumate_path_layer_snapshot;

  /**
   * ShumatePathLayer:closed:
   *
   * The shape is a closed path
   */
  obj_properties[PROP_CLOSED_PATH] =
    g_param_spec_boolean ("closed",
                          "Closed Path",
                          "The Path is Closed",
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * ShumatePathLayer:fill:
   *
   * The shape should be filled
   */
  obj_properties[PROP_FILL] =
    g_param_spec_boolean ("fill",
                          "Fill",
                          "The shape is filled",
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * ShumatePathLayer:stroke:
   *
   * The shape should be stroked
   */
  obj_properties[PROP_STROKE] =
    g_param_spec_boolean ("stroke",
                          "Stroke",
                          "The shape is stroked",
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * ShumatePathLayer:stroke-color:
   *
   * The path's stroke color
   */
  obj_properties[PROP_STROKE_COLOR] =
    g_param_spec_boxed ("stroke-color",
                        "Stroke Color",
                        "The path's stroke color",
                        GDK_TYPE_RGBA,
                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * ShumatePathLayer:fill-color:
   *
   * The path's fill color
   */
  obj_properties[PROP_FILL_COLOR] =
    g_param_spec_boxed ("fill-color",
                        "Fill Color",
                        "The path's fill color",
                        GDK_TYPE_RGBA,
                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * ShumatePathLayer:stroke-width:
   *
   * The path's stroke width (in pixels)
   */
  obj_properties[PROP_STROKE_WIDTH] =
    g_param_spec_double ("stroke-width",
                         "Stroke Width",
                         "The path's stroke width",
                         0,
                         100.0,
                         2.0,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * ShumatePathLayer:outline-color:
   *
   * The path's outline color
   */
  obj_properties[PROP_OUTLINE_COLOR] =
    g_param_spec_boxed ("outline-color",
                        "Outline Color",
                        "The path's outline color",
                        GDK_TYPE_RGBA,
                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * ShumatePathLayer:outline-width:
   *
   * The path's outline width (in pixels)
   */
  obj_properties[PROP_OUTLINE_WIDTH] =
    g_param_spec_double ("outline-width",
                         "Outline Width",
                         "The path's outline width",
                         0,
                         50.0,
                         0.0,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class,
                                     N_PROPERTIES,
                                     obj_properties);
}

static void
shumate_path_layer_init (ShumatePathLayer *self)
{
  ShumatePathLayerPrivate *priv = shumate_path_layer_get_instance_private (self);

  priv->fill = FALSE;
  priv->stroke = TRUE;
  priv->stroke_width = 2.0;
  priv->outline_width = 0.0;
  priv->nodes = NULL;
  priv->dashes = g_array_new (FALSE, TRUE, sizeof(double));

  priv->fill_color = gdk_rgba_copy (&DEFAULT_FILL_COLOR);
  priv->stroke_color = gdk_rgba_copy (&DEFAULT_STROKE_COLOR);
  priv->outline_color = gdk_rgba_copy (&DEFAULT_OUTLINE_COLOR);
}

/**
 * shumate_path_layer_new:
 * @viewport: the @ShumateViewport
 *
 * Creates a new instance of #ShumatePathLayer.
 *
 * Returns: a new instance of #ShumatePathLayer.
 */
ShumatePathLayer *
shumate_path_layer_new (ShumateViewport *viewport)
{
  return g_object_new (SHUMATE_TYPE_PATH_LAYER,
                       "viewport", viewport,
                       NULL);
}

static void
position_notify (ShumateLocation  *location,
                 GParamSpec       *pspec,
                 ShumatePathLayer *layer)
{
  gtk_widget_queue_draw (GTK_WIDGET (layer));
}

static void
add_node (ShumatePathLayer *layer,
          ShumateLocation  *location,
          gboolean          prepend,
          guint             position)
{
  ShumatePathLayerPrivate *priv = shumate_path_layer_get_instance_private (layer);

  g_signal_connect (G_OBJECT (location), "notify::latitude", G_CALLBACK (position_notify), layer);

  if (prepend)
    priv->nodes = g_list_prepend (priv->nodes, g_object_ref_sink (location));
  else
    priv->nodes = g_list_insert (priv->nodes, g_object_ref_sink (location), position);

  gtk_widget_queue_draw (GTK_WIDGET (layer));
}


/**
 * shumate_path_layer_add_node:
 * @layer: a #ShumatePathLayer
 * @location: a #ShumateLocation
 *
 * Adds a #ShumateLocation object to the layer.
 * The node is prepended to the list.
 */
void
shumate_path_layer_add_node (ShumatePathLayer *layer,
    ShumateLocation *location)
{
  g_return_if_fail (SHUMATE_IS_PATH_LAYER (layer));
  g_return_if_fail (SHUMATE_IS_LOCATION (location));

  add_node (layer, location, TRUE, 0);
}


/**
 * shumate_path_layer_remove_all:
 * @layer: a #ShumatePathLayer
 *
 * Removes all #ShumateLocation objects from the layer.
 */
void
shumate_path_layer_remove_all (ShumatePathLayer *layer)
{
  ShumatePathLayerPrivate *priv = shumate_path_layer_get_instance_private (layer);
  GList *elem;

  g_return_if_fail (SHUMATE_IS_PATH_LAYER (layer));

  for (elem = priv->nodes; elem != NULL; elem = elem->next)
    {
      GObject *node = G_OBJECT (elem->data);

      g_signal_handlers_disconnect_by_func (node,
          G_CALLBACK (position_notify), layer);

      g_object_unref (node);
    }

  g_clear_pointer (&priv->nodes, g_list_free);
  gtk_widget_queue_draw (GTK_WIDGET (layer));
}


/**
 * shumate_path_layer_get_nodes:
 * @layer: a #ShumatePathLayer
 *
 * Gets a copy of the list of all #ShumateLocation objects inserted into the layer. You should
 * free the list but not its contents.
 *
 * Returns: (transfer container) (element-type ShumateLocation): the list
 */
GList *
shumate_path_layer_get_nodes (ShumatePathLayer *layer)
{
  ShumatePathLayerPrivate *priv = shumate_path_layer_get_instance_private (layer);
  GList *lst;

  g_return_val_if_fail (SHUMATE_IS_PATH_LAYER (layer), NULL);

  lst = g_list_copy (priv->nodes);
  return g_list_reverse (lst);
}

/**
 * shumate_path_layer_remove_node:
 * @layer: a #ShumatePathLayer
 * @location: a #ShumateLocation
 *
 * Removes the #ShumateLocation object from the layer.
 */
void
shumate_path_layer_remove_node (ShumatePathLayer *layer,
    ShumateLocation *location)
{
  ShumatePathLayerPrivate *priv = shumate_path_layer_get_instance_private (layer);

  g_return_if_fail (SHUMATE_IS_PATH_LAYER (layer));
  g_return_if_fail (SHUMATE_IS_LOCATION (location));

  g_signal_handlers_disconnect_by_func (G_OBJECT (location), G_CALLBACK (position_notify), layer);

  priv->nodes = g_list_remove (priv->nodes, location);
  g_object_unref (location);
  gtk_widget_queue_draw (GTK_WIDGET (layer));
}

/**
 * shumate_path_layer_insert_node:
 * @layer: a #ShumatePathLayer
 * @location: a #ShumateLocation
 * @position: position in the list where the #ShumateLocation object should be inserted
 *
 * Inserts a #ShumateLocation object to the specified position.
 */
void
shumate_path_layer_insert_node (ShumatePathLayer *layer,
    ShumateLocation *location,
    guint position)
{
  g_return_if_fail (SHUMATE_IS_PATH_LAYER (layer));
  g_return_if_fail (SHUMATE_IS_LOCATION (location));

  add_node (layer, location, FALSE, position);
}

/**
 * shumate_path_layer_set_fill_color:
 * @layer: a #ShumatePathLayer
 * @color: (nullable): The path's fill color or %NULL to reset to the
 *         default color. The color parameter is copied.
 *
 * Set the path's fill color.
 */
void
shumate_path_layer_set_fill_color (ShumatePathLayer *layer,
    const GdkRGBA *color)
{
  ShumatePathLayerPrivate *priv = shumate_path_layer_get_instance_private (layer);

  g_return_if_fail (SHUMATE_IS_PATH_LAYER (layer));

  if (priv->fill_color != NULL)
    gdk_rgba_free (priv->fill_color);

  if (color == NULL)
    color = &DEFAULT_FILL_COLOR;

  priv->fill_color = gdk_rgba_copy (color);
  g_object_notify_by_pspec (G_OBJECT (layer), obj_properties[PROP_FILL_COLOR]);

  gtk_widget_queue_draw (GTK_WIDGET (layer));
}


/**
 * shumate_path_layer_get_fill_color:
 * @layer: a #ShumatePathLayer
 *
 * Gets the path's fill color.
 *
 * Returns: the path's fill color.
 */
GdkRGBA *
shumate_path_layer_get_fill_color (ShumatePathLayer *layer)
{
  ShumatePathLayerPrivate *priv = shumate_path_layer_get_instance_private (layer);

  g_return_val_if_fail (SHUMATE_IS_PATH_LAYER (layer), NULL);

  return priv->fill_color;
}


/**
 * shumate_path_layer_set_stroke_color:
 * @layer: a #ShumatePathLayer
 * @color: (nullable): The path's stroke color or %NULL to reset to the
 *         default color. The color parameter is copied.
 *
 * Set the path's stroke color.
 */
void
shumate_path_layer_set_stroke_color (ShumatePathLayer *layer,
    const GdkRGBA *color)
{
  ShumatePathLayerPrivate *priv = shumate_path_layer_get_instance_private (layer);

  g_return_if_fail (SHUMATE_IS_PATH_LAYER (layer));

  if (priv->stroke_color != NULL)
    gdk_rgba_free (priv->stroke_color);

  if (color == NULL)
    color = &DEFAULT_STROKE_COLOR;

  priv->stroke_color = gdk_rgba_copy (color);
  g_object_notify_by_pspec (G_OBJECT (layer), obj_properties[PROP_STROKE_COLOR]);

  gtk_widget_queue_draw (GTK_WIDGET (layer));
}


/**
 * shumate_path_layer_get_stroke_color:
 * @layer: a #ShumatePathLayer
 *
 * Gets the path's stroke color.
 *
 * Returns: the path's stroke color.
 */
GdkRGBA *
shumate_path_layer_get_stroke_color (ShumatePathLayer *layer)
{
  ShumatePathLayerPrivate *priv = shumate_path_layer_get_instance_private (layer);

  g_return_val_if_fail (SHUMATE_IS_PATH_LAYER (layer), NULL);

  return priv->stroke_color;
}

/**
 * shumate_path_layer_set_outline_color:
 * @layer: a #ShumatePathLayer
 * @color: (nullable): The path's outline color or %NULL to reset to the
 *         default color. The color parameter is copied.
 *
 * Set the path's outline color.
 */
void
shumate_path_layer_set_outline_color (ShumatePathLayer *layer,
    const GdkRGBA *color)
{
  ShumatePathLayerPrivate *priv = shumate_path_layer_get_instance_private (layer);

  g_return_if_fail (SHUMATE_IS_PATH_LAYER (layer));

  if (priv->outline_color != NULL)
    gdk_rgba_free (priv->outline_color);

  if (color == NULL)
    color = &DEFAULT_OUTLINE_COLOR;

  priv->outline_color = gdk_rgba_copy (color);
  g_object_notify_by_pspec (G_OBJECT (layer), obj_properties[PROP_OUTLINE_COLOR]);

  gtk_widget_queue_draw (GTK_WIDGET (layer));
}

/**
 * shumate_path_layer_get_outline_color:
 * @layer: a #ShumatePathLayer
 *
 * Gets the path's outline color.
 *
 * Returns: the path's outline color.
 */
GdkRGBA *
shumate_path_layer_get_outline_color (ShumatePathLayer *layer)
{
  ShumatePathLayerPrivate *priv = shumate_path_layer_get_instance_private (layer);

  g_return_val_if_fail (SHUMATE_IS_PATH_LAYER (layer), NULL);

  return priv->outline_color;
}

/**
 * shumate_path_layer_set_stroke:
 * @layer: a #ShumatePathLayer
 * @value: if the path is stroked
 *
 * Sets the path to be stroked
 */
void
shumate_path_layer_set_stroke (ShumatePathLayer *layer,
    gboolean value)
{
  ShumatePathLayerPrivate *priv = shumate_path_layer_get_instance_private (layer);

  g_return_if_fail (SHUMATE_IS_PATH_LAYER (layer));

  priv->stroke = value;
  g_object_notify_by_pspec (G_OBJECT (layer), obj_properties[PROP_STROKE]);

  gtk_widget_queue_draw (GTK_WIDGET (layer));
}


/**
 * shumate_path_layer_get_stroke:
 * @layer: a #ShumatePathLayer
 *
 * Checks whether the path is stroked.
 *
 * Returns: %TRUE if the path is stroked, %FALSE otherwise.
 */
gboolean
shumate_path_layer_get_stroke (ShumatePathLayer *layer)
{
  ShumatePathLayerPrivate *priv = shumate_path_layer_get_instance_private (layer);

  g_return_val_if_fail (SHUMATE_IS_PATH_LAYER (layer), FALSE);

  return priv->stroke;
}


/**
 * shumate_path_layer_set_fill:
 * @layer: a #ShumatePathLayer
 * @value: if the path is filled
 *
 * Sets the path to be filled
 */
void
shumate_path_layer_set_fill (ShumatePathLayer *layer,
    gboolean value)
{
  ShumatePathLayerPrivate *priv = shumate_path_layer_get_instance_private (layer);

  g_return_if_fail (SHUMATE_IS_PATH_LAYER (layer));

  priv->fill = value;
  g_object_notify_by_pspec (G_OBJECT (layer), obj_properties[PROP_FILL]);

  gtk_widget_queue_draw (GTK_WIDGET (layer));
}


/**
 * shumate_path_layer_get_fill:
 * @layer: a #ShumatePathLayer
 *
 * Checks whether the path is filled.
 *
 * Returns: %TRUE if the path is filled, %FALSE otherwise.
 */
gboolean
shumate_path_layer_get_fill (ShumatePathLayer *layer)
{
  ShumatePathLayerPrivate *priv = shumate_path_layer_get_instance_private (layer);

  g_return_val_if_fail (SHUMATE_IS_PATH_LAYER (layer), FALSE);

  return priv->fill;
}


/**
 * shumate_path_layer_set_stroke_width:
 * @layer: a #ShumatePathLayer
 * @value: the width of the stroke (in pixels)
 *
 * Sets the width of the stroke
 */
void
shumate_path_layer_set_stroke_width (ShumatePathLayer *layer,
    double value)
{
  ShumatePathLayerPrivate *priv = shumate_path_layer_get_instance_private (layer);

  g_return_if_fail (SHUMATE_IS_PATH_LAYER (layer));

  priv->stroke_width = value;
  g_object_notify_by_pspec (G_OBJECT (layer), obj_properties[PROP_STROKE_WIDTH]);

  gtk_widget_queue_draw (GTK_WIDGET (layer));
}


/**
 * shumate_path_layer_get_stroke_width:
 * @layer: a #ShumatePathLayer
 *
 * Gets the width of the stroke.
 *
 * Returns: the width of the stroke
 */
double
shumate_path_layer_get_stroke_width (ShumatePathLayer *layer)
{
  ShumatePathLayerPrivate *priv = shumate_path_layer_get_instance_private (layer);

  g_return_val_if_fail (SHUMATE_IS_PATH_LAYER (layer), 0);

  return priv->stroke_width;
}

/**
 * shumate_path_layer_set_outline_width:
 * @layer: a #ShumatePathLayer
 * @value: the width of the outline (in pixels)
 *
 * Sets the width of the outline
 */
void
shumate_path_layer_set_outline_width (ShumatePathLayer *layer,
    double value)
{
  ShumatePathLayerPrivate *priv = shumate_path_layer_get_instance_private (layer);

  g_return_if_fail (SHUMATE_IS_PATH_LAYER (layer));

  priv->outline_width = value;
  g_object_notify_by_pspec (G_OBJECT (layer), obj_properties[PROP_OUTLINE_WIDTH]);

  gtk_widget_queue_draw (GTK_WIDGET (layer));
}


/**
 * shumate_path_layer_get_outline_width:
 * @layer: a #ShumatePathLayer
 *
 * Gets the width of the outline.
 *
 * Returns: the width of the outline
 */
double
shumate_path_layer_get_outline_width (ShumatePathLayer *layer)
{
  ShumatePathLayerPrivate *priv = shumate_path_layer_get_instance_private (layer);

  g_return_val_if_fail (SHUMATE_IS_PATH_LAYER (layer), 0);

  return priv->outline_width;
}

/**
 * shumate_path_layer_set_closed:
 * @layer: a #ShumatePathLayer
 * @value: %TRUE to make the path closed
 *
 * Makes the path closed.
 */
void
shumate_path_layer_set_closed (ShumatePathLayer *layer,
    gboolean value)
{
  ShumatePathLayerPrivate *priv = shumate_path_layer_get_instance_private (layer);

  g_return_if_fail (SHUMATE_IS_PATH_LAYER (layer));

  priv->closed_path = value;
  g_object_notify_by_pspec (G_OBJECT (layer), obj_properties[PROP_CLOSED_PATH]);

  gtk_widget_queue_draw (GTK_WIDGET (layer));
}


/**
 * shumate_path_layer_get_closed:
 * @layer: a #ShumatePathLayer
 *
 * Gets information whether the path is closed.
 *
 * Returns: %TRUE when the path is closed, %FALSE otherwise
 */
gboolean
shumate_path_layer_get_closed (ShumatePathLayer *layer)
{
  ShumatePathLayerPrivate *priv = shumate_path_layer_get_instance_private (layer);

  g_return_val_if_fail (SHUMATE_IS_PATH_LAYER (layer), FALSE);

  return priv->closed_path;
}


/**
 * shumate_path_layer_set_dash:
 * @layer: a #ShumatePathLayer
 * @dash_pattern: (element-type guint): list of integer values representing lengths
 *     of dashes/spaces (see cairo documentation of cairo_set_dash())
 *
 * Sets dashed line pattern in a way similar to cairo_set_dash() of cairo. This
 * method supports only integer values for segment lengths. The values have to be
 * passed inside the data pointer of the list (using the %GUINT_TO_POINTER conversion)
 *
 * Pass %NULL to use solid line.
 */
void
shumate_path_layer_set_dash (ShumatePathLayer *layer,
    GList *dash_pattern)
{
  ShumatePathLayerPrivate *priv = shumate_path_layer_get_instance_private (layer);
  GList *iter = NULL;

  g_return_if_fail (SHUMATE_IS_PATH_LAYER (layer));

  g_array_set_size (priv->dashes, 0);
  if (dash_pattern == NULL)
    return;

  for (iter = dash_pattern; iter != NULL; iter = iter->next)
    {
      double val = (double) GPOINTER_TO_UINT (iter->data);
      g_array_append_val (priv->dashes, val);
    }
}


/**
 * shumate_path_layer_get_dash:
 * @layer: a #ShumatePathLayer
 *
 * Returns the list of dash segment lengths.
 *
 * Returns: (transfer full) (element-type guint): the list
 */
GList *
shumate_path_layer_get_dash (ShumatePathLayer *layer)
{
  ShumatePathLayerPrivate *priv = shumate_path_layer_get_instance_private (layer);
  GList *list = NULL;
  guint i;

  g_return_val_if_fail (SHUMATE_IS_PATH_LAYER (layer), NULL);

  for (i = 0; i < priv->dashes->len; i++)
    list = g_list_append (list, GUINT_TO_POINTER ((guint) g_array_index (priv->dashes, double, i)));

  return list;
}
