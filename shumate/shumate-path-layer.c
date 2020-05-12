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

#include "shumate-defines.h"
#include "shumate-enum-types.h"
#include "shumate-private.h"
#include "shumate-view.h"

#include <cairo/cairo-gobject.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <glib.h>

enum
{
  /* normal signals */
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_CLOSED_PATH,
  PROP_STROKE_WIDTH,
  PROP_STROKE_COLOR,
  PROP_FILL,
  PROP_FILL_COLOR,
  PROP_STROKE,
  PROP_VISIBLE,
};

static GdkRGBA DEFAULT_FILL_COLOR = { 0.8, 0.0, 0.0, 0.67 };
static GdkRGBA DEFAULT_STROKE_COLOR = { 0.64, 0.0, 0.0, 1.0 };

/* static guint signals[LAST_SIGNAL] = { 0, }; */

typedef struct
{
  ShumateView *view;

  gboolean closed_path;
  GdkRGBA *stroke_color;
  gboolean fill;
  GdkRGBA *fill_color;
  gboolean stroke;
  gdouble stroke_width;
  gboolean visible;
  gdouble *dash;
  guint num_dashes;

  cairo_surface_t *surface;

  /* In order to correctly render paths in the horizontal wrap,
   * the path_actor (a map-wide actor) contains two children that
   * split the visible paths.
   *
   * The right_actor renders paths visible on the original map layer.
   * (from viewport's x coordinate to the rightmost point on the map)

   * The left_actor renders paths visible on the first cloned map layer.
   * (a fixed size, from the leftmost point on the map)
   *
   * If horizontal wrap is disabled, the left_actor won't render
   * anything.
   */
  /*
  ClutterActor *path_actor;

  ClutterActor *right_actor;
  ClutterActor *left_actor;

  ClutterContent *right_canvas;
  ClutterContent *left_canvas;
   */
  cairo_surface_t *right_surface;
  cairo_surface_t *left_surface;

  gboolean right_surface_updated;
  gboolean left_surface_updated;

  GList *nodes;
  gboolean redraw_scheduled;
} ShumatePathLayerPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (ShumatePathLayer, shumate_path_layer, SHUMATE_TYPE_LAYER);

/*
static gboolean redraw_path (ClutterCanvas *canvas,
    cairo_t *cr,
    int w,
    int h,
    ShumatePathLayer *layer);
*/

static void set_view (ShumateLayer *layer,
    ShumateView *view);

static ShumateBoundingBox *get_bounding_box (ShumateLayer *layer);


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
      g_value_set_boxed (value, gdk_rgba_copy (priv->fill_color));
      break;

    case PROP_STROKE_COLOR:
      g_value_set_boxed (value, gdk_rgba_copy (priv->stroke_color));
      break;

    case PROP_STROKE_WIDTH:
      g_value_set_double (value, priv->stroke_width);
      break;

    case PROP_VISIBLE:
      g_value_set_boolean (value, priv->visible);
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
          gdk_rgba_copy (g_value_get_boxed (value)));
      break;

    case PROP_STROKE_COLOR:
      shumate_path_layer_set_stroke_color (SHUMATE_PATH_LAYER (object),
          gdk_rgba_copy (g_value_get_boxed (value)));
      break;

    case PROP_STROKE_WIDTH:
      shumate_path_layer_set_stroke_width (SHUMATE_PATH_LAYER (object),
          g_value_get_double (value));
      break;

    case PROP_VISIBLE:
      shumate_path_layer_set_visible (SHUMATE_PATH_LAYER (object),
          g_value_get_boolean (value));
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

  if (priv->view != NULL)
    set_view (SHUMATE_LAYER (self), NULL);

  /*
  if (priv->right_canvas)
    {
      g_object_unref (priv->right_canvas);
      g_object_unref (priv->left_canvas);
      priv->right_canvas = NULL;
      priv->left_canvas = NULL;
    }
   */

  g_clear_pointer (&priv->surface, cairo_surface_destroy);
  g_clear_pointer (&priv->right_surface, cairo_surface_destroy);
  g_clear_pointer (&priv->left_surface, cairo_surface_destroy);

  G_OBJECT_CLASS (shumate_path_layer_parent_class)->dispose (object);
}


