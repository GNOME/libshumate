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


#include "shumate-vector-symbol-private.h"
#include "shumate-vector-utils-private.h"
#include "shumate-vector-symbol-info-private.h"
#include "shumate-vector-symbol-container-private.h"
#include "shumate-layer.h"
#include "shumate-symbol-event-private.h"
#include "../shumate-vector-sprite.h"

#define RGBA_BLACK ((GdkRGBA){0, 0, 0, 1})

struct _ShumateVectorSymbol
{
  GtkWidget parent_instance;

  ShumateVectorSymbolInfo *symbol_info;

  GArray *glyphs;

  GskRenderNode *glyphs_node;
  int layout_width, layout_height, baseline, layout_y;

  graphene_rect_t bounds;
  double x, y;

  ShumateVectorPoint midpoint;
  double midpoint_angle;
  double line_length;

  uint8_t show_text : 1;
  uint8_t show_icon : 1;
};

G_DEFINE_TYPE (ShumateVectorSymbol, shumate_vector_symbol, GTK_TYPE_WIDGET)


enum {
  PROP_0,
  PROP_SYMBOL_INFO,
  N_PROPS,
};

static GParamSpec *obj_properties[N_PROPS] = { NULL, };

enum {
  CLICKED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };


typedef struct {
  GskRenderNode *node;
  ShumateVectorSprite *sprite;
  double width;
  GdkRGBA icon_color;
} Glyph;

static void
glyph_clear (Glyph *glyph)
{
  g_clear_pointer (&glyph->node, gsk_render_node_unref);
}


ShumateVectorSymbol *
shumate_vector_symbol_new (ShumateVectorSymbolInfo *symbol_info)
{
  return g_object_new (SHUMATE_TYPE_VECTOR_SYMBOL,
                       "symbol-info", symbol_info,
                       NULL);
}


static PangoAttrShape *
get_shape_from_glyph_item (PangoGlyphItem *item)
{
  for (GSList *list = item->item->analysis.extra_attrs; list != NULL; list = list->next)
    if (((PangoAttribute*)list->data)->klass->type == PANGO_ATTR_SHAPE)
      return (PangoAttrShape*)list->data;
  return NULL;
}

static void
get_color_from_glyph_item (PangoGlyphItem *item, GdkRGBA *color)
{
  *color = RGBA_BLACK;

  for (GSList *list = item->item->analysis.extra_attrs; list != NULL; list = list->next)
    {
      if (((PangoAttribute*)list->data)->klass->type == PANGO_ATTR_FOREGROUND)
        {
          PangoColor c = ((PangoAttrColor*)list->data)->color;
          color->red = c.red / 65535.0;
          color->green = c.green / 65535.0;
          color->blue = c.blue / 65535.0;
        }
      else if (((PangoAttribute*)list->data)->klass->type == PANGO_ATTR_FOREGROUND_ALPHA)
        color->alpha = ((PangoAttrInt*)list->data)->value / 65535.0;
    }
}


