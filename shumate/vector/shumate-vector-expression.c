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


#include "shumate-vector-renderer.h"
#include "shumate-vector-expression-private.h"
#include "shumate-vector-expression-filter-private.h"
#include "shumate-vector-expression-interpolate-private.h"
#include "shumate-vector-value-private.h"


G_DEFINE_TYPE (ShumateVectorExpression, shumate_vector_expression, G_TYPE_OBJECT)


ShumateVectorExpression *
shumate_vector_expression_from_json (JsonNode  *json,
                                     GError   **error)
{
  if (json == NULL || JSON_NODE_HOLDS_NULL (json))
    return shumate_vector_expression_filter_from_literal (&SHUMATE_VECTOR_VALUE_INIT);
  else if (JSON_NODE_HOLDS_VALUE (json))
    {
      g_auto(GValue) gvalue = G_VALUE_INIT;
      g_auto(ShumateVectorValue) value = SHUMATE_VECTOR_VALUE_INIT;
      const char *string;

      json_node_get_value (json, &gvalue);
      if (!shumate_vector_value_set_from_g_value (&value, &gvalue))
        {
          g_set_error (error,
                       SHUMATE_STYLE_ERROR,
                       SHUMATE_STYLE_ERROR_INVALID_EXPRESSION,
                       "Unsupported literal value in expression");
          return NULL;
        }

      if (shumate_vector_value_get_string (&value, &string))
        return shumate_vector_expression_filter_from_format (string, error);
      else
        return shumate_vector_expression_filter_from_literal (&value);
    }
  else if (JSON_NODE_HOLDS_OBJECT (json))
    return shumate_vector_expression_interpolate_from_json_obj (json_node_get_object (json), error);
  else if (JSON_NODE_HOLDS_ARRAY (json))
    {
      JsonArray *array = json_node_get_array (json);
      if (json_array_get_length (array) > 1)
        {
          JsonNode *first = json_array_get_element (array, 0);
          if (JSON_NODE_HOLDS_VALUE (first) && json_node_get_value_type (first) != G_TYPE_STRING)
            {
              /* If it's an array of numbers, return it as a literal */
              g_auto(ShumateVectorValue) value = SHUMATE_VECTOR_VALUE_INIT;
              shumate_vector_value_start_array (&value);
              for (guint i = 0; i < json_array_get_length (array); i++)
                {
                  g_auto(ShumateVectorValue) element = SHUMATE_VECTOR_VALUE_INIT;
                  if (!shumate_vector_value_set_from_json_literal (&element, json_array_get_element (array, i), error))
                    return NULL;
                  shumate_vector_value_array_append (&value, &element);
                }

              return shumate_vector_expression_filter_from_literal (&value);
            }
        }

      return shumate_vector_expression_filter_from_json_array (json_node_get_array (json), NULL, error);
    }
  else
    g_assert_not_reached ();
}


static gboolean
shumate_vector_expression_real_eval (ShumateVectorExpression  *self,
                                     ShumateVectorRenderScope *scope,
                                     ShumateVectorValue       *out)
{
  g_assert_not_reached ();
}

/* A default implementation for eval_bitset() that calls the regular eval() method for each
   feature. The mask parameter allows it to skip features that don't need to be calculated
   (e.g. they have been excluded by a previous child of an ALL expression). */
static ShumateVectorIndexBitset *
shumate_vector_expression_real_eval_bitset (ShumateVectorExpression  *self,
                                            ShumateVectorRenderScope *scope,
                                            ShumateVectorIndexBitset *mask)
{
  VectorTile__Tile__Layer *layer = shumate_vector_reader_iter_get_layer_struct (scope->reader);
  ShumateVectorIndexBitset *result = shumate_vector_index_bitset_new (layer->n_features);
  int feature_idx = 0;

  if (mask != NULL)
    {
      feature_idx = -1;
      while ((feature_idx = shumate_vector_index_bitset_next (mask, feature_idx)) != -1)
        {
          shumate_vector_reader_iter_read_feature (scope->reader, feature_idx);
          if (shumate_vector_expression_eval_boolean (self, scope, FALSE))
            shumate_vector_index_bitset_set (result, feature_idx);
        }
    }
  else
    {
      shumate_vector_reader_iter_read_feature (scope->reader, 0);
      while (TRUE)
        {
          if (shumate_vector_expression_eval_boolean (self, scope, FALSE))
            shumate_vector_index_bitset_set (result, feature_idx);

          if (!shumate_vector_reader_iter_next_feature (scope->reader))
            break;

          feature_idx ++;
        }
    }

  return result;
}