static void
shumate_path_layer_finalize (GObject *object)
{
  ShumatePathLayer *self = SHUMATE_PATH_LAYER (object);
  ShumatePathLayerPrivate *priv = shumate_path_layer_get_instance_private (self);

  gdk_rgba_free (priv->stroke_color);
  gdk_rgba_free (priv->fill_color);
  g_free (priv->dash);

  G_OBJECT_CLASS (shumate_path_layer_parent_class)->finalize (object);
}


static void
shumate_path_layer_class_init (ShumatePathLayerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ShumateLayerClass *layer_class = SHUMATE_LAYER_CLASS (klass);

  object_class->finalize = shumate_path_layer_finalize;
  object_class->dispose = shumate_path_layer_dispose;
  object_class->get_property = shumate_path_layer_get_property;
  object_class->set_property = shumate_path_layer_set_property;

  layer_class->set_view = set_view;
  layer_class->get_bounding_box = get_bounding_box;

  /**
   * ShumatePathLayer:closed:
   *
   * The shape is a closed path
   */
  g_object_class_install_property (object_class,
      PROP_CLOSED_PATH,
      g_param_spec_boolean ("closed",
          "Closed Path",
          "The Path is Closed",
          FALSE,
          SHUMATE_PARAM_READWRITE));

  /**
   * ShumatePathLayer:fill:
   *
   * The shape should be filled
   */
  g_object_class_install_property (object_class,
      PROP_FILL,
      g_param_spec_boolean ("fill",
          "Fill",
          "The shape is filled",
          FALSE,
          SHUMATE_PARAM_READWRITE));

  /**
   * ShumatePathLayer:stroke:
   *
   * The shape should be stroked
   */
  g_object_class_install_property (object_class,
      PROP_STROKE,
      g_param_spec_boolean ("stroke",
          "Stroke",
          "The shape is stroked",
          TRUE,
          SHUMATE_PARAM_READWRITE));

  /**
   * ShumatePathLayer:stroke-color:
   *
   * The path's stroke color
   */
  g_object_class_install_property (object_class,
      PROP_STROKE_COLOR,
      g_param_spec_boxed ("stroke-color",
          "Stroke Color",
          "The path's stroke color",
          GDK_TYPE_RGBA,
          G_PARAM_READWRITE));

  /**
   * ShumatePathLayer:fill-color:
   *
   * The path's fill color
   */
  g_object_class_install_property (object_class,
      PROP_FILL_COLOR,
      g_param_spec_boxed ("fill-color",
          "Fill Color",
          "The path's fill color",
          GDK_TYPE_RGBA,
          G_PARAM_READWRITE));

  /**
   * ShumatePathLayer:stroke-width:
   *
   * The path's stroke width (in pixels)
   */
  g_object_class_install_property (object_class,
      PROP_STROKE_WIDTH,
      g_param_spec_double ("stroke-width",
          "Stroke Width",
          "The path's stroke width",
          0,
          100.0,
          2.0,
          SHUMATE_PARAM_READWRITE));

  /**
   * ShumatePathLayer:visible:
   *
   * Wether the path is visible
   */
  g_object_class_install_property (object_class,
      PROP_VISIBLE,
      g_param_spec_boolean ("visible",
          "Visible",
          "The path's visibility",
          TRUE,
          SHUMATE_PARAM_READWRITE));
}

/*
static void
initialize_child_actor (ShumatePathLayer *self,
    ClutterActor *child_actor,
    ClutterContent *canvas)
{
  ShumatePathLayerPrivate *priv = self->priv;;

  clutter_actor_set_content (child_actor, canvas);
  g_signal_connect (canvas, "draw", G_CALLBACK (redraw_path), self);
  clutter_actor_set_size (child_actor, 255, 255);
  clutter_actor_add_child (priv->path_actor, child_actor);
}
 */

