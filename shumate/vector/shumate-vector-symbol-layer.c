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
#include "shumate-vector-renderer.h"
#include "shumate-vector-utils-private.h"

struct _ShumateVectorSymbolLayer
{
  ShumateVectorLayer parent_instance;

  ShumateVectorExpression *icon_anchor;
  ShumateVectorExpression *icon_image;
  ShumateVectorExpression *icon_rotation_alignment;
  ShumateVectorExpression *icon_size;
  ShumateVectorExpression *text_anchor;
  ShumateVectorExpression *text_field;
  ShumateVectorExpression *text_color;
  ShumateVectorExpression *text_size;
  ShumateVectorExpression *cursor;
  ShumateVectorExpression *text_padding;
  ShumateVectorExpression *text_keep_upright;
  ShumateVectorExpression *text_rotation_alignment;
  ShumateVectorExpression *symbol_sort_key;
  ShumateVectorExpression *symbol_placement;
  ShumateVectorExpression *symbol_spacing;
  char *text_fonts;
  double icon_offset_x, icon_offset_y;
  double text_offset_x, text_offset_y;
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
      JsonNode *icon_offset_node;
      JsonNode *text_font_node;
      JsonNode *text_offset_node;
      JsonArray *text_font;

      layer->icon_anchor = shumate_vector_expression_from_json (json_object_get_member (layout, "icon-anchor"), error);
      if (layer->icon_anchor == NULL)
        return NULL;

      layer->icon_image = shumate_vector_expression_from_json (json_object_get_member (layout, "icon-image"), error);
      if (layer->icon_image == NULL)
        return NULL;

      icon_offset_node = json_object_get_member (layout, "icon-offset");
      if (icon_offset_node != NULL)
        {
          JsonArray *icon_offset;

          if (!shumate_vector_json_get_array (icon_offset_node, &icon_offset, error))
            return NULL;

          if (json_array_get_length (icon_offset) != 2)
            {
              g_set_error (error,
                           SHUMATE_STYLE_ERROR,
                           SHUMATE_STYLE_ERROR_INVALID_EXPRESSION,
                           "Expected `icon-offset` to be an array of two numbers");
              return NULL;
            }

          layer->icon_offset_x = json_array_get_double_element (icon_offset, 0);
          layer->icon_offset_y = json_array_get_double_element (icon_offset, 1);
        }

      layer->icon_size = shumate_vector_expression_from_json (json_object_get_member (layout, "icon-size"), error);
      if (layer->icon_size == NULL)
        return NULL;

      layer->icon_rotation_alignment = shumate_vector_expression_from_json (json_object_get_member (layout, "icon-rotation-alignment"), error);
      if (layer->icon_rotation_alignment == NULL)
        return NULL;

      layer->text_field = shumate_vector_expression_from_json (json_object_get_member (layout, "text-field"), error);
      if (layer->text_field == NULL)
        return NULL;

      layer->text_anchor = shumate_vector_expression_from_json (json_object_get_member (layout, "text-anchor"), error);
      if (layer->text_anchor == NULL)
        return NULL;

      layer->text_keep_upright = shumate_vector_expression_from_json (json_object_get_member (layout, "text-keep-upright"), error);
      if (layer->text_keep_upright == NULL)
        return NULL;

      text_offset_node = json_object_get_member (layout, "text-offset");
      if (text_offset_node != NULL)
        {
          JsonArray *text_offset;

          if (!shumate_vector_json_get_array (text_offset_node, &text_offset, error))
            return NULL;

          if (json_array_get_length (text_offset) != 2)
            {
              g_set_error (error,
                           SHUMATE_STYLE_ERROR,
                           SHUMATE_STYLE_ERROR_INVALID_EXPRESSION,
                           "Expected `text-offset` to be an array of two numbers");
              return NULL;
            }

          layer->text_offset_x = json_array_get_double_element (text_offset, 0);
          layer->text_offset_y = json_array_get_double_element (text_offset, 1);
        }

      layer->text_rotation_alignment = shumate_vector_expression_from_json (json_object_get_member (layout, "text-rotation-alignment"), error);
      if (layer->text_rotation_alignment == NULL)
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

      layer->symbol_placement = shumate_vector_expression_from_json (json_object_get_member (layout, "symbol-placement"), error);
      if (layer->symbol_placement == NULL)
        return NULL;

