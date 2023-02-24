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


struct _ShumateVectorSymbol
{
  GtkWidget parent_instance;

  ShumateVectorSymbolInfo *symbol_info;

  GArray *glyphs;
  int glyphs_length;
  double line_length;
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
  double width;
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


static void
shumate_vector_symbol_constructed (GObject *object)
{
  ShumateVectorSymbol *self = (ShumateVectorSymbol *)object;
  g_autoptr(PangoAttrList) attrs = pango_attr_list_new ();
  PangoAttribute *attr;

  if (self->symbol_info->text_font != NULL)
    {
      g_autoptr(PangoFontDescription) desc = pango_font_description_from_string (self->symbol_info->text_font);
      attr = pango_attr_font_desc_new (desc);
      pango_attr_list_insert (attrs, attr);
    }

  attr = pango_attr_foreground_new (self->symbol_info->text_color.red * 65535,
                                    self->symbol_info->text_color.green * 65535,
                                    self->symbol_info->text_color.blue * 65535);
  pango_attr_list_insert (attrs, attr);

  attr = pango_attr_foreground_alpha_new (self->symbol_info->text_color.alpha * 65535);
  pango_attr_list_insert (attrs, attr);

  attr = pango_attr_size_new_absolute (self->symbol_info->text_size * PANGO_SCALE);
  pango_attr_list_insert (attrs, attr);

  if (self->symbol_info->line_placement)
    {
      PangoContext *context = gtk_widget_get_pango_context (GTK_WIDGET (self));
      g_autoptr(PangoLayout) layout = pango_layout_new (context);
      g_autoptr(PangoLayoutIter) iter = NULL;
      PangoGlyphItem *current_item;
      int i;

      self->glyphs = g_array_new (FALSE, FALSE, sizeof (Glyph));
      g_array_set_clear_func (self->glyphs, (GDestroyNotify)glyph_clear);

      pango_layout_set_attributes (layout, attrs);
      pango_layout_set_text (layout, self->symbol_info->text, -1);
      iter = pango_layout_get_iter (layout);

      pango_layout_get_size (layout, &self->glyphs_length, NULL);
      self->glyphs_length /= PANGO_SCALE;
      self->line_length = shumate_vector_line_string_length (self->symbol_info->line);

      do {
        current_item = pango_layout_iter_get_run (iter);

        if (current_item == NULL)
          continue;

        for (i = 0; i < current_item->glyphs->num_glyphs; i ++)
          {
            GskRenderNode *node;
            Glyph glyph;
            PangoGlyphString *glyph_string;

            glyph_string = pango_glyph_string_new ();
            pango_glyph_string_set_size (glyph_string, 1);
            glyph_string->glyphs[0] = current_item->glyphs->glyphs[i];
            glyph_string->log_clusters[0] = 0;

            node =
              gsk_text_node_new (current_item->item->analysis.font,
                                 glyph_string,
                                 &self->symbol_info->text_color,
                                 &GRAPHENE_POINT_INIT (0, 0));

            glyph.node = node;
            glyph.width = glyph_string->glyphs[0].geometry.width / (double) PANGO_SCALE;
            g_array_append_vals (self->glyphs, &glyph, 1);

            pango_glyph_string_free (glyph_string);
          }
      } while (pango_layout_iter_next_run (iter));
    }
  else
    {
      GtkWidget *label = gtk_label_new (self->symbol_info->text);
      gtk_label_set_attributes (GTK_LABEL (label), attrs);
      gtk_widget_set_parent (label, GTK_WIDGET (self));
    }

  if (self->symbol_info->cursor != NULL)
    gtk_widget_set_cursor_from_name (GTK_WIDGET (self), self->symbol_info->cursor);

  gtk_accessible_update_property (GTK_ACCESSIBLE (self),
                                  GTK_ACCESSIBLE_PROPERTY_LABEL,
                                  self->symbol_info->text,
                                  -1);

  G_OBJECT_CLASS (shumate_vector_symbol_parent_class)->constructed (object);
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

  if (self->symbol_info->line_placement)
    {
      if (minimum)
        *minimum = 0;
      if (natural)
        *natural = 0;
    }
  else
    GTK_WIDGET_CLASS (shumate_vector_symbol_parent_class)->measure (widget,
                                                                    orientation,
                                                                    for_size,
                                                                    minimum,
                                                                    natural,
                                                                    minimum_baseline,
                                                                    natural_baseline);
}


static void
shumate_vector_symbol_snapshot (GtkWidget   *widget,
                                GtkSnapshot *snapshot)
{
  ShumateVectorSymbol *self = SHUMATE_VECTOR_SYMBOL (widget);

  if (self->symbol_info->line_placement)
    {
      GtkWidget *parent = gtk_widget_get_parent (widget);
      int i;
      ShumateVectorPointIter iter;
      double rotation = 0.0;
      double scale = 512.0;
      double avg_angle;

      if (SHUMATE_IS_VECTOR_SYMBOL_CONTAINER (parent))
        {
          ShumateViewport *viewport = shumate_layer_get_viewport (SHUMATE_LAYER (parent));
          ShumateMapSource *map_source = shumate_vector_symbol_container_get_map_source (SHUMATE_VECTOR_SYMBOL_CONTAINER (parent));
          scale = shumate_map_source_get_tile_size_at_zoom (map_source, shumate_viewport_get_zoom_level (viewport));
          rotation = shumate_viewport_get_rotation (viewport);
        }

      if (self->glyphs_length > self->line_length * scale)
        return;

      gtk_snapshot_save (snapshot);
      gtk_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (gtk_widget_get_allocated_width (widget) / 2,
                                                              gtk_widget_get_allocated_height (widget) / 2));

      gtk_snapshot_rotate (snapshot, rotation * 180 / G_PI);

      shumate_vector_point_iter_init (&iter, self->symbol_info->line);
      shumate_vector_point_iter_advance (&iter, (self->line_length - self->glyphs_length / scale) / 2.0);

      /* If the label is upside down on average, draw it the other way around */
      avg_angle = shumate_vector_point_iter_get_average_angle (&iter, self->glyphs_length / scale);
      avg_angle = positive_mod (avg_angle + rotation, G_PI * 2.0);
      if (avg_angle > G_PI / 2.0 && avg_angle < 3.0 * G_PI / 2.0)
        {
          iter.reversed = TRUE;
          iter.current_point = iter.num_points - 1;
          iter.distance = 0;
          shumate_vector_point_iter_advance (&iter, (self->line_length - self->glyphs_length / scale) / 2.0);
        }

      for (i = 0; i < self->glyphs->len; i ++)
        {
          Glyph *glyph = &((Glyph *)self->glyphs->data)[i];
          ShumateVectorPoint point;

          /* Whitespace has no glyph, but still has a width that needs to be
           * advanced in the point iter */
          if (glyph->node != NULL)
            {
              float average_angle = shumate_vector_point_iter_get_average_angle (&iter, glyph->width / scale);
              shumate_vector_point_iter_advance (&iter, glyph->width / scale / 2.0);
              shumate_vector_point_iter_get_current_point (&iter, &point);
              shumate_vector_point_iter_advance (&iter, glyph->width / scale / 2.0);

              gtk_snapshot_save (snapshot);
              gtk_snapshot_translate (snapshot,
                                      &GRAPHENE_POINT_INIT (
                                        (point.x - self->symbol_info->x) * scale,
                                        (point.y - self->symbol_info->y) * scale
                                      ));
              gtk_snapshot_rotate (snapshot, average_angle * 180 / G_PI);
              gtk_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (-glyph->width / 2.0, self->symbol_info->text_size / 2.0));
              gtk_snapshot_append_node (snapshot, glyph->node);
              gtk_snapshot_restore (snapshot);
            }
          else
            shumate_vector_point_iter_advance (&iter, glyph->width / scale);
        }

