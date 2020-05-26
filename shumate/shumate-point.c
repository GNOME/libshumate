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
 * SECTION:shumate-point
 * @short_description: A simple point to mark a coordinate
 *
 * #ShumatePoint is a simple variant of #ShumateMarker. Contrary to
 * #ShumateLabel, it is not capable of labelling the point with text and
 * only shows the location of the point as a circle on the map.
 */

#include "config.h"

#include "shumate.h"

#include "shumate-defines.h"
#include "shumate-marshal.h"
#include "shumate-tile.h"

#include <cairo/cairo-gobject.h>
#include <gdk/gdk.h>
#include <glib.h>
#include <glib-object.h>
#include <cairo.h>
#include <math.h>
#include <string.h>

#define DEFAULT_FONT_NAME "Sans 11"

static GdkRGBA DEFAULT_COLOR = { 0.2, 0.2, 0.2, 1.0 };

enum
{
  /* normal signals */
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_COLOR,
  PROP_SIZE,
  PROP_SURFACE,
};

/* static guint shumate_point_signals[LAST_SIGNAL] = { 0, }; */

typedef struct
{
  GdkRGBA *color;
  gdouble size;
  //ClutterContent *canvas;
  cairo_surface_t *surface;

  guint redraw_id;
} ShumatePointPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (ShumatePoint, shumate_point, SHUMATE_TYPE_MARKER);