static void
shumate_vector_expression_class_init (ShumateVectorExpressionClass *klass)
{
  klass->eval = shumate_vector_expression_real_eval;
  klass->eval_bitset = shumate_vector_expression_real_eval_bitset;
  klass->collect_indexes = NULL;
}


static void
shumate_vector_expression_init (ShumateVectorExpression *self)
{
}


gboolean
shumate_vector_expression_eval (ShumateVectorExpression  *self,
                                ShumateVectorRenderScope *scope,
                                ShumateVectorValue       *out)
{
  g_assert (self == NULL || SHUMATE_IS_VECTOR_EXPRESSION (self));

  if (self == NULL)
    return FALSE;
  else
    return SHUMATE_VECTOR_EXPRESSION_GET_CLASS (self)->eval (self, scope, out);
}


/* Evaluates the expression as a boolean for every feature in the scope's current layer and
   returns the result in a bitset. If the mask parameter is provided, only those features
   need to be calculated. */
ShumateVectorIndexBitset *
shumate_vector_expression_eval_bitset (ShumateVectorExpression *self,
                                       ShumateVectorRenderScope *scope,
                                       ShumateVectorIndexBitset *mask)
{
  g_assert (SHUMATE_IS_VECTOR_EXPRESSION (self));
  return SHUMATE_VECTOR_EXPRESSION_GET_CLASS (self)->eval_bitset (self, scope, mask);
}


/* Adds any indexes this expression uses to the index description. Indexes can only be used
   in eval_bitset() implementations, not eval(). */
void
shumate_vector_expression_collect_indexes (ShumateVectorExpression       *self,
                                           const char                    *layer_name,
                                           ShumateVectorIndexDescription *index_description)
{
  g_assert (SHUMATE_IS_VECTOR_EXPRESSION (self));
  if (SHUMATE_VECTOR_EXPRESSION_GET_CLASS (self)->collect_indexes != NULL)
    SHUMATE_VECTOR_EXPRESSION_GET_CLASS (self)->collect_indexes (self, layer_name, index_description);
}


double
shumate_vector_expression_eval_number (ShumateVectorExpression  *self,
                                       ShumateVectorRenderScope *scope,
                                       double                    default_val)
{
  double result;
  g_auto(ShumateVectorValue) value = SHUMATE_VECTOR_VALUE_INIT;

  shumate_vector_expression_eval (self, scope, &value);

  if (shumate_vector_value_get_number (&value, &result))
    return result;
  else
    return default_val;
}


gboolean
shumate_vector_expression_eval_boolean (ShumateVectorExpression  *self,
                                        ShumateVectorRenderScope *scope,
                                        gboolean                  default_val)
{
  gboolean result;
  g_auto(ShumateVectorValue) value = SHUMATE_VECTOR_VALUE_INIT;

  shumate_vector_expression_eval (self, scope, &value);

  if (shumate_vector_value_get_boolean (&value, &result))
    return result;
  else
    return default_val;
}


char *
shumate_vector_expression_eval_string (ShumateVectorExpression  *self,
                                       ShumateVectorRenderScope *scope,
                                       const char               *default_val)
{
  const char *result;
  g_auto(ShumateVectorValue) value = SHUMATE_VECTOR_VALUE_INIT;

  shumate_vector_expression_eval (self, scope, &value);

  if (shumate_vector_value_get_string (&value, &result))
    return g_strdup (result);
  else
    return g_strdup (default_val);
}


void
shumate_vector_expression_eval_color (ShumateVectorExpression  *self,
                                      ShumateVectorRenderScope *scope,
                                      GdkRGBA                  *color)
{
  g_auto(ShumateVectorValue) value = SHUMATE_VECTOR_VALUE_INIT;
  shumate_vector_expression_eval (self, scope, &value);
  shumate_vector_value_get_color (&value, color);
}

ShumateVectorSprite *
shumate_vector_expression_eval_image (ShumateVectorExpression  *self,
                                      ShumateVectorRenderScope *scope)
{
  g_auto(ShumateVectorValue) value = SHUMATE_VECTOR_VALUE_INIT;
  shumate_vector_expression_eval (self, scope, &value);
  if (value.type == SHUMATE_VECTOR_VALUE_TYPE_STRING)
    {
      const char *name;
      shumate_vector_value_get_string (&value, &name);
      return shumate_vector_sprite_sheet_get_sprite (scope->sprites, name, scope->scale_factor);
    }
  else if (value.type == SHUMATE_VECTOR_VALUE_TYPE_RESOLVED_IMAGE)
    {
      ShumateVectorSprite *sprite;
      shumate_vector_value_get_image (&value, &sprite);
      return g_object_ref (sprite);
    }
  else
    return NULL;
}

