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

  ShumateVectorExpression *icon_allow_overlap;
  ShumateVectorExpression *icon_anchor;
  ShumateVectorExpression *icon_color;
  ShumateVectorExpression *icon_ignore_placement;
  ShumateVectorExpression *icon_image;
  ShumateVectorExpression *icon_opacity;
  ShumateVectorExpression *icon_optional;
  ShumateVectorExpression *icon_overlap;
  ShumateVectorExpression *icon_padding;
  ShumateVectorExpression *icon_rotate;
  ShumateVectorExpression *icon_rotation_alignment;
  ShumateVectorExpression *icon_size;
  ShumateVectorExpression *text_anchor;
  ShumateVectorExpression *text_field;
  ShumateVectorExpression *text_letter_spacing;
  ShumateVectorExpression *text_allow_overlap;
  ShumateVectorExpression *text_ignore_placement;
  ShumateVectorExpression *text_color;
  ShumateVectorExpression *text_opacity;
  ShumateVectorExpression *text_optional;
  ShumateVectorExpression *text_overlap;
  ShumateVectorExpression *text_size;
  ShumateVectorExpression *text_transform;
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

      layer->icon_color = shumate_vector_expression_from_json (json_object_get_member (paint, "icon-color"), error);
      if (layer->icon_color == NULL)
        return NULL;

      layer->icon_opacity = shumate_vector_expression_from_json (json_object_get_member (paint, "icon-opacity"), error);
      if (layer->icon_opacity == NULL)
        return NULL;

      layer->text_color = shumate_vector_expression_from_json (json_object_get_member (paint, "text-color"), error);
      if (layer->text_color == NULL)
        return NULL;

      layer->text_opacity = shumate_vector_expression_from_json (json_object_get_member (paint, "text-opacity"), error);
      if (layer->text_opacity == NULL)
        return NULL;
    }

  if (json_object_has_member (object, "layout"))
    {
      JsonObject *layout = json_object_get_object_member (object, "layout");
      JsonNode *icon_offset_node;
      JsonNode *text_font_node;
      JsonNode *text_offset_node;
      JsonArray *text_font;

      layer->icon_allow_overlap = shumate_vector_expression_from_json (json_object_get_member (layout, "icon-allow-overlap"), error);
      if (layer->icon_allow_overlap == NULL)
        return NULL;

      layer->icon_anchor = shumate_vector_expression_from_json (json_object_get_member (layout, "icon-anchor"), error);
      if (layer->icon_anchor == NULL)
        return NULL;

      layer->icon_ignore_placement = shumate_vector_expression_from_json (json_object_get_member (layout, "icon-ignore-placement"), error);
      if (layer->icon_ignore_placement == NULL)
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

      layer->icon_optional = shumate_vector_expression_from_json (json_object_get_member (layout, "icon-optional"), error);
      if (layer->icon_optional == NULL)
        return NULL;

      layer->icon_overlap = shumate_vector_expression_from_json (json_object_get_member (layout, "icon-overlap"), error);
      if (layer->icon_overlap == NULL)
        return NULL;

      layer->icon_padding = shumate_vector_expression_from_json (json_object_get_member (layout, "icon-padding"), error);
      if (layer->icon_padding == NULL)
        return NULL;

      layer->icon_rotate = shumate_vector_expression_from_json (json_object_get_member (layout, "icon-rotate"), error);
      if (layer->icon_rotate == NULL)
        return NULL;

      layer->icon_size = shumate_vector_expression_from_json (json_object_get_member (layout, "icon-size"), error);
      if (layer->icon_size == NULL)
        return NULL;

      layer->icon_rotation_alignment = shumate_vector_expression_from_json (json_object_get_member (layout, "icon-rotation-alignment"), error);
      if (layer->icon_rotation_alignment == NULL)
        return NULL;

      layer->text_allow_overlap = shumate_vector_expression_from_json (json_object_get_member (layout, "text-allow-overlap"), error);
      if (layer->text_allow_overlap == NULL)
        return NULL;

      layer->text_field = shumate_vector_expression_from_json (json_object_get_member (layout, "text-field"), error);
      if (layer->text_field == NULL)
        return NULL;

      layer->text_ignore_placement = shumate_vector_expression_from_json (json_object_get_member (layout, "text-ignore-placement"), error);
      if (layer->text_ignore_placement == NULL)
        return NULL;

      layer->text_letter_spacing = shumate_vector_expression_from_json (json_object_get_member (layout, "text-letter-spacing"), error);
      if (layer->text_letter_spacing == NULL)
        return NULL;

      layer->text_transform = shumate_vector_expression_from_json (json_object_get_member (layout, "text-transform"), error);
      if (layer->text_transform == NULL)
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

      layer->text_optional = shumate_vector_expression_from_json (json_object_get_member (layout, "text-optional"), error);
      if (layer->text_optional == NULL)
        return NULL;

      layer->text_overlap = shumate_vector_expression_from_json (json_object_get_member (layout, "text-overlap"), error);
      if (layer->text_overlap == NULL)
        return NULL;

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

  g_clear_object (&self->icon_allow_overlap);
  g_clear_object (&self->icon_anchor);
  g_clear_object (&self->icon_color);
  g_clear_object (&self->icon_ignore_placement);
  g_clear_object (&self->icon_image);
  g_clear_object (&self->icon_opacity);
  g_clear_object (&self->icon_optional);
  g_clear_object (&self->icon_overlap);
  g_clear_object (&self->icon_padding);
  g_clear_object (&self->icon_rotate);
  g_clear_object (&self->icon_rotation_alignment);
  g_clear_object (&self->icon_size);
  g_clear_object (&self->text_allow_overlap);
  g_clear_object (&self->text_field);
  g_clear_object (&self->text_ignore_placement);
  g_clear_object (&self->text_anchor);
  g_clear_object (&self->text_color);
  g_clear_object (&self->text_size);
  g_clear_object (&self->cursor);
  g_clear_object (&self->text_keep_upright);
  g_clear_object (&self->text_letter_spacing);
  g_clear_object (&self->text_opacity);
  g_clear_object (&self->text_optional);
  g_clear_object (&self->text_overlap);
  g_clear_object (&self->text_padding);
  g_clear_object (&self->text_rotation_alignment);
  g_clear_object (&self->text_transform);
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
  if (x >= 0 && x < 1 && y >= 0 && y < 1)
    g_ptr_array_add (scope->symbols, create_symbol_info (details, x, y));
}