static void
shumate_point_get_property (GObject *object,
    guint prop_id,
    GValue *value,
    GParamSpec *pspec)
{
  ShumatePoint *point = SHUMATE_POINT (object);
  ShumatePointPrivate *priv = shumate_point_get_instance_private (point);

  switch (prop_id)
    {
    case PROP_COLOR:
      g_value_set_boxed (value, priv->color);
      break;

    case PROP_SIZE:
      g_value_set_double (value, priv->size);
      break;

    case PROP_SURFACE:
      g_value_set_boxed (value, priv->surface);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}


static void
shumate_point_set_property (GObject *object,
    guint prop_id,
    const GValue *value,
    GParamSpec *pspec)
{
  ShumatePoint *point = SHUMATE_POINT (object);

  switch (prop_id)
    {
    case PROP_COLOR:
      shumate_point_set_color (point, g_value_get_boxed (value));
      break;

    case PROP_SIZE:
      shumate_point_set_size (point, g_value_get_double (value));
      break;

    case PROP_SURFACE:
      shumate_point_set_surface (point, g_value_get_boxed (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

/*
static void
pick (ClutterActor *self,
    const ClutterColor *color)
{
  ShumatePointPrivate *priv = GET_PRIVATE (self);
  gdouble radius = priv->size / 2.0;

  cogl_path_new ();

  cogl_set_source_color4ub (color->red,
      color->green,
      color->blue,
      color->alpha);

  cogl_path_move_to (radius, radius);
  cogl_path_arc (radius, radius, radius, radius, 0, 360.0);
  cogl_path_close ();
  cogl_path_fill ();
}
*/

static void
shumate_point_dispose (GObject *object)
{
  ShumatePoint *point = SHUMATE_POINT (object);
  ShumatePointPrivate *priv = shumate_point_get_instance_private (point);

  g_clear_pointer (&priv->surface, cairo_surface_destroy);

  G_OBJECT_CLASS (shumate_point_parent_class)->dispose (object);
}


static void
shumate_point_finalize (GObject *object)
{
  ShumatePoint *point = SHUMATE_POINT (object);
  ShumatePointPrivate *priv = shumate_point_get_instance_private (point);

  g_clear_pointer (&priv->color, gdk_rgba_free);

  /*
  if (priv->canvas)
    {
      g_object_unref (priv->canvas);
      priv->canvas = NULL;
    }
   */

  G_OBJECT_CLASS (shumate_point_parent_class)->finalize (object);
}


static void
shumate_point_class_init (ShumatePointClass *klass)
{
  //ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = shumate_point_finalize;
  object_class->dispose = shumate_point_dispose;
  object_class->get_property = shumate_point_get_property;
  object_class->set_property = shumate_point_set_property;

  //actor_class->pick = pick;

  g_object_class_install_property (object_class, PROP_COLOR,
      g_param_spec_boxed ("color",
          "Color",
          "The point's color",
          GDK_TYPE_RGBA,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_SIZE,
      g_param_spec_double ("size",
          "Size",
          "The point size",
          0,
          G_MAXDOUBLE,
          12,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
      PROP_SURFACE,
      g_param_spec_boxed ("surface",
          "Surface",
          "Cairo surface representaion",
          CAIRO_GOBJECT_TYPE_SURFACE,
          G_PARAM_READWRITE));

}


static void
draw (/*ClutterCanvas *canvas,*/
      cairo_t       *cr,
      gint           width,
      gint           height,
      ShumatePoint *point)
{
  ShumatePointPrivate *priv = shumate_point_get_instance_private (point);
  gdouble size = priv->size;
  gdouble radius = size / 2.0;
  const GdkRGBA *color;

  shumate_point_set_surface (point, cairo_get_target (cr));

  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

  if (shumate_marker_get_selected (SHUMATE_MARKER (point)))
    color = shumate_marker_get_selection_color ();
  else
    color = priv->color;

  cairo_set_source_rgba (cr, color->red, color->green, color->blue, color->alpha);

  cairo_arc (cr, radius, radius, radius, 0, 2 * M_PI);
  cairo_fill (cr);

  cairo_fill_preserve (cr);
  cairo_set_line_width (cr, 1.0);
  cairo_stroke (cr);
}


static void
notify_selected (GObject *gobject,
    G_GNUC_UNUSED GParamSpec *pspec,
    G_GNUC_UNUSED gpointer user_data)
{
  //clutter_content_invalidate (SHUMATE_POINT (gobject)->priv->canvas);
}


static void
shumate_point_init (ShumatePoint *point)
{
  ShumatePointPrivate *priv = shumate_point_get_instance_private (point);

  priv->color = gdk_rgba_copy (&DEFAULT_COLOR);
  priv->size = 12;
  /*
  priv->canvas = clutter_canvas_new ();
  g_signal_connect (priv->canvas, "draw", G_CALLBACK (draw), point);
  clutter_canvas_set_size (CLUTTER_CANVAS (priv->canvas), priv->size, priv->size);
  clutter_actor_set_size (CLUTTER_ACTOR (point), priv->size, priv->size);
  clutter_actor_set_content (CLUTTER_ACTOR (point), priv->canvas);
  clutter_actor_set_translation (CLUTTER_ACTOR (point), -priv->size/2, -priv->size/2, 0.0);
  clutter_content_invalidate (priv->canvas);
   */

  g_signal_connect (point, "notify::selected", G_CALLBACK (notify_selected), NULL);
}

/**
 * shumate_point_new:
 *
 * Creates an instance of #ShumatePoint with default size and color.
 *
 * Returns: a new #ShumatePoint.
 */
ShumatePoint *
shumate_point_new (void)
{
  return SHUMATE_POINT (g_object_new (SHUMATE_TYPE_POINT, NULL));
}


/**
 * shumate_point_new_full:
 * @size: Marker size
 * @color: Marker color
 *
 * Creates an instance of #ShumatePoint with the specified size and color.
 *
 * Returns: a new #ShumatePoint.
 */
ShumatePoint *
shumate_point_new_full (gdouble size,
    const GdkRGBA *color)
{
  ShumatePoint *point = SHUMATE_POINT (shumate_point_new ());

  shumate_point_set_size (point, size);
  shumate_point_set_color (point, color);

  return point;
}


/**
 * shumate_point_set_size:
 * @point: a #ShumatePoint
 * @size: The size of the point.
 *
 * Set the size of the point.
 */
void
shumate_point_set_size (ShumatePoint *point,
    gdouble size)
{
  ShumatePointPrivate *priv = shumate_point_get_instance_private (point);

  g_return_if_fail (SHUMATE_IS_POINT (point));

  priv->size = size;
  /*
  clutter_canvas_set_size (CLUTTER_CANVAS (priv->canvas), size, size);
  clutter_actor_set_size (CLUTTER_ACTOR (point), priv->size, priv->size);
  clutter_actor_set_translation (CLUTTER_ACTOR (point), -priv->size/2, -priv->size/2, 0.0);
   */
  g_object_notify (G_OBJECT (point), "size");
  //clutter_content_invalidate (priv->canvas);
}


/**
 * shumate_point_get_size:
 * @point: a #ShumatePoint
 *
 * Gets the size of the point.
 *
 * Returns: the size.
 */
gdouble
shumate_point_get_size (ShumatePoint *point)
{
  ShumatePointPrivate *priv = shumate_point_get_instance_private (point);

  g_return_val_if_fail (SHUMATE_IS_POINT (point), 0);

  return priv->size;
}


/**
 * shumate_point_set_color:
 * @point: a #ShumatePoint
 * @color: (allow-none): The color of the point or NULL to reset the background to the
 *         default color. The color parameter is copied.
 *
 * Set the color of the point.
 */
void
shumate_point_set_color (ShumatePoint *point,
    const GdkRGBA *color)
{
  ShumatePointPrivate *priv = shumate_point_get_instance_private (point);

  g_return_if_fail (SHUMATE_IS_POINT (point));

  if (priv->color != NULL)
    gdk_rgba_free (priv->color);

  if (color == NULL)
    color = &DEFAULT_COLOR;

  priv->color = gdk_rgba_copy (color);
  g_object_notify (G_OBJECT (point), "color");
  //clutter_content_invalidate (priv->canvas);
}


/**
 * shumate_point_get_color:
 * @point: a #ShumatePoint
 *
 * Gets the color of the point.
 *
 * Returns: the color.
 */
GdkRGBA *
shumate_point_get_color (ShumatePoint *point)
{
  ShumatePointPrivate *priv = shumate_point_get_instance_private (point);

  g_return_val_if_fail (SHUMATE_IS_POINT (point), NULL);

  return priv->color;
}

/**
 * shumate_point_get_surface:
 * @point: the #ShumatePoint
 *
 * Returns: (transfer none): A #cairo_surface_t
 */
cairo_surface_t *
shumate_point_get_surface (ShumatePoint *point)
{
  ShumatePointPrivate *priv = shumate_point_get_instance_private (point);

  g_return_val_if_fail (SHUMATE_IS_POINT (point), NULL);

  return priv->surface;
}

/**
 * shumate_point_set_surface:
 * @point: the #ShumatePoint
 * @surface: a #cairo_surface_t
 *
 */
void
shumate_point_set_surface (ShumatePoint *point,
    cairo_surface_t *surface)
{
  ShumatePointPrivate *priv = shumate_point_get_instance_private (point);

  g_return_if_fail (SHUMATE_IS_POINT (point));

  if (priv->surface == surface)
    return;

  g_clear_pointer (&priv->surface, cairo_surface_destroy);
  if (surface)
    priv->surface = cairo_surface_reference (surface);

  g_object_notify (G_OBJECT (point), "surface");
}
