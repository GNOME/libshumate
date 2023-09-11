/*
 * Copyright (C) 2023 James Westman <james@jwestman.net>
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

#include "shumate-vector-sprite.h"

/**
 * ShumateVectorSprite:
 *
 * A sprite used to draw textures or icons.
 *
 * ## Symbolic icons
 *
 * If a sprite is created from a [iface@Gtk.SymbolicPaintable] source, such
 * as a symbolic icon, then when the sprite is part of a symbol layer it
 * will be drawn using the icon-color property (or the text color, if the
 * sprite is part of a formatted string).
 *
 * Since: 1.1
 */


struct _ShumateVectorSprite
{
  GObject parent_instance;

  GdkPaintable *source_paintable;
  int width, height;
  double scale_factor;

  GdkRectangle source_rect;
  gboolean source_rect_set : 1;
};

static void shumate_vector_sprite_paintable_iface_init (GdkPaintableInterface *iface);
static void shumate_vector_sprite_symbolic_paintable_iface_init (GtkSymbolicPaintableInterface *iface);

G_DEFINE_TYPE_WITH_CODE (ShumateVectorSprite, shumate_vector_sprite, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GDK_TYPE_PAINTABLE, shumate_vector_sprite_paintable_iface_init)
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_SYMBOLIC_PAINTABLE, shumate_vector_sprite_symbolic_paintable_iface_init))

enum {
  PROP_0,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_SCALE_FACTOR,
  PROP_SOURCE_PAINTABLE,
  PROP_SOURCE_RECT,
  N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES];

/**
 * shumate_vector_sprite_new:
 * @source_paintable: a [iface@Gdk.Paintable]
 *
 * Creates a new [class@VectorSprite] using the paintable's intrinsic size.
 *
 * Since: 1.1
 */
ShumateVectorSprite *
shumate_vector_sprite_new (GdkPaintable *source_paintable)
{
  return g_object_new (SHUMATE_TYPE_VECTOR_SPRITE,
                       "source-paintable", source_paintable,
                       "width", gdk_paintable_get_intrinsic_width (source_paintable),
                       "height", gdk_paintable_get_intrinsic_height (source_paintable),
                       NULL);
}

/**
 * shumate_vector_sprite_new_full:
 * @source_paintable: a [iface@Gdk.Paintable]
 * @width: the width of the sprite in pixels
 * @height: the height of the sprite in pixels
 * @scale_factor: the intended scale factor of the sprite
 * @source_rect: (nullable): the source rectangle of the sprite, or %NULL to use the entire paintable
 *
 * Creates a new [class@VectorSprite] with the given size, scale factor,
 * and area of the source paintable.
 *
 * Since: 1.1
 */
ShumateVectorSprite *
shumate_vector_sprite_new_full (GdkPaintable *source_paintable,
                                int           width,
                                int           height,
                                double        scale_factor,
                                GdkRectangle *source_rect)
{
  return g_object_new (SHUMATE_TYPE_VECTOR_SPRITE,
                       "source-paintable", source_paintable,
                       "width", width,
                       "height", height,
                       "scale-factor", scale_factor,
                       "source-rect", source_rect,
                       NULL);
}

static void
shumate_vector_sprite_finalize (GObject *object)
{
  ShumateVectorSprite *self = SHUMATE_VECTOR_SPRITE (object);

  g_clear_object (&self->source_paintable);

  G_OBJECT_CLASS (shumate_vector_sprite_parent_class)->finalize (object);
}