static void
shumate_vector_symbol_constructed (GObject *object)
{
  ShumateVectorSymbol *self = (ShumateVectorSymbol *)object;
  g_autofree char *text = NULL;

  if (self->symbol_info->details->formatted_text != NULL)
    {
      PangoContext *context = gtk_widget_get_pango_context (GTK_WIDGET (self));
      g_autoptr(PangoLayout) layout = pango_layout_new (context);
      g_autoptr(PangoAttrList) attrs = pango_attr_list_new ();
      g_autoptr(GString) string = g_string_new ("");
      PangoAttribute *attr;
      gboolean any_images = FALSE;
      PangoRectangle ink_rect;

      if (self->symbol_info->details->text_font != NULL)
        {
          g_autoptr(PangoFontDescription) desc = pango_font_description_from_string (self->symbol_info->details->text_font);
          attr = pango_attr_font_desc_new (desc);
          pango_attr_list_insert (attrs, attr);
        }

      attr = pango_attr_letter_spacing_new (self->symbol_info->details->text_letter_spacing * self->symbol_info->details->text_size * PANGO_SCALE);
      pango_attr_list_insert (attrs, attr);

      attr = pango_attr_foreground_new (self->symbol_info->details->text_color.red * 65535,
                                        self->symbol_info->details->text_color.green * 65535,
                                        self->symbol_info->details->text_color.blue * 65535);
      pango_attr_list_insert (attrs, attr);

      attr = pango_attr_foreground_alpha_new (self->symbol_info->details->text_color.alpha * 65535);
      pango_attr_list_insert (attrs, attr);

      attr = pango_attr_size_new_absolute (self->symbol_info->details->text_size * PANGO_SCALE);
      pango_attr_list_insert (attrs, attr);

      pango_layout_set_attributes (layout, attrs);

      for (int i = 0, n = self->symbol_info->details->formatted_text->len; i < n; i ++)
        {
          ShumateVectorFormatPart *part = g_ptr_array_index (self->symbol_info->details->formatted_text, i);

          if (part->sprite != NULL)
            {
              int width = shumate_vector_sprite_get_width (part->sprite);
              int height = shumate_vector_sprite_get_height (part->sprite);
              /* For shapes, since we're overriding the ink and logical rects
                 of the glyph, we have to take letter spacing into account ourselves */
              double spacing = self->symbol_info->details->text_letter_spacing * self->symbol_info->details->text_size;

              PangoRectangle ink_rect = {
                .x = spacing / 2 * PANGO_SCALE,
                .y = -height * PANGO_SCALE,
                .width = width * PANGO_SCALE,
                .height = height * PANGO_SCALE,
              };
              PangoRectangle logical_rect = {
                .x = 0,
                .y = -height * PANGO_SCALE,
                .width = (width + spacing) * PANGO_SCALE,
                .height = height * PANGO_SCALE,
              };

              attr = pango_attr_shape_new_with_data (&ink_rect, &logical_rect, g_object_ref (part->sprite), (PangoAttrDataCopyFunc)g_object_ref, g_object_unref);
              attr->start_index = string->len;
              attr->end_index = string->len + strlen ("\uFFFC");
              pango_attr_list_insert (attrs, attr);

              if (spacing != 0)
                {
                  attr = pango_attr_letter_spacing_new (0);
                  attr->start_index = string->len;
                  attr->end_index = string->len + strlen ("\uFFFC");
                  pango_attr_list_insert (attrs, attr);
                }

              g_string_append (string, "\uFFFC");

              any_images = TRUE;
            }
          else
            {
              if (part->has_font_scale)
                {
                  attr = pango_attr_size_new_absolute (part->font_scale * self->symbol_info->details->text_size * PANGO_SCALE);
                  attr->start_index = string->len;
                  attr->end_index = string->len + strlen (part->string);
                  pango_attr_list_insert (attrs, attr);
                }

              if (part->has_text_color)
                {
                  attr = pango_attr_foreground_new (part->text_color.red * 65535,
                                                    part->text_color.green * 65535,
                                                    part->text_color.blue * 65535);
                  attr->start_index = string->len;
                  attr->end_index = string->len + strlen (part->string);
                  pango_attr_list_insert (attrs, attr);

                  attr = pango_attr_foreground_alpha_new (part->text_color.alpha * 65535);
                  attr->start_index = string->len;
                  attr->end_index = string->len + strlen (part->string);
                  pango_attr_list_insert (attrs, attr);
                }

              g_string_append (string, part->string);
            }
        }

      text = g_string_free (g_steal_pointer (&string), FALSE);
      pango_layout_set_text (layout, text, -1);

      pango_layout_get_pixel_extents (layout, &ink_rect, NULL);
      self->layout_width = ink_rect.width;
      self->layout_height = ink_rect.height;
      self->layout_y = ink_rect.y;
      self->baseline = pango_layout_get_baseline (layout) / (double) PANGO_SCALE;

      if ((self->symbol_info->details->text_rotation_alignment == SHUMATE_VECTOR_ALIGNMENT_MAP
          || self->symbol_info->details->text_rotation_alignment == SHUMATE_VECTOR_ALIGNMENT_VIEWPORT_GLYPH)
          && (self->symbol_info->details->symbol_placement == SHUMATE_VECTOR_PLACEMENT_LINE
              || self->symbol_info->details->symbol_placement == SHUMATE_VECTOR_PLACEMENT_LINE_CENTER))
        {
          g_autoptr(PangoLayoutIter) iter = NULL;
          int i;
          PangoGlyphItem *current_item;

          self->glyphs = g_array_new (FALSE, FALSE, sizeof (Glyph));
          g_array_set_clear_func (self->glyphs, (GDestroyNotify)glyph_clear);

          iter = pango_layout_get_iter (layout);

          do {
            PangoAttrShape *shape = NULL;

            current_item = pango_layout_iter_get_run (iter);

            if (current_item == NULL)
              continue;

            shape = get_shape_from_glyph_item (current_item);

            if (shape != NULL)
              {
                Glyph glyph = {
                  .sprite = g_object_ref (shape->data),
                  .node = NULL,
                  .width = shape->logical_rect.width / (double) PANGO_SCALE,
                };
                get_color_from_glyph_item (current_item, &glyph.icon_color);
                g_array_append_vals (self->glyphs, &glyph, 1);
              }
            else
              {
                for (i = 0; i < current_item->glyphs->num_glyphs; i ++)
                  {
                    GskRenderNode *node;
                    Glyph glyph;
                    PangoGlyphString *glyph_string;
                    GdkRGBA color;

                    glyph_string = pango_glyph_string_new ();
                    pango_glyph_string_set_size (glyph_string, 1);
                    glyph_string->glyphs[0] = current_item->glyphs->glyphs[i];
                    glyph_string->log_clusters[0] = 0;

                    get_color_from_glyph_item (current_item, &color);

                    node =
                      gsk_text_node_new (current_item->item->analysis.font,
                                         glyph_string,
                                         &color,
                                         &GRAPHENE_POINT_INIT (0, 0));

                    glyph.node = node;
                    glyph.sprite = NULL;
                    glyph.width = glyph_string->glyphs[0].geometry.width / (double) PANGO_SCALE;
                    g_array_append_vals (self->glyphs, &glyph, 1);

                    pango_glyph_string_free (glyph_string);
                  }
              }
          } while (pango_layout_iter_next_run (iter));
        }
      else
        {
          g_autoptr(GtkSnapshot) snapshot = gtk_snapshot_new ();

          gtk_snapshot_append_layout (snapshot, layout, &RGBA_BLACK);

          if (any_images)
            {
              g_autoptr(PangoLayoutIter) iter = NULL;

              iter = pango_layout_get_iter (layout);

              do {
                PangoGlyphItem *current_item = pango_layout_iter_get_run (iter);
                PangoAttrShape *shape;

                if (current_item == NULL)
                  continue;

                shape = get_shape_from_glyph_item (current_item);
                if (shape != NULL)
                  {
                    ShumateVectorSprite *sprite = shape->data;
                    PangoRectangle extents;
                    GdkRGBA color;

                    pango_layout_iter_get_run_extents (iter, &extents, NULL);
                    get_color_from_glyph_item (current_item, &color);

                    gtk_snapshot_save (snapshot);
                    gtk_snapshot_translate (snapshot,
                                            &GRAPHENE_POINT_INIT (
                                              PANGO_PIXELS (extents.x),
                                              PANGO_PIXELS (extents.y)
                                            ));
                    gtk_symbolic_paintable_snapshot_symbolic (
                      GTK_SYMBOLIC_PAINTABLE (sprite),
                      snapshot,
                      PANGO_PIXELS (extents.width),
                      PANGO_PIXELS (extents.height),
                      &color,
                      1
                    );
                    gtk_snapshot_restore (snapshot);
                  }
              } while (pango_layout_iter_next_run (iter));
            }

          self->glyphs_node = gtk_snapshot_free_to_node (g_steal_pointer (&snapshot));
        }
    }

  if (self->symbol_info->details->cursor != NULL)
    gtk_widget_set_cursor_from_name (GTK_WIDGET (self), self->symbol_info->details->cursor);

  if (self->symbol_info->line != NULL)
    {
      ShumateVectorPointIter iter;

      shumate_vector_point_iter_init (&iter, self->symbol_info->line);
      shumate_vector_point_iter_advance (&iter, self->symbol_info->line_position);

      shumate_vector_point_iter_get_current_point (&iter, &self->midpoint);
      self->midpoint.x -= self->symbol_info->x;
      self->midpoint.y -= self->symbol_info->y;

      self->midpoint_angle = shumate_vector_point_iter_get_current_angle (&iter);

      self->line_length = shumate_vector_line_string_length (self->symbol_info->line);
    }

  gtk_accessible_update_property (GTK_ACCESSIBLE (self),
                                  GTK_ACCESSIBLE_PROPERTY_LABEL,
                                  text,
                                  -1);

  G_OBJECT_CLASS (shumate_vector_symbol_parent_class)->constructed (object);
}