      layer->symbol_spacing = shumate_vector_expression_from_json (json_object_get_member (layout, "symbol-spacing"), error);
      if (layer->symbol_spacing == NULL)
        return NULL;

      layer->text_size = shumate_vector_expression_from_json (json_object_get_member (layout, "text-size"), error);
      if (layer->text_size == NULL)
        return NULL;

      layer->text_padding = shumate_vector_expression_from_json (json_object_get_member (layout, "text-padding"), error);
      if (layer->text_padding == NULL)
        return NULL;

      layer->symbol_sort_key = shumate_vector_expression_from_json (json_object_get_member (layout, "symbol-sort-key"), error);
      if (layer->symbol_sort_key == NULL)
        return NULL;
    }

  /* libshumate-specific extensions to the MapLibre style format */
  if (json_object_has_member (object, "metadata"))
    {
      JsonObject *metadata = json_object_get_object_member (object, "metadata");

      /* Specifies the cursor to use when hovering over the symbol. In
       * MapLibre GL JS, you would to this by listening to mouseenter and
       * mouseleave and setting the cursor on the whole map via JS.
       *
       * See gdk_cursor_new_from_name for possible values. */
      layer->cursor = shumate_vector_expression_from_json (json_object_get_member (metadata, "libshumate:cursor"), error);
      if (layer->cursor == NULL)
        return NULL;
    }

  return (ShumateVectorLayer *)layer;
}


static void
shumate_vector_symbol_layer_finalize (GObject *object)
{
  ShumateVectorSymbolLayer *self = SHUMATE_VECTOR_SYMBOL_LAYER (object);

  g_clear_object (&self->icon_anchor);
  g_clear_object (&self->icon_image);
  g_clear_object (&self->icon_rotation_alignment);
  g_clear_object (&self->icon_size);
  g_clear_object (&self->text_field);
  g_clear_object (&self->text_anchor);
  g_clear_object (&self->text_color);
  g_clear_object (&self->text_size);
  g_clear_object (&self->cursor);
  g_clear_object (&self->text_keep_upright);
  g_clear_object (&self->text_padding);
  g_clear_object (&self->text_rotation_alignment);
  g_clear_object (&self->symbol_sort_key);
  g_clear_object (&self->symbol_placement);
  g_clear_object (&self->symbol_spacing);
  g_clear_pointer (&self->text_fonts, g_free);

  G_OBJECT_CLASS (shumate_vector_symbol_layer_parent_class)->finalize (object);
}

static ShumateVectorSymbolInfo *
create_symbol_info (ShumateVectorSymbolDetails *details,
                    double                      x,
                    double                      y)
{
  ShumateVectorSymbolInfo *symbol_info = g_new0 (ShumateVectorSymbolInfo, 1);
  *symbol_info = (ShumateVectorSymbolInfo) {
    .details = shumate_vector_symbol_details_ref (details),
    .x = x,
    .y = y,
    .ref_count = 1,
  };
  return symbol_info;
}


static void
place_point_label (ShumateVectorSymbolDetails *details,
                   double                      x,
                   double                      y,
                   ShumateVectorRenderScope   *scope)
{
  g_ptr_array_add (scope->symbols, create_symbol_info (details, x, y));
}


static void
place_line_label (ShumateVectorSymbolDetails *details,
                  double                      x,
                  double                      y,
                  ShumateVectorRenderScope   *scope)
{
  ShumateVectorSymbolInfo *symbol_info;
  g_autoptr(GPtrArray) lines = shumate_vector_render_scope_get_geometry (scope);
  guint i, j;

  for (i = 0; i < lines->len; i ++)
    {
      g_autoptr(GPtrArray) split_lines = shumate_vector_line_string_simplify ((ShumateVectorLineString *)lines->pdata[i]);

      for (j = 0; j < split_lines->len; j ++)
        {
          g_autoptr(ShumateVectorLineString) linestring = (ShumateVectorLineString *)split_lines->pdata[j];
          double length = shumate_vector_line_string_length (linestring);
          float distance = 0;
          ShumateVectorPointIter iter;
          float spacing = details->symbol_spacing / scope->target_size;

          if (spacing <= 0)
            return;

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

          shumate_vector_point_iter_init (&iter, linestring);

          /* Make the spacing even on both sides */
          shumate_vector_point_iter_advance (&iter, fmod (length, spacing) / 2);
          distance += fmod (length, spacing) / 2;

          while (!shumate_vector_point_iter_is_at_end (&iter))
            {
              ShumateVectorPoint point;
              shumate_vector_point_iter_get_current_point (&iter, &point);

              symbol_info = create_symbol_info (details, x, y);

              shumate_vector_symbol_info_set_line_points (symbol_info,
                                                          shumate_vector_line_string_copy (linestring),
                                                          distance);

              g_ptr_array_add (scope->symbols, symbol_info);

              shumate_vector_point_iter_advance (&iter, spacing);
              distance += spacing;
            }
        }
    }
}


