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
      PangoGlyphString *glyph_string;
      int i;

      self->glyphs = g_array_new (FALSE, FALSE, sizeof (Glyph));
      g_array_set_clear_func (self->glyphs, (GDestroyNotify)glyph_clear);

      pango_layout_set_attributes (layout, attrs);
      pango_layout_set_text (layout, self->symbol_info->text, -1);
      iter = pango_layout_get_iter (layout);

      pango_layout_get_size (layout, &self->glyphs_length, NULL);
      self->glyphs_length /= PANGO_SCALE;
      self->line_length = shumate_vector_line_string_length (&self->symbol_info->line);

      do {
        current_item = pango_layout_iter_get_run (iter);

        if (current_item == NULL)
          continue;

        for (i = 0; i < current_item->glyphs->num_glyphs; i ++)
          {
            GskRenderNode *node;
            Glyph glyph;

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
          }
      } while (pango_layout_iter_next_run (iter));
    }
  else
    {
      GtkWidget *label = gtk_label_new (self->symbol_info->text);
      gtk_label_set_attributes (GTK_LABEL (label), attrs);
      gtk_widget_set_parent (label, GTK_WIDGET (self));
    }

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

      if (SHUMATE_IS_VECTOR_SYMBOL_CONTAINER (parent))
        {
          ShumateViewport *viewport = shumate_layer_get_viewport (SHUMATE_LAYER (parent));
          ShumateMapSource *map_source = shumate_vector_symbol_container_get_map_source (SHUMATE_VECTOR_SYMBOL_CONTAINER (parent));
          scale = shumate_map_source_get_tile_size_at_zoom (map_source, shumate_viewport_get_zoom_level (viewport));
          rotation = shumate_viewport_get_rotation (viewport);
        }

      if (self->glyphs_length > self->line_length * scale)
        return;

      gtk_snapshot_rotate (snapshot, rotation * 180 / G_PI);

      shumate_vector_point_iter_init (&iter, &self->symbol_info->line);
      shumate_vector_point_iter_advance (&iter, (self->line_length - self->glyphs_length / scale) / 2.0);

      for (i = 0; i < self->glyphs->len; i ++)
        {
          Glyph *glyph = &((Glyph *)self->glyphs->data)[i];
          ShumateVectorPoint point;

          shumate_vector_point_iter_get_current_point (&iter, &point);

          /* Whitespace has no glyph, but still has a width that needs to be
           * advanced in the point iter */
          if (glyph->node != NULL)
            {
              gtk_snapshot_save (snapshot);
              gtk_snapshot_translate (snapshot,
                                      &GRAPHENE_POINT_INIT (
                                        (point.x - self->symbol_info->x) * scale,
                                        (point.y - self->symbol_info->y) * scale
                                      ));
              gtk_snapshot_rotate (snapshot, shumate_vector_point_iter_get_current_angle (&iter) * 180 / G_PI);
              gtk_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (0, self->symbol_info->text_size / 2));
              gtk_snapshot_append_node (snapshot, glyph->node);
              gtk_snapshot_restore (snapshot);
            }

          shumate_vector_point_iter_advance (&iter, glyph->width / scale);
        }
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

  obj_properties[PROP_SYMBOL_INFO] =
    g_param_spec_boxed ("symbol-info",
                        "Symbol info",
                        "Symbol info",
                        SHUMATE_TYPE_VECTOR_SYMBOL_INFO,
                        G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BOX_LAYOUT);
  g_object_class_install_properties (object_class, N_PROPS, obj_properties);
}


static void
shumate_vector_symbol_init (ShumateVectorSymbol *self)
{
}


ShumateVectorSymbolInfo *
shumate_vector_symbol_get_symbol_info (ShumateVectorSymbol *self)
{
  g_return_val_if_fail (SHUMATE_IS_VECTOR_SYMBOL (self), NULL);

  return self->symbol_info;
}