static gboolean
shumate_vector_symbol_contains (GtkWidget *widget,
                                double     x,
                                double     y)
{
  GtkWidget *parent;
  graphene_rect_t alloc;

  if (x < 0 || y < 0)
    return FALSE;

  parent = gtk_widget_get_parent (widget);
  if (!gtk_widget_compute_bounds (widget, parent, &alloc))
    return FALSE;

  if (x > alloc.size.width || y > alloc.size.height)
    return FALSE;

  if (SHUMATE_IS_VECTOR_SYMBOL_CONTAINER (parent))
    {
      ShumateVectorCollision *collision =
        shumate_vector_symbol_container_get_collision ((ShumateVectorSymbolContainer *) (parent));

      return shumate_vector_collision_query_point (collision,
                                                   alloc.origin.x + x + collision->delta_x,
                                                   alloc.origin.y + y + collision->delta_y,
                                                   widget);
    }

  return TRUE;
}


static void
shumate_vector_symbol_dispose (GObject *object)
{
  ShumateVectorSymbol *self = (ShumateVectorSymbol *)object;
  GtkWidget *child;

  while ((child = gtk_widget_get_first_child (GTK_WIDGET (object))))
    gtk_widget_unparent (child);

  g_clear_pointer (&self->symbol_info, shumate_vector_symbol_info_unref);
  g_clear_pointer (&self->glyphs, g_array_unref);
  g_clear_pointer (&self->glyphs_node, gsk_render_node_unref);

  G_OBJECT_CLASS (shumate_vector_symbol_parent_class)->dispose (object);
}