static void
shumate_vector_symbol_layer_render (ShumateVectorLayer *layer, ShumateVectorRenderScope *scope)
{
  ShumateVectorSymbolLayer *self = SHUMATE_VECTOR_SYMBOL_LAYER (layer);
  g_auto(ShumateVectorValue) value = SHUMATE_VECTOR_VALUE_INIT;
  g_autoptr(GPtrArray) text_field = NULL;
  g_autofree char *cursor = NULL;
  g_autofree char *feature_id = NULL;
  ShumateVectorPlacement symbol_placement;
  ShumateVectorAlignment icon_rotation_alignment, text_rotation_alignment;
  g_autoptr(GHashTable) tags = NULL;
  g_autoptr(ShumateVectorSymbolDetails) details = NULL;
  g_autoptr(ShumateVectorSprite) icon_image = shumate_vector_expression_eval_image (self->icon_image, scope);
  ShumateVectorGeometryType geometry_type = shumate_vector_render_scope_get_geometry_type (scope);
  double x, y;

  shumate_vector_expression_eval (self->text_field, scope, &value);
  if (value.type == SHUMATE_VECTOR_VALUE_TYPE_FORMATTED_STRING)
    {
      shumate_vector_value_get_formatted (&value, &text_field);
      if (text_field->len == 0)
        text_field = NULL;
      else
        g_ptr_array_ref (text_field);
    }
  else if (value.type == SHUMATE_VECTOR_VALUE_TYPE_STRING)
    {
      g_autofree char *text = shumate_vector_value_as_string (&value);

      if (strlen (text) > 0)
        {
          text_field = g_ptr_array_new_with_free_func ((GDestroyNotify)shumate_vector_format_part_free);
          ShumateVectorFormatPart *part = g_new0 (ShumateVectorFormatPart, 1);
          part->string = g_steal_pointer (&text);
          g_ptr_array_add (text_field, part);
        }
    }

  if (text_field == NULL && icon_image == NULL)
    return;

  symbol_placement = shumate_vector_expression_eval_placement (self->symbol_placement, scope);
  if (geometry_type == SHUMATE_VECTOR_GEOMETRY_POINT && symbol_placement != SHUMATE_VECTOR_PLACEMENT_POINT)
    return;

  icon_rotation_alignment = shumate_vector_expression_eval_alignment (self->icon_rotation_alignment, scope);
  if (icon_rotation_alignment == SHUMATE_VECTOR_ALIGNMENT_AUTO)
    {
      icon_rotation_alignment = symbol_placement == SHUMATE_VECTOR_PLACEMENT_POINT
        ? SHUMATE_VECTOR_ALIGNMENT_VIEWPORT
        : SHUMATE_VECTOR_ALIGNMENT_MAP;
    }

  text_rotation_alignment = shumate_vector_expression_eval_alignment (self->text_rotation_alignment, scope);
  if (text_rotation_alignment == SHUMATE_VECTOR_ALIGNMENT_AUTO)
    {
      text_rotation_alignment = symbol_placement == SHUMATE_VECTOR_PLACEMENT_POINT
        ? SHUMATE_VECTOR_ALIGNMENT_VIEWPORT
        : SHUMATE_VECTOR_ALIGNMENT_MAP;
    }

  feature_id = g_strdup_printf ("%ld", scope->feature->id);
  cursor = shumate_vector_expression_eval_string (self->cursor, scope, NULL);

  tags = shumate_vector_render_scope_create_tag_table (scope);

  details = g_new0 (ShumateVectorSymbolDetails, 1);
  *details = (ShumateVectorSymbolDetails) {
    .ref_count = 1,
    .layer = g_strdup (shumate_vector_layer_get_id (layer)),
    .source_layer = g_strdup (scope->layer->name),
    .feature_id = g_strdup (feature_id),
    .tags = g_hash_table_ref (tags),

    .icon_anchor = shumate_vector_expression_eval_anchor (self->icon_anchor, scope),
    .icon_image = g_steal_pointer (&icon_image),
    .icon_rotation_alignment = icon_rotation_alignment,
    .icon_size = shumate_vector_expression_eval_number (self->icon_size, scope, 1.0),
    .icon_offset_x = self->icon_offset_x,
    .icon_offset_y = self->icon_offset_y,

    .formatted_text = g_steal_pointer (&text_field),

    .text_anchor = shumate_vector_expression_eval_anchor (self->text_anchor, scope),
    .text_size = shumate_vector_expression_eval_number (self->text_size, scope, 16.0),
    .text_padding = shumate_vector_expression_eval_number (self->text_padding, scope, 2.0),
    .text_font = g_strdup (self->text_fonts),
    .text_keep_upright = shumate_vector_expression_eval_boolean (self->text_keep_upright, scope, TRUE),
    .text_rotation_alignment = text_rotation_alignment,
    .text_offset_x = self->text_offset_x,
    .text_offset_y = self->text_offset_y,

    .symbol_placement = symbol_placement,
    .symbol_spacing = shumate_vector_expression_eval_number (self->symbol_spacing, scope, 250),

    .cursor = g_strdup (cursor),
    .layer_idx = scope->layer_idx,
    .symbol_sort_key = shumate_vector_expression_eval_number (self->symbol_sort_key, scope, 0),
    .tile_x = scope->tile_x,
    .tile_y = scope->tile_y,
    .tile_zoom_level = scope->zoom_level,
  };

  details->text_color = SHUMATE_VECTOR_COLOR_BLACK;
  shumate_vector_expression_eval_color (self->text_color, scope, &details->text_color);

  switch (shumate_vector_render_scope_get_geometry_type (scope))
    {
    case SHUMATE_VECTOR_GEOMETRY_POINT:
        {
          g_autoptr(GPtrArray) geometry = shumate_vector_render_scope_get_geometry (scope);
          g_ptr_array_set_free_func (geometry, (GDestroyNotify)shumate_vector_line_string_free);

          for (int i = 0; i < geometry->len; i ++)
            {
              ShumateVectorLineString *string = g_ptr_array_index (geometry, i);

              for (int j = 0; j < string->n_points; j ++)
                {
                  x = string->points[j].x;
                  y = string->points[j].y;
                  if (x < 0 || x >= 1 || y < 0 || y >= 1)
                    break;
                  place_point_label (details, x, y, scope);
                }
            }
        }

      break;

    case SHUMATE_VECTOR_GEOMETRY_LINESTRING:
    case SHUMATE_VECTOR_GEOMETRY_POLYGON:
      shumate_vector_render_scope_get_geometry_center (scope, &x, &y);
      if (x < 0 || x >= 1 || y < 0 || y >= 1)
        /* Tiles usually include a bit of margin. Don't include symbols that are
         * covered by a different tile. */
        break;

      if (symbol_placement == SHUMATE_VECTOR_PLACEMENT_LINE
          || symbol_placement == SHUMATE_VECTOR_PLACEMENT_LINE_CENTER)
        place_line_label (details, x, y, scope);
      else
        {
          if (shumate_vector_render_scope_get_geometry_type (scope) == SHUMATE_VECTOR_GEOMETRY_LINESTRING)
            {
              // Place at the middle point of the linestring
              g_autoptr(GPtrArray) geometry = shumate_vector_render_scope_get_geometry (scope);
              g_ptr_array_set_free_func (geometry, (GDestroyNotify)shumate_vector_line_string_free);

              for (int i = 0; i < geometry->len; i ++)
                {
                  ShumateVectorLineString *linestring = g_ptr_array_index (geometry, i);
                  double length = shumate_vector_line_string_length (linestring);
                  ShumateVectorPointIter iter;
                  ShumateVectorPoint point;

                  shumate_vector_point_iter_init (&iter, linestring);
                  shumate_vector_point_iter_advance (&iter, length / 2);
                  shumate_vector_point_iter_get_current_point (&iter, &point);

                  place_point_label (details, point.x, point.y, scope);
                }
            }
          else
            {
              // Place at the center of the polygon's bounding box
              place_point_label (details, x, y, scope);
            }
        }

      break;
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