static void
shumate_path_layer_init (ShumatePathLayer *self)
{
  ShumatePathLayerPrivate *priv = shumate_path_layer_get_instance_private (self);

  priv->view = NULL;

  priv->visible = TRUE;
  priv->fill = FALSE;
  priv->stroke = TRUE;
  priv->stroke_width = 2.0;
  priv->nodes = NULL;
  priv->dash = NULL;
  priv->num_dashes = 0;
  priv->redraw_scheduled = FALSE;

  priv->fill_color = gdk_rgba_copy (&DEFAULT_FILL_COLOR);
  priv->stroke_color = gdk_rgba_copy (&DEFAULT_STROKE_COLOR);

  /*
  priv->path_actor = clutter_actor_new ();
  clutter_actor_add_child (CLUTTER_ACTOR (self), priv->path_actor);
  clutter_actor_set_size (priv->path_actor, 255, 255);

  priv->right_actor = clutter_actor_new ();
  priv->left_actor = clutter_actor_new ();

  priv->right_canvas = clutter_canvas_new ();
  priv->left_canvas = clutter_canvas_new ();
   */

  priv->surface = NULL;
  priv->right_surface = NULL;
  priv->left_surface = NULL;

  priv->right_surface_updated = FALSE;
  priv->left_surface_updated = FALSE;

  /*
  clutter_canvas_set_size (CLUTTER_CANVAS (priv->right_canvas), 255, 255);
  clutter_canvas_set_size (CLUTTER_CANVAS (priv->left_canvas), 0, 0);

  initialize_child_actor (self, priv->right_actor, priv->right_canvas);
  initialize_child_actor (self, priv->left_actor, priv->left_canvas);
   */
}

static void
get_map_size (ShumateView *view, gint *width, gint *height)
{
  gint size, rows, cols;
  ShumateMapSource *map_source = shumate_view_get_map_source (view);
  gint zoom_level = shumate_view_get_zoom_level (view);
  size = shumate_map_source_get_tile_size (map_source);
  rows = shumate_map_source_get_row_count (map_source,
                                                zoom_level);
  cols = shumate_map_source_get_column_count (map_source,
                                                zoom_level);
  if (width)
    *width = size * rows;

  if (height)
    *height = size * cols;
}

static cairo_surface_t *
create_merged_surface (ShumatePathLayer *layer)
{
  ShumatePathLayerPrivate *priv = shumate_path_layer_get_instance_private (layer);
  gint view_width, view_height;
  gint map_width, viewport_x, anchor_x;
  cairo_surface_t *new_surface;
  cairo_t *cr;

  get_map_size (priv->view, &map_width, NULL);
  view_width = gtk_widget_get_allocated_width (GTK_WIDGET (priv->view));
  view_height = gtk_widget_get_allocated_height (GTK_WIDGET (priv->view));
  shumate_view_get_viewport_origin (priv->view, &viewport_x, NULL);
  shumate_view_get_viewport_anchor (priv->view, &anchor_x, NULL);
  new_surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, view_width, view_height);
  cr = cairo_create (new_surface);

  cairo_set_source_surface (cr,
                            priv->right_surface,
                            0, 0);
  cairo_paint (cr);

  cairo_set_source_surface (cr,
                            priv->left_surface,
                            map_width - viewport_x - anchor_x, 0);
  cairo_paint (cr);
  cairo_destroy (cr);

  return new_surface;
}

/**
 * shumate_path_layer_new:
 *
 * Creates a new instance of #ShumatePathLayer.
 *
 * Returns: a new instance of #ShumatePathLayer.
 */
ShumatePathLayer *
shumate_path_layer_new ()
{
  return g_object_new (SHUMATE_TYPE_PATH_LAYER, NULL);
}