static void
shumate_vector_symbol_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  ShumateVectorSymbol *self = (ShumateVectorSymbol *)object;

  switch (property_id)
    {
    case PROP_SYMBOL_INFO:
      g_value_set_boxed (value, self->symbol_info);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}


static void
shumate_vector_symbol_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  ShumateVectorSymbol *self = SHUMATE_VECTOR_SYMBOL (object);

  switch (property_id)
    {
    case PROP_SYMBOL_INFO:
      g_assert (self->symbol_info == NULL);
      self->symbol_info = g_value_dup_boxed (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}


static double
positive_mod (double i, double n)
{
  return fmod (fmod (i, n) + n, n);
}


static void
shumate_vector_symbol_measure (GtkWidget      *widget,
                               GtkOrientation  orientation,
                               int             for_size,
                               int            *minimum,
                               int            *natural,
                               int            *minimum_baseline,
                               int            *natural_baseline)
{
  ShumateVectorSymbol *self = SHUMATE_VECTOR_SYMBOL (widget);

  if (self->symbol_info->line != NULL)
    {
      if (minimum)
        *minimum = 0;
      if (natural)
        *natural = 0;
    }
  else
    {
      if (orientation == GTK_ORIENTATION_HORIZONTAL)
        {
          if (minimum)
            *minimum = self->layout_width;
          if (natural)
            *natural = self->layout_width;
        }
      else if (orientation == GTK_ORIENTATION_VERTICAL)
        {
          if (minimum)
            *minimum = self->layout_height;
          if (natural)
            *natural = self->layout_height;
        }
    }
}

static GtkSizeRequestMode
shumate_vector_symbol_get_request_mode (GtkWidget *widget)
{
  return GTK_SIZE_REQUEST_CONSTANT_SIZE;
}


static void
add_anchor_offset (ShumateVectorAnchor anchor,
                   double *offset_x,
                   double *offset_y,
                   double width,
                   double height)
{
  if (anchor == SHUMATE_VECTOR_ANCHOR_LEFT
      || anchor == SHUMATE_VECTOR_ANCHOR_TOP_LEFT
      || anchor == SHUMATE_VECTOR_ANCHOR_BOTTOM_LEFT)
    *offset_x += width / 2.0;

  if (anchor == SHUMATE_VECTOR_ANCHOR_RIGHT
      || anchor == SHUMATE_VECTOR_ANCHOR_TOP_RIGHT
      || anchor == SHUMATE_VECTOR_ANCHOR_BOTTOM_RIGHT)
    *offset_x -= width / 2.0;

  if (anchor == SHUMATE_VECTOR_ANCHOR_TOP
      || anchor == SHUMATE_VECTOR_ANCHOR_TOP_LEFT
      || anchor == SHUMATE_VECTOR_ANCHOR_TOP_RIGHT)
    *offset_y += height / 2.0;

  if (anchor == SHUMATE_VECTOR_ANCHOR_BOTTOM
      || anchor == SHUMATE_VECTOR_ANCHOR_BOTTOM_LEFT
      || anchor == SHUMATE_VECTOR_ANCHOR_BOTTOM_RIGHT)
    *offset_y -= height / 2.0;
}


static void
shumate_vector_symbol_snapshot (GtkWidget   *widget,
                                GtkSnapshot *snapshot)
{
  ShumateVectorSymbol *self = SHUMATE_VECTOR_SYMBOL (widget);
  double rotation = 0.0;
  double tile_size_for_zoom = 512.0;
  GtkWidget *parent = gtk_widget_get_parent (widget);
  ShumateVectorPointIter iter;

  if (SHUMATE_IS_VECTOR_SYMBOL_CONTAINER (parent))
    {
      ShumateViewport *viewport = shumate_layer_get_viewport (SHUMATE_LAYER (parent));
      ShumateMapSource *map_source = shumate_vector_symbol_container_get_map_source (SHUMATE_VECTOR_SYMBOL_CONTAINER (parent));
      double zoom_level = shumate_viewport_get_zoom_level (viewport);
      tile_size_for_zoom = shumate_map_source_get_tile_size (map_source) * pow (2, zoom_level - self->symbol_info->details->tile_zoom_level);
      rotation = shumate_viewport_get_rotation (viewport);
    }

  gtk_snapshot_save (snapshot);

  /* Translate so the origin is at the origin point of the symbol */
  gtk_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (self->x - self->bounds.origin.x,
                                                          self->y - self->bounds.origin.y));

  if (self->show_icon && self->symbol_info->details->icon_image && self->symbol_info->details->icon_opacity > 0.0)
    {
      double angle = 0;
      double icon_width = shumate_vector_sprite_get_width (self->symbol_info->details->icon_image)
                          * self->symbol_info->details->icon_size;
      double icon_height = shumate_vector_sprite_get_height (self->symbol_info->details->icon_image)
                           * self->symbol_info->details->icon_size;

      double offset_x = self->symbol_info->details->icon_offset_x * self->symbol_info->details->icon_size;
      double offset_y = self->symbol_info->details->icon_offset_y * self->symbol_info->details->icon_size;

      add_anchor_offset (self->symbol_info->details->icon_anchor,
                         &offset_x, &offset_y,
                         icon_width, icon_height);

      if (self->symbol_info->details->icon_rotation_alignment == SHUMATE_VECTOR_ALIGNMENT_MAP)
        angle = self->midpoint_angle;
      else
        angle = -rotation;

      angle += self->symbol_info->details->icon_rotate;

      gtk_snapshot_save (snapshot);

      gtk_snapshot_rotate (snapshot, rotation * 180 / G_PI);
      gtk_snapshot_translate (snapshot,
                              &GRAPHENE_POINT_INIT (
                                self->midpoint.x * tile_size_for_zoom,
                                self->midpoint.y * tile_size_for_zoom
                              ));
      gtk_snapshot_rotate (snapshot, angle * 180 / G_PI);
      gtk_snapshot_translate (snapshot,
                              &GRAPHENE_POINT_INIT (
                                -icon_width / 2.0 + offset_x,
                                -icon_height / 2.0 + offset_y
                              ));

      if (self->symbol_info->details->icon_opacity < 1.0)
        gtk_snapshot_push_opacity (snapshot, self->symbol_info->details->icon_opacity);

      gtk_symbolic_paintable_snapshot_symbolic (
        GTK_SYMBOLIC_PAINTABLE (self->symbol_info->details->icon_image),
        snapshot,
        icon_width,
        icon_height,
        &self->symbol_info->details->icon_color,
        1
      );

      if (self->symbol_info->details->icon_opacity < 1.0)
        gtk_snapshot_pop (snapshot);

      gtk_snapshot_restore (snapshot);
    }

  if (self->show_text && self->glyphs && self->symbol_info->details->text_opacity > 0.0)
    {
      double length = self->layout_width / tile_size_for_zoom;
      double start_pos = MAX (0, self->symbol_info->line_position - (length / 2.0));
      double avg_angle;

      shumate_vector_point_iter_init (&iter, self->symbol_info->line);
      shumate_vector_point_iter_advance (&iter, start_pos);

      /* If the label is upside down on average, draw it the other way around */
      if (self->symbol_info->details->text_keep_upright)
        {
          avg_angle = shumate_vector_point_iter_get_average_angle (&iter, self->layout_width / tile_size_for_zoom);
          avg_angle = positive_mod (avg_angle + rotation, G_PI * 2.0);
          if (avg_angle > G_PI / 2.0 && avg_angle < 3.0 * G_PI / 2.0)
            {
              iter.reversed = TRUE;
              iter.current_point = iter.num_points - 1;
              iter.distance = 0;
              shumate_vector_point_iter_advance (&iter, self->line_length - start_pos - (self->layout_width / tile_size_for_zoom));
            }
        }

      gtk_snapshot_save (snapshot);
      gtk_snapshot_rotate (snapshot, rotation * 180 / G_PI);

      if (self->symbol_info->details->text_opacity < 1.0)
        gtk_snapshot_push_opacity (snapshot, self->symbol_info->details->text_opacity);

      for (int i = 0; i < self->glyphs->len; i ++)
        {
          Glyph *glyph = &((Glyph *)self->glyphs->data)[i];
          ShumateVectorPoint point;

          /* Whitespace has no glyph, but still has a width that needs to be
           * advanced in the point iter */
          if (glyph->node != NULL || glyph->sprite != NULL)
            {
              double angle;

              if (self->symbol_info->details->text_rotation_alignment == SHUMATE_VECTOR_ALIGNMENT_VIEWPORT_GLYPH)
                angle = -rotation;
              else
                angle = shumate_vector_point_iter_get_average_angle (&iter, glyph->width / tile_size_for_zoom);

              shumate_vector_point_iter_advance (&iter, glyph->width / tile_size_for_zoom / 2.0);
              shumate_vector_point_iter_get_current_point (&iter, &point);
              shumate_vector_point_iter_advance (&iter, glyph->width / tile_size_for_zoom / 2.0);

              gtk_snapshot_save (snapshot);
              gtk_snapshot_translate (snapshot,
                                      &GRAPHENE_POINT_INIT (
                                        (point.x - self->symbol_info->x) * tile_size_for_zoom,
                                        (point.y - self->symbol_info->y) * tile_size_for_zoom
                                      ));
              gtk_snapshot_rotate (snapshot, angle * 180 / G_PI);
              gtk_snapshot_translate (snapshot,
                                      &GRAPHENE_POINT_INIT (-glyph->width / 2.0,
                                                            self->baseline - self->layout_y - self->layout_height / 2.0));

              if (glyph->node != NULL)
                gtk_snapshot_append_node (snapshot, glyph->node);
              else
                {
                  int width = shumate_vector_sprite_get_width (glyph->sprite);
                  int height = shumate_vector_sprite_get_height (glyph->sprite);
                  gtk_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (0, -height));
                  gtk_symbolic_paintable_snapshot_symbolic (
                    GTK_SYMBOLIC_PAINTABLE (glyph->sprite),
                    snapshot,
                    width,
                    height,
                    &glyph->icon_color,
                    1
                  );
                }

              gtk_snapshot_restore (snapshot);
            }
          else
            shumate_vector_point_iter_advance (&iter, glyph->width / tile_size_for_zoom);
        }

      if (self->symbol_info->details->text_opacity < 1.0)
        gtk_snapshot_pop (snapshot);

      gtk_snapshot_restore (snapshot);
    }
  else if (self->show_text && self->glyphs_node && self->symbol_info->details->text_opacity > 0.0)
    {
      double angle = 0;
      double offset_x = self->symbol_info->details->text_offset_x * self->symbol_info->details->text_size;
      double offset_y = self->symbol_info->details->text_offset_y * self->symbol_info->details->text_size;

      add_anchor_offset (self->symbol_info->details->text_anchor,
                         &offset_x, &offset_y,
                         self->layout_width, self->layout_height);

      if (self->symbol_info->details->text_rotation_alignment != SHUMATE_VECTOR_ALIGNMENT_MAP)
        angle = -rotation;

      gtk_snapshot_save (snapshot);

      if (self->symbol_info->details->text_opacity < 1)
        gtk_snapshot_push_opacity (snapshot, self->symbol_info->details->text_opacity);

      gtk_snapshot_rotate (snapshot, rotation * 180 / G_PI);
      gtk_snapshot_translate (snapshot,
                              &GRAPHENE_POINT_INIT (
                                self->midpoint.x * tile_size_for_zoom,
                                self->midpoint.y * tile_size_for_zoom
                              ));
      gtk_snapshot_rotate (snapshot, angle * 180 / G_PI);

      gtk_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (
                                          -self->layout_width / 2.0 + offset_x,
                                          -self->layout_y - self->layout_height / 2.0 + offset_y
                                        ));
      gtk_snapshot_append_node (snapshot, self->glyphs_node);

      if (self->symbol_info->details->text_opacity < 1)
        gtk_snapshot_pop (snapshot);

      gtk_snapshot_restore (snapshot);
    }

  gtk_snapshot_restore (snapshot);
}