static void
shumate_vector_sprite_get_property (GObject    *object,
                                    unsigned    prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  ShumateVectorSprite *self = SHUMATE_VECTOR_SPRITE (object);

  switch (prop_id)
    {
    case PROP_WIDTH:
      g_value_set_int (value, self->width);
      break;
    case PROP_HEIGHT:
      g_value_set_int (value, self->height);
      break;
    case PROP_SCALE_FACTOR:
      g_value_set_double (value, self->scale_factor);
      break;
    case PROP_SOURCE_PAINTABLE:
      g_value_set_object (value, self->source_paintable);
      break;
    case PROP_SOURCE_RECT:
      if (self->source_rect_set)
        g_value_set_boxed (value, &self->source_rect);
      else
        g_value_set_boxed (value, NULL);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
shumate_vector_sprite_set_property (GObject      *object,
                                    unsigned      prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  ShumateVectorSprite *self = SHUMATE_VECTOR_SPRITE (object);

  switch (prop_id)
    {
    case PROP_WIDTH:
      self->width = g_value_get_int (value);
      break;
    case PROP_HEIGHT:
      self->height = g_value_get_int (value);
      break;
    case PROP_SCALE_FACTOR:
      self->scale_factor = g_value_get_double (value);
      break;
    case PROP_SOURCE_PAINTABLE:
      g_assert (self->source_paintable == NULL);
      self->source_paintable = g_value_dup_object (value);
      break;
    case PROP_SOURCE_RECT:
      {
        GdkRectangle *source_rect = (GdkRectangle *) g_value_get_boxed (value);
        if (source_rect)
          {
            self->source_rect = *source_rect;
            self->source_rect_set = TRUE;
          }
        else
          self->source_rect_set = FALSE;
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static GdkPaintable *
shumate_vector_sprite_get_current_image (GdkPaintable *paintable)
{
  ShumateVectorSprite *self = SHUMATE_VECTOR_SPRITE (paintable);
  g_autoptr(GdkPaintable) source_paintable = gdk_paintable_get_current_image (self->source_paintable);

  return GDK_PAINTABLE (shumate_vector_sprite_new_full (
    source_paintable,
    self->width,
    self->height,
    self->scale_factor,
    self->source_rect_set ? &self->source_rect : NULL
  ));
}

static int
shumate_vector_sprite_get_intrinsic_width (GdkPaintable *paintable)
{
  ShumateVectorSprite *self = SHUMATE_VECTOR_SPRITE (paintable);
  return self->width;
}

static int
shumate_vector_sprite_get_intrinsic_height (GdkPaintable *paintable)
{
  ShumateVectorSprite *self = SHUMATE_VECTOR_SPRITE (paintable);
  return self->height;
}

static void
do_snapshot (GdkPaintable  *paintable,
             GdkSnapshot   *snapshot,
             double         width,
             double         height,
             gboolean       symbolic,
             const GdkRGBA  *colors,
             gsize          n_colors)
{
  ShumateVectorSprite *self = SHUMATE_VECTOR_SPRITE (paintable);

  if (self->source_rect_set)
    {
      double paintable_width = gdk_paintable_get_intrinsic_width (self->source_paintable);
      double paintable_height = gdk_paintable_get_intrinsic_height (self->source_paintable);

      gtk_snapshot_save (snapshot);
      gtk_snapshot_push_clip (snapshot, &GRAPHENE_RECT_INIT (0, 0, width, height));

      gtk_snapshot_translate (snapshot,
                              &GRAPHENE_POINT_INIT (
                                -self->source_rect.x * (width / self->source_rect.width),
                                -self->source_rect.y * (height / self->source_rect.height)
                              ));

      width = paintable_width * (width / self->source_rect.width);
      height = paintable_height * (height / self->source_rect.height);
    }

    if (symbolic && GTK_IS_SYMBOLIC_PAINTABLE (self->source_paintable))
      gtk_symbolic_paintable_snapshot_symbolic (GTK_SYMBOLIC_PAINTABLE (self->source_paintable),
                                                snapshot,
                                                width,
                                                height,
                                                colors,
                                                n_colors);
    else
      gdk_paintable_snapshot (self->source_paintable,
                              snapshot,
                              width,
                              height);

    if (self->source_rect_set)
      {
        gtk_snapshot_pop (snapshot);
        gtk_snapshot_restore (snapshot);
      }
}

static void
shumate_vector_sprite_snapshot_symbolic (GtkSymbolicPaintable *paintable,
                                         GdkSnapshot          *snapshot,
                                         double                width,
                                         double                height,
                                         const GdkRGBA        *colors,
                                         gsize                 n_colors)
{
  do_snapshot (GDK_PAINTABLE (paintable), snapshot, width, height, TRUE, colors, n_colors);
}

static void
shumate_vector_sprite_snapshot (GdkPaintable *paintable,
                                GdkSnapshot  *snapshot,
                                double        width,
                                double        height)
{
  do_snapshot (GDK_PAINTABLE (paintable), snapshot, width, height, FALSE, NULL, 0);
}

static void
shumate_vector_sprite_class_init (ShumateVectorSpriteClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = shumate_vector_sprite_finalize;
  object_class->get_property = shumate_vector_sprite_get_property;
  object_class->set_property = shumate_vector_sprite_set_property;

  /**
   * ShumateVectorSprite:source-paintable:
   *
   * The [iface@Gdk.Paintable] used to draw the sprite.
   *
   * Since: 1.1
   */
  properties[PROP_SOURCE_PAINTABLE] =
    g_param_spec_object ("source-paintable",
                         "source-paintable",
                         "source-paintable",
                         GDK_TYPE_PAINTABLE,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  /**
   * ShumateVectorSprite:width:
   *
   * The width at which the sprite should be drawn, in pixels.
   *
   * Since: 1.1
   */
  properties[PROP_WIDTH] =
    g_param_spec_int ("width",
                      "width",
                      "width",
                      0, G_MAXINT, 0,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  /**
   * ShumateVectorSprite:height:
   *
   * The height at which the sprite should be drawn, in pixels.
   *
   * Since: 1.1
   */
  properties[PROP_HEIGHT] =
    g_param_spec_int ("height",
                      "height",
                      "height",
                      0, G_MAXINT, 0,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  /**
   * ShumateVectorSprite:scale-factor:
   *
   * The intended scale factor of the sprite.
   *
   * Since: 1.1
   */
  properties[PROP_SCALE_FACTOR] =
    g_param_spec_double ("scale-factor",
                         "scale-factor",
                         "scale-factor",
                         1, G_MAXDOUBLE, 1,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  /**
   * ShumateVectorSprite:source-rect:
   *
   * The area of the source rectangle to draw, or %NULL to use the entire paintable.
   *
   * Since: 1.1
   */
  properties[PROP_SOURCE_RECT] =
    g_param_spec_boxed ("source-rect",
                        "source-rect",
                        "source-rect",
                        GDK_TYPE_RECTANGLE,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPERTIES, properties);
}

static void
shumate_vector_sprite_init (ShumateVectorSprite *self)
{
  self->scale_factor = 1;
}

static void
shumate_vector_sprite_paintable_iface_init (GdkPaintableInterface *iface)
{
  iface->get_current_image = shumate_vector_sprite_get_current_image;
  iface->get_intrinsic_width = shumate_vector_sprite_get_intrinsic_width;
  iface->get_intrinsic_height = shumate_vector_sprite_get_intrinsic_height;
  iface->snapshot = shumate_vector_sprite_snapshot;
}

static void
shumate_vector_sprite_symbolic_paintable_iface_init (GtkSymbolicPaintableInterface *iface)
{
  iface->snapshot_symbolic = shumate_vector_sprite_snapshot_symbolic;
}

/**
 * shumate_vector_sprite_get_source_paintable:
 * @self: a [class@VectorSprite]
 *
 * Gets the source [iface@Gdk.Paintable] used to draw the sprite.
 *
 * Note that [class@VectorSprite] also implements [iface@Gdk.Paintable].
 * In most cases, you should draw the sprite rather than the original paintable.
 *
 * Returns: (transfer none): the source paintable
 *
 * Since: 1.1
 */
GdkPaintable *
shumate_vector_sprite_get_source_paintable (ShumateVectorSprite *self)
{
  g_return_val_if_fail (SHUMATE_IS_VECTOR_SPRITE (self), NULL);

  return self->source_paintable;
}

/**
 * shumate_vector_sprite_get_width:
 * @self: a [class@VectorSprite]
 *
 * Gets the width at which the sprite should be drawn.
 *
 * Returns: the sprite's width in pixels
 *
 * Since: 1.1
 */
int
shumate_vector_sprite_get_width (ShumateVectorSprite *self)
{
  g_return_val_if_fail (SHUMATE_IS_VECTOR_SPRITE (self), 0);

  return self->width;
}

/**
 * shumate_vector_sprite_get_height:
 * @self: a [class@VectorSprite]
 *
 * Gets the height at which the sprite should be drawn.
 *
 * Returns: the sprite's height in pixels
 *
 * Since: 1.1
 */
int
shumate_vector_sprite_get_height (ShumateVectorSprite *self)
{
  g_return_val_if_fail (SHUMATE_IS_VECTOR_SPRITE (self), 0);

  return self->height;
}

/**
 * shumate_vector_sprite_get_scale_factor:
 * @self: a [class@VectorSprite]
 *
 * Gets the intended scale factor of the sprite.
 *
 * Returns: the sprite's scale factor
 *
 * Since: 1.1
 */
double
shumate_vector_sprite_get_scale_factor (ShumateVectorSprite *self)
{
  g_return_val_if_fail (SHUMATE_IS_VECTOR_SPRITE (self), 0);

  return self->scale_factor;
}

/**
 * shumate_vector_sprite_get_source_rect:
 * @self: a [class@VectorSprite]
 *
 * Gets the source rectangle of the sprite.
 *
 * Returns: (nullable) (transfer none): the sprite's source rectangle, or %NULL if the entire paintable is used
 *
 * Since: 1.1
 */
GdkRectangle *
shumate_vector_sprite_get_source_rect (ShumateVectorSprite *self)
{
  g_return_val_if_fail (SHUMATE_IS_VECTOR_SPRITE (self), NULL);

  if (self->source_rect_set)
    return &self->source_rect;
  else
    return NULL;
}