static gboolean
invalidate_canvas (ShumatePathLayer *layer)
{
  ShumatePathLayerPrivate *priv = shumate_path_layer_get_instance_private (layer);
  gint view_width, view_height;
  gint map_width, map_height;
  gint viewport_x, viewport_y;
  gint anchor_x, anchor_y;
  gfloat right_actor_width, right_actor_height;
  gfloat left_actor_width, left_actor_height;

  right_actor_width = 256;
  right_actor_height = 256;
  left_actor_width = 0;
  left_actor_height = 0;
  map_width = 256;
  map_height = 256;

  if (priv->view != NULL)
    {
      get_map_size (priv->view, &map_width, &map_height);
      view_width = gtk_widget_get_allocated_width (GTK_WIDGET (priv->view));
      view_height = gtk_widget_get_allocated_height (GTK_WIDGET (priv->view));
      shumate_view_get_viewport_origin (priv->view, &viewport_x, &viewport_y);
      shumate_view_get_viewport_anchor (priv->view, &anchor_x, &anchor_y);

      /* For efficiency in terms of clipping, the path actors must have a minimal size.
       * The right_actor renders the paths on the visible side of the original map layer
       * (from viewport offset to end of the map).
       * The left_actor renders the paths on the visible side of the first cloned map layer
       * (from the leftmost point on the map, clamped by the viewport width).
       */
      right_actor_width = MIN (map_width - (viewport_x + anchor_x), view_width);
      right_actor_height = MIN (map_height - (viewport_y + anchor_y), view_height);
      left_actor_width = MIN (view_width - right_actor_width, map_width - right_actor_width);
      left_actor_height = right_actor_height;

      /* Ensure sizes are positive  */
      right_actor_width = MAX (0, right_actor_width);
      right_actor_height = MAX (0, right_actor_height);
      left_actor_width = MAX (0, left_actor_width);
      left_actor_height = MAX (0, left_actor_height);
    }

  /*
  clutter_actor_set_size (priv->path_actor, map_width, map_height);

  clutter_actor_set_size (priv->right_actor, right_actor_width, right_actor_height);
  clutter_canvas_set_size (CLUTTER_CANVAS (priv->right_canvas), right_actor_width, right_actor_height);
  priv->right_surface_updated = FALSE;
  clutter_content_invalidate (priv->right_canvas);
   */

  /* Since the left actor only renders paths visible on the clone, it should be hidden
   * when no clone is visible.
   */
  /*
  if (left_actor_width != 0)
    {
      clutter_actor_set_size (priv->left_actor, left_actor_width, left_actor_height);
      clutter_canvas_set_size (CLUTTER_CANVAS (priv->left_canvas), left_actor_width, left_actor_height);
      priv->left_surface_updated = FALSE;
      clutter_content_invalidate (priv->left_canvas);
      clutter_actor_show (priv->left_actor);
    }
  else
    clutter_actor_hide (priv->left_actor);
  */

  priv->redraw_scheduled = FALSE;

  return FALSE;
}


static void
schedule_redraw (ShumatePathLayer *layer)
{
  ShumatePathLayerPrivate *priv = shumate_path_layer_get_instance_private (layer);

  if (!priv->redraw_scheduled)
    {
      priv->redraw_scheduled = TRUE;
      g_idle_add_full (G_PRIORITY_HIGH_IDLE + 50,
          (GSourceFunc) invalidate_canvas,
          g_object_ref (layer),
          (GDestroyNotify) g_object_unref);
    }
}


static void
position_notify (ShumateLocation *location,
    G_GNUC_UNUSED GParamSpec *pspec,
    ShumatePathLayer *layer)
{
  schedule_redraw (layer);
}