static void
shumate_vector_symbol_class_init (ShumateVectorSymbolClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = shumate_vector_symbol_constructed;
  object_class->dispose = shumate_vector_symbol_dispose;
  object_class->get_property = shumate_vector_symbol_get_property;
  object_class->set_property = shumate_vector_symbol_set_property;

  widget_class->contains = shumate_vector_symbol_contains;
  widget_class->get_request_mode = shumate_vector_symbol_get_request_mode;
  widget_class->snapshot = shumate_vector_symbol_snapshot;
  widget_class->measure = shumate_vector_symbol_measure;

  obj_properties[PROP_SYMBOL_INFO] =
    g_param_spec_boxed ("symbol-info",
                        "Symbol info",
                        "Symbol info",
                        SHUMATE_TYPE_VECTOR_SYMBOL_INFO,
                        G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, N_PROPS, obj_properties);

  signals[CLICKED] =
    g_signal_new ("clicked",
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE,
                  1,
                  SHUMATE_TYPE_SYMBOL_EVENT);

  gtk_widget_class_set_accessible_role (widget_class, GTK_ACCESSIBLE_ROLE_LABEL);
}

static void
on_clicked (ShumateVectorSymbol *self,
            gint                 n_press,
            double               x,
            double               y,
            GtkGestureClick     *click)
{
  g_autoptr(ShumateSymbolEvent) event =
    shumate_symbol_event_new_with_n_press (self->symbol_info->details->layer,
                                           self->symbol_info->details->source_layer,
                                           self->symbol_info->details->feature_id,
                                           self->symbol_info->details->tags,
                                           n_press);

  g_signal_emit (self, signals[CLICKED], 0, event);
}