static void
place_line_label (ShumateVectorSymbolDetails *details,
                  ShumateVectorRenderScope   *scope)
{
  ShumateVectorSymbolInfo *symbol_info;
  GPtrArray *lines;
  g_autoptr(GPtrArray) simplified_lines = g_ptr_array_new_with_free_func ((GDestroyNotify)shumate_vector_line_string_free);
  guint i;
  float spacing = details->symbol_spacing / scope->target_size;
  float total_length = 0;
  float remaining_distance;

  if (spacing <= 0)
    return;

  lines = shumate_vector_render_scope_get_geometry (scope);
  g_ptr_array_set_free_func (lines, NULL);
  for (i = 0; i < lines->len; i ++)
    {
      if (details->text_rotation_alignment == SHUMATE_VECTOR_ALIGNMENT_MAP
          || details->text_rotation_alignment == SHUMATE_VECTOR_ALIGNMENT_VIEWPORT_GLYPH)
        {
          GPtrArray *split_lines = shumate_vector_line_string_simplify ((ShumateVectorLineString *)lines->pdata[i]);
          g_ptr_array_extend_and_steal (simplified_lines, split_lines);
        }
      else
        g_ptr_array_add (simplified_lines, (ShumateVectorLineString *)lines->pdata[i]);
    }
  g_clear_pointer (&lines, g_ptr_array_unref);

  for (i = 0; i < simplified_lines->len; i ++)
    total_length += shumate_vector_line_string_length ((ShumateVectorLineString *)simplified_lines->pdata[i]);

  /* Make the spacing even on both sides */
  remaining_distance = fmod (total_length / 2, spacing);

  for (i = 0; i < simplified_lines->len; i ++)
    {
      ShumateVectorLineString *linestring = (ShumateVectorLineString *)simplified_lines->pdata[i];
      float distance = 0;
      ShumateVectorPointIter iter;

      shumate_vector_point_iter_init (&iter, linestring);

      shumate_vector_point_iter_advance (&iter, remaining_distance);
      distance += remaining_distance;

      while (!shumate_vector_point_iter_is_at_end (&iter))
        {
          ShumateVectorPoint point;
          shumate_vector_point_iter_get_current_point (&iter, &point);

          if (point.x >= 0 && point.x < 1 && point.y >= 0 && point.y < 1)
            {
              symbol_info = create_symbol_info (details, point.x, point.y);

              shumate_vector_symbol_info_set_line_points (symbol_info,
                                                          shumate_vector_line_string_copy (linestring),
                                                          distance);

              g_ptr_array_add (scope->symbols, symbol_info);
            }

          shumate_vector_point_iter_advance (&iter, spacing);
          distance += spacing;
        }

      remaining_distance = distance - shumate_vector_line_string_length (linestring);
    }
}


