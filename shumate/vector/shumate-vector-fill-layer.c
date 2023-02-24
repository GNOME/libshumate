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
#include "shumate-vector-expression-private.h"
#include "shumate-vector-fill-layer-private.h"
#include "shumate-vector-sprite-sheet-private.h"
#include "shumate-vector-utils-private.h"
#include "shumate-vector-value-private.h"

struct _ShumateVectorFillLayer
{
  ShumateVectorLayer parent_instance;

  ShumateVectorExpression *color;
  ShumateVectorExpression *opacity;
  ShumateVectorExpression *pattern;
};

G_DEFINE_TYPE (ShumateVectorFillLayer, shumate_vector_fill_layer, SHUMATE_TYPE_VECTOR_LAYER)


ShumateVectorLayer *
shumate_vector_fill_layer_create_from_json (JsonObject *object, GError **error)
{
  ShumateVectorFillLayer *layer = g_object_new (SHUMATE_TYPE_VECTOR_FILL_LAYER, NULL);
  JsonNode *paint_node;

  if ((paint_node = json_object_get_member (object, "paint")))
    {
      JsonObject *paint;

      if (!shumate_vector_json_get_object (paint_node, &paint, error))
        return NULL;

      if (!(layer->color = shumate_vector_expression_from_json (json_object_get_member (paint, "fill-color"), error)))
        return NULL;

      if (!(layer->opacity = shumate_vector_expression_from_json (json_object_get_member (paint, "fill-opacity"), error)))
        return NULL;

      if (!(layer->pattern = shumate_vector_expression_from_json (json_object_get_member (paint, "fill-pattern"), error)))
        return NULL;
    }

  return (ShumateVectorLayer *)layer;
}


static void
shumate_vector_fill_layer_render (ShumateVectorLayer *layer, ShumateVectorRenderScope *scope)
{
  ShumateVectorFillLayer *self = SHUMATE_VECTOR_FILL_LAYER (layer);
  GdkRGBA color = SHUMATE_VECTOR_COLOR_BLACK;
  double opacity;
  g_autofree char *pattern;

  shumate_vector_expression_eval_color (self->color, scope, &color);
  opacity = shumate_vector_expression_eval_number (self->opacity, scope, 1.0);
  pattern = shumate_vector_expression_eval_string (self->pattern, scope, NULL);

  shumate_vector_render_scope_exec_geometry (scope);

  if (pattern != NULL)
    {
      GdkPixbuf *sprite = shumate_vector_sprite_sheet_get_icon (scope->sprites, pattern);
      if (sprite != NULL)
        {
          cairo_pattern_t *cr_pattern;
          cairo_matrix_t matrix;

          gdk_cairo_set_source_pixbuf (scope->cr, sprite, 0, 0);
          cr_pattern = cairo_get_source (scope->cr);
          cairo_matrix_init_scale (&matrix, 1 / scope->scale, 1 / scope->scale);
          cairo_pattern_set_matrix (cr_pattern, &matrix);
          cairo_pattern_set_extend (cr_pattern, CAIRO_EXTEND_REPEAT);

          /* Use cairo_paint_with_alpha so that we can set fill-opacity correctly. */
          cairo_save (scope->cr);
          cairo_clip (scope->cr);
          cairo_paint_with_alpha (scope->cr, opacity);
          cairo_restore (scope->cr);
        }
    }
  else
    {
      cairo_set_source_rgba (scope->cr, color.red, color.green, color.blue, color.alpha * opacity);
      cairo_fill (scope->cr);
    }
}


static void
shumate_vector_fill_layer_finalize (GObject *object)
{
  ShumateVectorFillLayer *self = SHUMATE_VECTOR_FILL_LAYER (object);

  g_clear_object (&self->color);
  g_clear_object (&self->opacity);
  g_clear_object (&self->pattern);

  G_OBJECT_CLASS (shumate_vector_fill_layer_parent_class)->finalize (object);
}


static void
shumate_vector_fill_layer_class_init (ShumateVectorFillLayerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ShumateVectorLayerClass *layer_class = SHUMATE_VECTOR_LAYER_CLASS (klass);

  object_class->finalize = shumate_vector_fill_layer_finalize;
  layer_class->render = shumate_vector_fill_layer_render;
}


static void
shumate_vector_fill_layer_init (ShumateVectorFillLayer *self)
{
}