ShumateVectorAlignment
shumate_vector_expression_eval_alignment (ShumateVectorExpression  *self,
                                          ShumateVectorRenderScope *scope)
{
  g_auto(ShumateVectorValue) value = SHUMATE_VECTOR_VALUE_INIT;
  const char *string;

  shumate_vector_expression_eval (self, scope, &value);

  if (shumate_vector_value_get_string (&value, &string))
    {
      if (g_strcmp0 (string, "map") == 0)
        return SHUMATE_VECTOR_ALIGNMENT_MAP;
      else if (g_strcmp0 (string, "viewport") == 0)
        return SHUMATE_VECTOR_ALIGNMENT_VIEWPORT;
      else if (g_strcmp0 (string, "viewport-glyph") == 0)
        return SHUMATE_VECTOR_ALIGNMENT_VIEWPORT_GLYPH;
    }

  return SHUMATE_VECTOR_ALIGNMENT_AUTO;
}

ShumateVectorPlacement
shumate_vector_expression_eval_placement (ShumateVectorExpression  *self,
                                          ShumateVectorRenderScope *scope)
{
  g_auto(ShumateVectorValue) value = SHUMATE_VECTOR_VALUE_INIT;
  const char *string;

  shumate_vector_expression_eval (self, scope, &value);

  if (shumate_vector_value_get_string (&value, &string))
    {
      if (g_strcmp0 (string, "line") == 0)
        return SHUMATE_VECTOR_PLACEMENT_LINE;
      else if (g_strcmp0 (string, "line-center") == 0)
        return SHUMATE_VECTOR_PLACEMENT_LINE_CENTER;
    }

  return SHUMATE_VECTOR_PLACEMENT_POINT;
}

ShumateVectorAnchor
shumate_vector_expression_eval_anchor (ShumateVectorExpression  *self,
                                       ShumateVectorRenderScope *scope)
{
  g_auto(ShumateVectorValue) value = SHUMATE_VECTOR_VALUE_INIT;
  const char *string;

  shumate_vector_expression_eval (self, scope, &value);

  if (shumate_vector_value_get_string (&value, &string))
    {
      if (g_strcmp0 (string, "top") == 0)
        return SHUMATE_VECTOR_ANCHOR_TOP;
      else if (g_strcmp0 (string, "bottom") == 0)
        return SHUMATE_VECTOR_ANCHOR_BOTTOM;
      else if (g_strcmp0 (string, "left") == 0)
        return SHUMATE_VECTOR_ANCHOR_LEFT;
      else if (g_strcmp0 (string, "right") == 0)
        return SHUMATE_VECTOR_ANCHOR_RIGHT;
      else if (g_strcmp0 (string, "top-left") == 0)
        return SHUMATE_VECTOR_ANCHOR_TOP_LEFT;
      else if (g_strcmp0 (string, "top-right") == 0)
        return SHUMATE_VECTOR_ANCHOR_TOP_RIGHT;
      else if (g_strcmp0 (string, "bottom-left") == 0)
        return SHUMATE_VECTOR_ANCHOR_BOTTOM_LEFT;
      else if (g_strcmp0 (string, "bottom-right") == 0)
        return SHUMATE_VECTOR_ANCHOR_BOTTOM_RIGHT;
    }

  return SHUMATE_VECTOR_ANCHOR_CENTER;
}

ShumateVectorOverlap
shumate_vector_expression_eval_overlap (ShumateVectorExpression  *self,
                                        ShumateVectorExpression  *allow_overlap,
                                        ShumateVectorRenderScope *scope)
{
  g_auto(ShumateVectorValue) value = SHUMATE_VECTOR_VALUE_INIT;
  const char *string;

  shumate_vector_expression_eval (self, scope, &value);

  if (shumate_vector_value_get_string (&value, &string))
    {
      if (g_strcmp0 (string, "always") == 0)
        return SHUMATE_VECTOR_OVERLAP_ALWAYS;
      else if (g_strcmp0 (string, "never") == 0)
        return SHUMATE_VECTOR_OVERLAP_NEVER;
      else if (g_strcmp0 (string, "cooperative") == 0)
        return SHUMATE_VECTOR_OVERLAP_COOPERATIVE;
    }

  if (shumate_vector_expression_eval_boolean (allow_overlap, scope, FALSE))
    return SHUMATE_VECTOR_OVERLAP_ALWAYS;
  else
    return SHUMATE_VECTOR_OVERLAP_NEVER;
}

void
shumate_vector_expression_context_clear (ShumateVectorExpressionContext *ctx)
{
  g_clear_pointer (&ctx->variables, g_hash_table_unref);
}