static void
add_node (ShumatePathLayer *layer,
    ShumateLocation *location,
    gboolean prepend,
    guint position)
{
  ShumatePathLayerPrivate *priv = shumate_path_layer_get_instance_private (layer);

  g_signal_connect (G_OBJECT (location), "notify::latitude",
      G_CALLBACK (position_notify), layer);

  g_object_ref_sink (location);

  if (prepend)
    priv->nodes = g_list_prepend (priv->nodes, location);
  else
    priv->nodes = g_list_insert (priv->nodes, location, position);
  schedule_redraw (layer);
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

  g_list_free (priv->nodes);
  priv->nodes = NULL;
  schedule_redraw (layer);
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

  g_signal_handlers_disconnect_by_func (G_OBJECT (location),
      G_CALLBACK (position_notify), layer);

  priv->nodes = g_list_remove (priv->nodes, location);
  g_object_unref (location);
  schedule_redraw (layer);
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


static void
relocate_cb (G_GNUC_UNUSED GObject *gobject,
    ShumatePathLayer *layer)
{
  g_return_if_fail (SHUMATE_IS_PATH_LAYER (layer));

  schedule_redraw (layer);
}

/* static void */
/* update_surface (ShumatePathLayer *layer, */
/*     ClutterCanvas *canvas, */
/*     cairo_surface_t *surface) */
/* { */
/*   ShumatePathLayerPrivate *priv = layer->priv; */

/*   if (canvas == CLUTTER_CANVAS (priv->right_canvas)) */
/*     { */
/*       cairo_surface_destroy (priv->right_surface); */
/*       priv->right_surface = cairo_surface_reference (surface); */
/*       priv->right_surface_updated = TRUE; */
/*     } */
/*   else if (canvas == CLUTTER_CANVAS (priv->left_canvas)) */
/*     { */
/*       cairo_surface_destroy (priv->left_surface); */
/*       priv->left_surface = cairo_surface_reference (surface); */
/*       priv->left_surface_updated = TRUE; */
/*     } */

 /* Updating the exportable surface. Path layer has two surfaces (one for each canvas)
   * which have to be merged into a single new one. */

/*   if (priv->left_surface_updated && priv->right_surface_updated) */
/*     { */
/*       cairo_surface_t *new_surface; */
/*       new_surface = create_merged_surface (layer); */

/*       set_surface (SHUMATE_CAIRO_IMPORTABLE (layer), new_surface); */

/*       cairo_surface_destroy (new_surface); */
/*     } */
  /* When only the right actor is visible, no merging is required */
/*   else if (!CLUTTER_ACTOR_IS_VISIBLE (priv->left_actor)) */
/*     set_surface (SHUMATE_CAIRO_IMPORTABLE (layer), priv->right_surface); */
/* } */

/* static gboolean */
/* redraw_path (ClutterCanvas *canvas, */
/*     cairo_t *cr, */
/*     int width, */
/*     int height, */
/*     ShumatePathLayer *layer) */
/* { */
/*   ShumatePathLayerPrivate *priv = layer->priv; */
/*   GList *elem; */
/*   ShumateView *view = priv->view; */
/*   gint  viewport_x, viewport_y; */
/*   gint anchor_x, anchor_y; */

  /* layer not yet added to the view */
/*   if (view == NULL) */
/*     return FALSE; */

/*   if (!priv->visible || width == 0.0 || height == 0.0) */
/*     return FALSE; */

/*   shumate_view_get_viewport_origin (priv->view, &viewport_x, &viewport_y); */
/*   shumate_view_get_viewport_anchor (priv->view, &anchor_x, &anchor_y); */

/*   if (canvas == CLUTTER_CANVAS (priv->right_canvas)) */
/*     clutter_actor_set_position (priv->right_actor, viewport_x, viewport_y); */
/*   else */
/*     clutter_actor_set_position (priv->left_actor, -anchor_x, viewport_y); */

  /* Clear the drawing area */
/*   cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR); */
/*   cairo_paint (cr); */
/*   cairo_set_operator (cr, CAIRO_OPERATOR_OVER); */

/*   cairo_set_line_join (cr, CAIRO_LINE_JOIN_BEVEL); */

/*   for (elem = priv->nodes; elem != NULL; elem = elem->next) */
/*     { */
/*       ShumateLocation *location = SHUMATE_LOCATION (elem->data); */
/*       gfloat x, y; */

/*       x = shumate_view_longitude_to_x (view, shumate_location_get_longitude (location)); */
/*       y = shumate_view_latitude_to_y (view, shumate_location_get_latitude (location)); */

/*       if (canvas == CLUTTER_CANVAS (priv->right_canvas)) */
/*         cairo_line_to (cr, x, y); */
/*       else */
/*         cairo_line_to (cr, x + (viewport_x + anchor_x), y); */
/*     } */

/*   if (priv->closed_path) */
/*     cairo_close_path (cr); */

/*   cairo_set_source_rgba (cr, */
/*       priv->fill_color->red / 255.0, */
/*       priv->fill_color->green / 255.0, */
/*       priv->fill_color->blue / 255.0, */
/*       priv->fill_color->alpha / 255.0); */

/*   if (priv->fill) */
/*     cairo_fill_preserve (cr); */

/*   cairo_set_source_rgba (cr, */
/*       priv->stroke_color->red / 255.0, */
/*       priv->stroke_color->green / 255.0, */
/*       priv->stroke_color->blue / 255.0, */
/*       priv->stroke_color->alpha / 255.0); */

/*   cairo_set_line_width (cr, priv->stroke_width); */
/*   cairo_set_dash(cr, priv->dash, priv->num_dashes, 0); */

/*   if (priv->stroke) */
/*     cairo_stroke (cr); */

/*   update_surface (layer, canvas, cairo_get_target (cr)); */

/*   return FALSE; */
/* } */


static void
redraw_path_cb (G_GNUC_UNUSED GObject *gobject,
    G_GNUC_UNUSED GParamSpec *arg1,
    ShumatePathLayer *layer)
{
  schedule_redraw (layer);
}


static void
set_view (ShumateLayer *layer,
    ShumateView *view)
{
  ShumatePathLayer *path_layer = SHUMATE_PATH_LAYER (layer);
  ShumatePathLayerPrivate *priv = shumate_path_layer_get_instance_private (path_layer);

  g_return_if_fail (SHUMATE_IS_PATH_LAYER (layer) && (SHUMATE_IS_VIEW (view) || view == NULL));

  if (priv->view != NULL)
    {
      g_signal_handlers_disconnect_by_func (priv->view,
          G_CALLBACK (relocate_cb), path_layer);

      //g_signal_handlers_disconnect_by_func (path_layer->priv->view,
      //    G_CALLBACK (redraw_path_cb), path_layer);

      g_object_unref (priv->view);
    }

  priv->view = view;

  if (view != NULL)
    {
      g_object_ref (view);

      /*
      g_signal_connect (view, "layer-relocated",
          G_CALLBACK (relocate_cb), layer);

      g_signal_connect (view, "notify::latitude",
          G_CALLBACK (redraw_path_cb), layer);

      g_signal_connect (view, "notify::zoom-level",
          G_CALLBACK (redraw_path_cb), layer);
       */

      schedule_redraw (path_layer);
    }
}


static ShumateBoundingBox *
get_bounding_box (ShumateLayer *layer)
{
  ShumatePathLayer *path_layer = SHUMATE_PATH_LAYER (layer);
  ShumatePathLayerPrivate *priv = shumate_path_layer_get_instance_private (path_layer);
  GList *elem;
  ShumateBoundingBox *bbox;

  bbox = shumate_bounding_box_new ();

  for (elem = priv->nodes; elem != NULL; elem = elem->next)
    {
      ShumateLocation *location = SHUMATE_LOCATION (elem->data);
      gdouble lat, lon;

      lat = shumate_location_get_latitude (location);
      lon = shumate_location_get_longitude (location);

      shumate_bounding_box_extend (bbox, lat, lon);
    }

  if (bbox->left == bbox->right)
    {
      bbox->left -= 0.0001;
      bbox->right += 0.0001;
    }

  if (bbox->bottom == bbox->top)
    {
      bbox->bottom -= 0.0001;
      bbox->top += 0.0001;
    }

  return bbox;
}


/**
 * shumate_path_layer_set_fill_color:
 * @layer: a #ShumatePathLayer
 * @color: (allow-none): The path's fill color or NULL to reset to the
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
  g_object_notify (G_OBJECT (layer), "fill-color");

  schedule_redraw (layer);
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
 * @color: (allow-none): The path's stroke color or NULL to reset to the
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
  g_object_notify (G_OBJECT (layer), "stroke-color");

  schedule_redraw (layer);
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
  g_object_notify (G_OBJECT (layer), "stroke");

  schedule_redraw (layer);
}


/**
 * shumate_path_layer_get_stroke:
 * @layer: a #ShumatePathLayer
 *
 * Checks whether the path is stroked.
 *
 * Returns: TRUE if the path is stroked, FALSE otherwise.
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
  g_object_notify (G_OBJECT (layer), "fill");

  schedule_redraw (layer);
}


/**
 * shumate_path_layer_get_fill:
 * @layer: a #ShumatePathLayer
 *
 * Checks whether the path is filled.
 *
 * Returns: TRUE if the path is filled, FALSE otherwise.
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
    gdouble value)
{
  ShumatePathLayerPrivate *priv = shumate_path_layer_get_instance_private (layer);

  g_return_if_fail (SHUMATE_IS_PATH_LAYER (layer));

  priv->stroke_width = value;
  g_object_notify (G_OBJECT (layer), "stroke-width");

  schedule_redraw (layer);
}


/**
 * shumate_path_layer_get_stroke_width:
 * @layer: a #ShumatePathLayer
 *
 * Gets the width of the stroke.
 *
 * Returns: the width of the stroke
 */
gdouble
shumate_path_layer_get_stroke_width (ShumatePathLayer *layer)
{
  ShumatePathLayerPrivate *priv = shumate_path_layer_get_instance_private (layer);

  g_return_val_if_fail (SHUMATE_IS_PATH_LAYER (layer), 0);

  return priv->stroke_width;
}


/**
 * shumate_path_layer_set_visible:
 * @layer: a #ShumatePathLayer
 * @value: TRUE to make the path visible
 *
 * Sets path visibility.
 */
void
shumate_path_layer_set_visible (ShumatePathLayer *layer,
    gboolean value)
{
  ShumatePathLayerPrivate *priv = shumate_path_layer_get_instance_private (layer);

  g_return_if_fail (SHUMATE_IS_PATH_LAYER (layer));

  priv->visible = value;
  /*
  if (value)
    clutter_actor_show (CLUTTER_ACTOR (layer->priv->path_actor));
  else
    clutter_actor_hide (CLUTTER_ACTOR (layer->priv->path_actor));
   */
  g_object_notify (G_OBJECT (layer), "visible");
}


/**
 * shumate_path_layer_get_visible:
 * @layer: a #ShumatePathLayer
 *
 * Gets path visibility.
 *
 * Returns: TRUE when the path is visible, FALSE otherwise
 */
gboolean
shumate_path_layer_get_visible (ShumatePathLayer *layer)
{
  ShumatePathLayerPrivate *priv = shumate_path_layer_get_instance_private (layer);

  g_return_val_if_fail (SHUMATE_IS_PATH_LAYER (layer), FALSE);

  return priv->visible;
}


/**
 * shumate_path_layer_set_closed:
 * @layer: a #ShumatePathLayer
 * @value: TRUE to make the path closed
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
  g_object_notify (G_OBJECT (layer), "closed");

  schedule_redraw (layer);
}


/**
 * shumate_path_layer_get_closed:
 * @layer: a #ShumatePathLayer
 *
 * Gets information whether the path is closed.
 *
 * Returns: TRUE when the path is closed, FALSE otherwise
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
 * passed inside the data pointer of the list (using the GUINT_TO_POINTER conversion)
 *
 * Pass NULL to use solid line.
 */
void
shumate_path_layer_set_dash (ShumatePathLayer *layer,
    GList *dash_pattern)
{
  ShumatePathLayerPrivate *priv = shumate_path_layer_get_instance_private (layer);

  g_return_if_fail (SHUMATE_IS_PATH_LAYER (layer));

  GList *iter;
  guint i;

  if (priv->dash)
    g_free (priv->dash);
  priv->dash = NULL;

  priv->num_dashes = g_list_length (dash_pattern);

  if (dash_pattern == NULL)
    return;

  priv->dash = g_new (gdouble, priv->num_dashes);
  for (iter = dash_pattern, i = 0; iter != NULL; iter = iter->next, i++)
    (priv->dash)[i] = (gdouble) GPOINTER_TO_UINT (iter->data);
}


/**
 * shumate_path_layer_get_dash:
 * @layer: a #ShumatePathLayer
 *
 * Returns the list of dash segment lengths.
 *
 * Returns: (transfer container) (element-type guint): the list
 */
GList *
shumate_path_layer_get_dash (ShumatePathLayer *layer)
{
  ShumatePathLayerPrivate *priv = shumate_path_layer_get_instance_private (layer);

  g_return_val_if_fail (SHUMATE_IS_PATH_LAYER (layer), NULL);

  GList *list = NULL;
  guint i;

  for (i = 0; i < priv->num_dashes; i++)
    list = g_list_append(list, GUINT_TO_POINTER((guint)(priv->dash)[i]));

  return list;
}