static void
shumate_vector_symbol_layer_render (ShumateVectorLayer *layer, ShumateVectorRenderScope *scope)
{
  ShumateVectorSymbolLayer *self = SHUMATE_VECTOR_SYMBOL_LAYER (layer);
  g_auto(ShumateVectorValue) value = SHUMATE_VECTOR_VALUE_INIT;
  g_auto(ShumateVectorValue) icon_padding_value = SHUMATE_VECTOR_VALUE_INIT;
  GPtrArray *icon_padding_array;
  g_autoptr(GPtrArray) text_field = NULL;
  g_autofree char *cursor = NULL;
  g_autofree char *feature_id = NULL;
  ShumateVectorPlacement symbol_placement;
  ShumateVectorAlignment icon_rotation_alignment, text_rotation_alignment;
  g_autofree char *text_transform = NULL;
  g_autoptr(GHashTable) tags = NULL;
  g_autoptr(ShumateVectorSymbolDetails) details = NULL;
  g_autoptr(ShumateVectorSprite) icon_image = shumate_vector_expression_eval_image (self->icon_image, scope);
  ShumateVectorGeometryType geometry_type = shumate_vector_render_scope_get_geometry_type (scope);
  VectorTile__Tile__Layer *layer_struct = shumate_vector_reader_iter_get_layer_struct (scope->reader);
  VectorTile__Tile__Feature *feature = shumate_vector_reader_iter_get_feature_struct (scope->reader);
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
          ShumateVectorFormatPart *part;
          text_field = g_ptr_array_new_with_free_func ((GDestroyNotify)shumate_vector_format_part_free);
          part = g_new0 (ShumateVectorFormatPart, 1);
          part->string = g_steal_pointer (&text);
          g_ptr_array_add (text_field, part);
        }
    }

  if (text_field == NULL && icon_image == NULL)
    return;

  text_transform = shumate_vector_expression_eval_string (self->text_transform, scope, NULL);
  if (text_field != NULL && text_transform != NULL)
    {
      for (int i = 0; i < text_field->len; i ++)
        {
          ShumateVectorFormatPart *part = g_ptr_array_index (text_field, i);
          if (part->string != NULL)
            {
              if (g_strcmp0 (text_transform, "uppercase") == 0)
                part->string = g_utf8_strup (part->string, -1);
              else if (g_strcmp0 (text_transform, "lowercase") == 0)
                part->string = g_utf8_strdown (part->string, -1);
            }
        }
    }

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

  feature_id = g_strdup_printf ("%ld", feature->id);
  cursor = shumate_vector_expression_eval_string (self->cursor, scope, NULL);

  tags = shumate_vector_render_scope_create_tag_table (scope);

  details = g_new0 (ShumateVectorSymbolDetails, 1);
  *details = (ShumateVectorSymbolDetails) {
    .ref_count = 1,
    .layer = g_strdup (shumate_vector_layer_get_id (layer)),
    .source_layer = g_strdup (layer_struct->name),
    .feature_id = g_strdup (feature_id),
    .tags = g_hash_table_ref (tags),

    .icon_anchor = shumate_vector_expression_eval_anchor (self->icon_anchor, scope),
    .icon_ignore_placement = shumate_vector_expression_eval_boolean (self->icon_ignore_placement, scope, FALSE),
    .icon_image = g_steal_pointer (&icon_image),
    .icon_opacity = CLAMP (shumate_vector_expression_eval_number (self->icon_opacity, scope, 1.0), 0.0, 1.0),
    .icon_optional = shumate_vector_expression_eval_boolean (self->icon_optional, scope, FALSE),
    .icon_overlap = shumate_vector_expression_eval_overlap (self->icon_overlap, self->icon_allow_overlap, scope),
    .icon_rotate = shumate_vector_expression_eval_number (self->icon_rotate, scope, 0.0) * G_PI / 180,
    .icon_rotation_alignment = icon_rotation_alignment,
    .icon_size = shumate_vector_expression_eval_number (self->icon_size, scope, 1.0),
    .icon_offset_x = self->icon_offset_x,
    .icon_offset_y = self->icon_offset_y,

    .formatted_text = g_steal_pointer (&text_field),

    .text_anchor = shumate_vector_expression_eval_anchor (self->text_anchor, scope),
    .text_ignore_placement = shumate_vector_expression_eval_boolean (self->text_ignore_placement, scope, FALSE),
    .text_letter_spacing = shumate_vector_expression_eval_number (self->text_letter_spacing, scope, 0.0),
    .text_opacity = shumate_vector_expression_eval_number (self->text_opacity, scope, 1.0),
    .text_optional = shumate_vector_expression_eval_boolean (self->text_optional, scope, FALSE),
    .text_overlap = shumate_vector_expression_eval_overlap (self->text_overlap, self->text_allow_overlap, scope),
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

  details->icon_color = SHUMATE_VECTOR_COLOR_BLACK;
  shumate_vector_expression_eval_color (self->icon_color, scope, &details->icon_color);

  details->text_color = SHUMATE_VECTOR_COLOR_BLACK;
  shumate_vector_expression_eval_color (self->text_color, scope, &details->text_color);

  shumate_vector_expression_eval (self->icon_padding, scope, &icon_padding_value);
  if ((icon_padding_array = shumate_vector_value_get_array (&icon_padding_value)) != NULL)
    {
      if (icon_padding_array->len == 0 || !shumate_vector_value_get_number (g_ptr_array_index (icon_padding_array, 0), &details->icon_padding_top))
        details->icon_padding_top = 2;

      if (icon_padding_array->len <= 1 || !shumate_vector_value_get_number (g_ptr_array_index (icon_padding_array, 1), &details->icon_padding_right))
        details->icon_padding_right = details->icon_padding_top;

      if (icon_padding_array->len <= 2 || !shumate_vector_value_get_number (g_ptr_array_index (icon_padding_array, 2), &details->icon_padding_bottom))
        details->icon_padding_bottom = details->icon_padding_top;

      if (icon_padding_array->len <= 3 || !shumate_vector_value_get_number (g_ptr_array_index (icon_padding_array, 3), &details->icon_padding_left))
        details->icon_padding_left = details->icon_padding_right;
    }
  else
    {
      if (!shumate_vector_value_get_number (&icon_padding_value, &details->icon_padding_top))
        details->icon_padding_top = 2;

      details->icon_padding_right = details->icon_padding_top;
      details->icon_padding_bottom = details->icon_padding_top;
      details->icon_padding_left = details->icon_padding_top;
    }

  switch (shumate_vector_render_scope_get_geometry_type (scope))
    {
    case SHUMATE_VECTOR_GEOMETRY_POINT:
        {
          g_autoptr(GPtrArray) geometry = shumate_vector_render_scope_get_geometry (scope);

          for (int i = 0; i < geometry->len; i ++)
            {
              ShumateVectorLineString *string = g_ptr_array_index (geometry, i);

              for (int j = 0; j < string->n_points; j ++)
                {
                  x = string->points[j].x;
                  y = string->points[j].y;
                  place_point_label (details, x, y, scope);
                }
            }
        }

      break;

    case SHUMATE_VECTOR_GEOMETRY_LINESTRING:
    case SHUMATE_VECTOR_GEOMETRY_POLYGON:
      shumate_vector_render_scope_get_geometry_center (scope, &x, &y);

      if (symbol_placement == SHUMATE_VECTOR_PLACEMENT_LINE
          || symbol_placement == SHUMATE_VECTOR_PLACEMENT_LINE_CENTER)
        place_line_label (details, scope);
      else
        {
          if (shumate_vector_render_scope_get_geometry_type (scope) == SHUMATE_VECTOR_GEOMETRY_LINESTRING)
            {
              double distance = 0;
              g_autoptr(GPtrArray) geometry = shumate_vector_render_scope_get_geometry (scope);

              for (int i = 0; i < geometry->len; i ++)
                distance += shumate_vector_line_string_length (g_ptr_array_index (geometry, i)) / 2;

              for (int i = 0; i < geometry->len; i ++)
                {
                  ShumateVectorLineString *linestring = g_ptr_array_index (geometry, i);
                  ShumateVectorPointIter iter;

                  shumate_vector_point_iter_init (&iter, linestring);
                  shumate_vector_point_iter_advance (&iter, distance);

                  if (shumate_vector_point_iter_is_at_end (&iter))
                    distance -= shumate_vector_line_string_length (linestring);
                  else
                    {
                      ShumateVectorPoint point;
                      shumate_vector_point_iter_get_current_point (&iter, &point);
                      place_point_label (details, point.x, point.y, scope);
                      break;
                    }
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