      gtk_snapshot_restore (snapshot);
    }
  else
    GTK_WIDGET_CLASS (shumate_vector_symbol_parent_class)->snapshot (widget, snapshot);
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

  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BOX_LAYOUT);
  gtk_widget_class_set_accessible_role (widget_class, GTK_ACCESSIBLE_ROLE_LABEL);
}

static void
on_clicked (ShumateVectorSymbol *self,
            gint                 n_press,
            double               x,
            double               y,
            GtkGestureClick     *click)
{
  g_autoptr(ShumateSymbolEvent) event = shumate_symbol_event_new (self->symbol_info->layer,
                                                                  self->symbol_info->feature_id,
                                                                  self->symbol_info->tags);
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


int
shumate_vector_symbol_get_text_length (ShumateVectorSymbol *self)
{
  if (self->symbol_info->line_placement)
    return self->glyphs_length;
  else
    {
      int natural;
      gtk_widget_measure (GTK_WIDGET (self), GTK_ORIENTATION_HORIZONTAL, 0, NULL, &natural, NULL, NULL);
      return natural;
    }
}

static void
rotate_around_center (float *x,
                      float *y,
                      float  angle)
{
  /* Rotate (x, y) around (0, 0) */

  if (angle == 0)
    return;

  float old_x = *x;
  float old_y = *y;

  *x = cosf (angle) * old_x - sinf (angle) * old_y;
  *y = sinf (angle) * old_x + cosf (angle) * old_y;
}


gboolean
shumate_vector_symbol_calculate_collision (ShumateVectorSymbol    *self,
                                           ShumateVectorCollision *collision,
                                           float                   x,
                                           float                   y,
                                           float                   tile_size_for_zoom,
                                           float                   rotation)
{
  float text_length = shumate_vector_symbol_get_text_length (self);
  float yextent = self->symbol_info->text_size / 2.0;

  if (self->symbol_info->line_placement)
    {
      ShumateVectorPointIter iter;
      float line_length = shumate_vector_line_string_length (self->symbol_info->line);
      float length = text_length / tile_size_for_zoom;

      if (length > line_length)
        return FALSE;

      shumate_vector_point_iter_init (&iter, self->symbol_info->line);
      shumate_vector_point_iter_advance (&iter, (line_length - length) / 2.0);

      do
        {
          ShumateVectorPoint point;
          float xextent = MIN (length, shumate_vector_point_iter_get_segment_length (&iter)) * tile_size_for_zoom / 2;

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
            xextent + self->symbol_info->text_padding,
            yextent + self->symbol_info->text_padding,
            rotation + shumate_vector_point_iter_get_current_angle (&iter)
          );

          if (!check)
            return FALSE;
        }
      while ((length -= shumate_vector_point_iter_next_segment (&iter)) > 0);

      shumate_vector_collision_commit_pending (collision);
      return TRUE;
    }
  else
    {
      gboolean check = shumate_vector_collision_check (
        collision,
        x,
        y,
        text_length / 2.0 + self->symbol_info->text_padding,
        yextent + self->symbol_info->text_padding,
        0
      );

      if (check)
        shumate_vector_collision_commit_pending (collision);

      return check;
    }
}

