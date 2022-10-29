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

#include <gtk/gtk.h>
#include "shumate-vector-symbol-info-private.h"
#include "shumate-vector-symbol-layer-private.h"
#include "shumate-vector-expression-private.h"
#include "shumate-vector-utils-private.h"

struct _ShumateVectorSymbolLayer
{
  ShumateVectorLayer parent_instance;

  ShumateVectorExpression *text_field;
  ShumateVectorExpression *text_color;
  ShumateVectorExpression *text_size;
  gboolean line_placement;
  char *text_fonts;
};

G_DEFINE_TYPE (ShumateVectorSymbolLayer, shumate_vector_symbol_layer, SHUMATE_TYPE_VECTOR_LAYER)


ShumateVectorLayer *
shumate_vector_symbol_layer_create_from_json (JsonObject *object, GError **error)
{
  ShumateVectorSymbolLayer *layer = g_object_new (SHUMATE_TYPE_VECTOR_SYMBOL_LAYER, NULL);

  if (json_object_has_member (object, "paint"))
    {
      JsonObject *paint = json_object_get_object_member (object, "paint");

      layer->text_color = shumate_vector_expression_from_json (json_object_get_member (paint, "text-color"), error);
      if (layer->text_color == NULL)
        return NULL;
    }

  if (json_object_has_member (object, "layout"))
    {
      JsonObject *layout = json_object_get_object_member (object, "layout");
      JsonNode *text_font_node;
      JsonArray *text_font;

      layer->text_field = shumate_vector_expression_from_json (json_object_get_member (layout, "text-field"), error);
      if (layer->text_field == NULL)
        return NULL;

      text_font_node = json_object_get_member (layout, "text-font");
      if (text_font_node != NULL)
        {
          g_autoptr(GStrvBuilder) builder = g_strv_builder_new ();
          g_auto(GStrv) fonts = NULL;

          if (!shumate_vector_json_get_array (text_font_node, &text_font, error))
            return NULL;

          for (int i = 0, n = json_array_get_length (text_font); i < n; i ++)
            g_strv_builder_add (builder, json_array_get_string_element (text_font, i));

          fonts = g_strv_builder_end (builder);
          layer->text_fonts = g_strjoinv (", ", fonts);
        }

      layer->line_placement = g_strcmp0 (json_object_get_string_member_with_default (layout, "symbol-placement", NULL), "line") == 0;

      layer->text_size = shumate_vector_expression_from_json (json_object_get_member (layout, "text-size"), error);
      if (layer->text_size == NULL)
        return NULL;
    }

  return (ShumateVectorLayer *)layer;
}


static void
shumate_vector_symbol_layer_finalize (GObject *object)
{
  ShumateVectorSymbolLayer *self = SHUMATE_VECTOR_SYMBOL_LAYER (object);

  g_clear_object (&self->text_field);
  g_clear_object (&self->text_color);
  g_clear_object (&self->text_size);
  g_clear_pointer (&self->text_fonts, g_free);

  G_OBJECT_CLASS (shumate_vector_symbol_layer_parent_class)->finalize (object);
}


static void
shumate_vector_symbol_layer_render (ShumateVectorLayer *layer, ShumateVectorRenderScope *scope)
{
  ShumateVectorSymbolLayer *self = SHUMATE_VECTOR_SYMBOL_LAYER (layer);
  g_autofree char *text_field = NULL;
  GdkRGBA text_color = SHUMATE_VECTOR_COLOR_BLACK;
  double text_size;
  ShumateVectorSymbolInfo *symbol_info;
  double x, y;
  double min_x, min_y, max_x, max_y;

  shumate_vector_render_scope_get_geometry_center (scope, &x, &y);
  if (x < 0 || x >= 1 || y < 0 || y >= 1)
    /* Tiles usually include a bit of margin. Don't include symbols that are
     * covered by a different tile. */
    return;

  shumate_vector_render_scope_get_bounds (scope, &min_x, &min_y, &max_x, &max_y);

  shumate_vector_expression_eval_color (self->text_color, scope, &text_color);
  text_size = shumate_vector_expression_eval_number (self->text_size, scope, 16.0);
  text_field = shumate_vector_expression_eval_string (self->text_field, scope, "");

  if (strlen (text_field) == 0)
    return;

  if (self->line_placement)
    {
      g_autoptr(GPtrArray) lines = shumate_vector_render_scope_get_geometry (scope);
      guint i, j;

      for (i = 0; i < lines->len; i ++)
        {
          g_autoptr(GPtrArray) split_lines = shumate_vector_line_string_simplify ((ShumateVectorLineString *)lines->pdata[i]);

          for (j = 0; j < split_lines->len; j ++)
            {
              ShumateVectorLineString *linestring = (ShumateVectorLineString *)split_lines->pdata[j];

#if 0
              /* visualize line simplification */

              if (linestring->n_points > 0)
                {
                  guint k;
                  float scale = scope->scale * scope->target_size;

                  cairo_set_source_rgb (scope->cr,
                                        rand () % 255 / 255.0,
                                        rand () % 255 / 255.0,
                                        rand () % 255 / 255.0);
                  cairo_set_line_width (scope->cr, scope->scale);

                  cairo_move_to (scope->cr, linestring->points[0].x * scale, linestring->points[0].y * scale);
                  for (k = 1; k < linestring->n_points; k ++)
                    cairo_line_to (scope->cr, linestring->points[k].x * scale, linestring->points[k].y * scale);

                  cairo_stroke (scope->cr);
                }
#endif

              symbol_info = shumate_vector_symbol_info_new (text_field,
                                                            &text_color,
                                                            text_size,
                                                            self->text_fonts,
                                                            TRUE,
                                                            x,
                                                            y);

              shumate_vector_symbol_info_set_line_points (symbol_info,
                                                          linestring);

              g_ptr_array_add (scope->symbols, symbol_info);
            }
        }
    }
  else
    {
      symbol_info = shumate_vector_symbol_info_new (text_field,
                                                    &text_color,
                                                    text_size,
                                                    self->text_fonts,
                                                    FALSE,
                                                    x,
                                                    y);

      g_ptr_array_add (scope->symbols, symbol_info);
    }
}


static void
shumate_vector_symbol_layer_class_init (ShumateVectorSymbolLayerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ShumateVectorLayerClass *layer_class = SHUMATE_VECTOR_LAYER_CLASS (klass);

  object_class->finalize = shumate_vector_symbol_layer_finalize;
  layer_class->render = shumate_vector_symbol_layer_render;
}


static void
shumate_vector_symbol_layer_init (ShumateVectorSymbolLayer *self)
{
}