static void
shumate_vector_symbol_init (ShumateVectorSymbol *self)
{
  GtkGesture *click = gtk_gesture_click_new ();
  g_signal_connect_object (click, "released", G_CALLBACK (on_clicked), self, G_CONNECT_SWAPPED);
  gtk_widget_add_controller (GTK_WIDGET (self), GTK_EVENT_CONTROLLER (click));
}


ShumateVectorSymbolInfo *
shumate_vector_symbol_get_symbol_info (ShumateVectorSymbol *self)
{
  g_return_val_if_fail (SHUMATE_IS_VECTOR_SYMBOL (self), NULL);

  return self->symbol_info;
}

static void
rotate_around_center (double *x,
                      double *y,
                      double  angle)
{
  /* Rotate (x, y) around (0, 0) */

  if (angle == 0)
    return;

  double old_x = *x;
  double old_y = *y;

  *x = cosf (angle) * old_x - sinf (angle) * old_y;
  *y = sinf (angle) * old_x + cosf (angle) * old_y;
}


gboolean
shumate_vector_symbol_calculate_collision (ShumateVectorSymbol    *self,
                                           ShumateVectorCollision *collision,
                                           double                  x,
                                           double                  y,
                                           double                  tile_size_for_zoom,
                                           double                  rotation,
                                           graphene_rect_t        *bounds_out)
{
  double yextent = self->symbol_info->details->text_size / 2.0;
  gboolean check;
  ShumateVectorPointIter iter;
  ShumateVectorPoint point;
  ShumateVectorPoint midpoint = {0};
  int save = 0;

  shumate_vector_collision_rollback_pending (collision, 0);

  if (self->symbol_info->line != NULL)
    {
      midpoint = self->midpoint;

      rotate_around_center (&midpoint.x, &midpoint.y, rotation);
      midpoint.x *= tile_size_for_zoom;
      midpoint.y *= tile_size_for_zoom;
    }

  if (self->glyphs != NULL)
    {
      double line_length = self->symbol_info->line_length;
      double length = self->layout_width / tile_size_for_zoom;
      double start_pos = MAX (0, self->symbol_info->line_position - (length / 2.0));

      g_assert (self->symbol_info->line != NULL);

      if (length > line_length - start_pos)
        {
          self->show_text = FALSE;
          return FALSE;
        }

      shumate_vector_point_iter_init (&iter, self->symbol_info->line);
      shumate_vector_point_iter_advance (&iter, start_pos);

      self->show_text = TRUE;

      do
        {
          double xextent = MIN (length, shumate_vector_point_iter_get_segment_length (&iter) - iter.distance) * tile_size_for_zoom / 2;

          if (shumate_vector_point_iter_is_at_end (&iter))
            return FALSE;

          shumate_vector_point_iter_get_segment_center (&iter, length, &point);
          point.x -= self->symbol_info->x;
          point.y -= self->symbol_info->y;

          rotate_around_center (&point.x, &point.y, rotation);
          point.x *= tile_size_for_zoom;
          point.y *= tile_size_for_zoom;

          gboolean check = shumate_vector_collision_check (
            collision,
            x + point.x,
            y + point.y,
            xextent + self->symbol_info->details->text_padding,
            yextent + self->symbol_info->details->text_padding,
            rotation + shumate_vector_point_iter_get_current_angle (&iter),
            self->symbol_info->details->text_overlap,
            self->symbol_info->details->text_ignore_placement,
            self
          );

          if (!check)
            {
              if (self->symbol_info->details->text_optional)
                {
                  shumate_vector_collision_rollback_pending (collision, save);
                  self->show_text = FALSE;
                  break;
                }
              else
                return FALSE;
            }
        }
      while ((length -= shumate_vector_point_iter_next_segment (&iter)) > 0);
    }
  else if (self->glyphs_node != NULL)
    {
      ShumateVectorAnchor anchor = self->symbol_info->details->text_anchor;
      double angle = 0;
      double offset_x = self->symbol_info->details->text_offset_x * self->symbol_info->details->text_size;
      double offset_y = self->symbol_info->details->text_offset_y * self->symbol_info->details->text_size;

      add_anchor_offset (anchor, &offset_x, &offset_y, self->layout_width, self->layout_height);

      if (self->symbol_info->details->text_rotation_alignment == SHUMATE_VECTOR_ALIGNMENT_MAP)
        angle = rotation;

      rotate_around_center (&offset_x, &offset_y, angle);

      self->show_text = TRUE;

      check = shumate_vector_collision_check (
        collision,
        x + midpoint.x + offset_x,
        y + midpoint.y + offset_y,
        self->layout_width / 2.0 + self->symbol_info->details->text_padding,
        yextent + self->symbol_info->details->text_padding,
        angle,
        self->symbol_info->details->text_overlap,
        self->symbol_info->details->text_ignore_placement,
        self
      );

      if (!check)
        {
          if (self->symbol_info->details->text_optional)
            {
              shumate_vector_collision_rollback_pending (collision, save);
              self->show_text = FALSE;
            }
          else
            return FALSE;
        }
    }

  save = shumate_vector_collision_save_pending (collision);

  if (self->symbol_info->details->icon_image)
    {
      float angle = 0;
      double icon_width = shumate_vector_sprite_get_width (self->symbol_info->details->icon_image)
                          * self->symbol_info->details->icon_size;
      double icon_height = shumate_vector_sprite_get_height (self->symbol_info->details->icon_image)
                           * self->symbol_info->details->icon_size;

      double offset_x = self->symbol_info->details->icon_offset_x * self->symbol_info->details->icon_size;
      double offset_y = self->symbol_info->details->icon_offset_y * self->symbol_info->details->icon_size;

      self->show_icon = TRUE;

      add_anchor_offset (self->symbol_info->details->icon_anchor, &offset_x, &offset_y, icon_width, icon_height);

      if (self->symbol_info->details->icon_rotation_alignment == SHUMATE_VECTOR_ALIGNMENT_MAP)
        angle = rotation + self->midpoint_angle;
      angle += self->symbol_info->details->icon_rotate;
      offset_x += (self->symbol_info->details->icon_padding_right - self->symbol_info->details->icon_padding_left) / 2.0;
      offset_y += (self->symbol_info->details->icon_padding_bottom - self->symbol_info->details->icon_padding_top) / 2.0;
      rotate_around_center (&offset_x, &offset_y, angle);

      check = shumate_vector_collision_check (
        collision,
        x + midpoint.x + offset_x,
        y + midpoint.y + offset_y,
        (icon_width + self->symbol_info->details->icon_padding_left + self->symbol_info->details->icon_padding_right) / 2 ,
        (icon_height + self->symbol_info->details->icon_padding_top + self->symbol_info->details->icon_padding_bottom) / 2,
        angle,
        self->symbol_info->details->icon_overlap,
        self->symbol_info->details->icon_ignore_placement,
        self
      );

      if (!check)
        {
          if (self->symbol_info->details->icon_optional)
            {
              shumate_vector_collision_rollback_pending (collision, save);
              self->show_icon = FALSE;
            }
          else
            return FALSE;
        }
    }

  if (!self->show_icon && !self->show_text)
    return FALSE;

  shumate_vector_collision_commit_pending (collision, &self->bounds);
  *bounds_out = self->bounds;
  self->x = x;
  self->y = y;
  return TRUE;
}

