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


#pragma once

#include <glib-object.h>
#include "shumate-vector-utils-private.h"
#include "shumate-vector-render-scope-private.h"
#include "../shumate-vector-sprite.h"


G_BEGIN_DECLS

typedef enum {
  SHUMATE_VECTOR_ALIGNMENT_AUTO,
  SHUMATE_VECTOR_ALIGNMENT_MAP,
  SHUMATE_VECTOR_ALIGNMENT_VIEWPORT,
  SHUMATE_VECTOR_ALIGNMENT_VIEWPORT_GLYPH,
} ShumateVectorAlignment;

typedef enum {
  SHUMATE_VECTOR_PLACEMENT_POINT,
  SHUMATE_VECTOR_PLACEMENT_LINE,
  SHUMATE_VECTOR_PLACEMENT_LINE_CENTER,
} ShumateVectorPlacement;

typedef enum {
  SHUMATE_VECTOR_ANCHOR_CENTER,
  SHUMATE_VECTOR_ANCHOR_TOP,
  SHUMATE_VECTOR_ANCHOR_BOTTOM,
  SHUMATE_VECTOR_ANCHOR_LEFT,
  SHUMATE_VECTOR_ANCHOR_RIGHT,
  SHUMATE_VECTOR_ANCHOR_TOP_LEFT,
  SHUMATE_VECTOR_ANCHOR_TOP_RIGHT,
  SHUMATE_VECTOR_ANCHOR_BOTTOM_LEFT,
  SHUMATE_VECTOR_ANCHOR_BOTTOM_RIGHT,
} ShumateVectorAnchor;

typedef struct {
  char *layer;
  char *source_layer;
  char *feature_id;

  ShumateVectorAnchor icon_anchor;
  ShumateVectorSprite *icon_image;
  double icon_offset_x, icon_offset_y;
  float icon_size;
  ShumateVectorAlignment icon_rotation_alignment;

  /* Array of ShumateVectorFormatPart */
  GPtrArray *formatted_text;
  ShumateVectorAnchor text_anchor;
  GdkRGBA text_color;
  double text_size;
  double text_padding;
  char *text_font;
  ShumateVectorAlignment text_rotation_alignment;
  double text_offset_x, text_offset_y;

  ShumateVectorPlacement symbol_placement;
  float symbol_spacing;

  int tile_x;
  int tile_y;
  int tile_zoom_level;

  int layer_idx;
  double symbol_sort_key;

  char *cursor;

  GHashTable *tags;

  gboolean text_keep_upright : 1;

  /*< private >*/
  guint ref_count;
} ShumateVectorSymbolDetails;


ShumateVectorSymbolDetails *shumate_vector_symbol_details_ref (ShumateVectorSymbolDetails *details);
void shumate_vector_symbol_details_unref (ShumateVectorSymbolDetails *details);

#define SHUMATE_TYPE_VECTOR_SYMBOL_INFO (shumate_vector_symbol_info_get_type ())

typedef struct _ShumateVectorSymbolInfo ShumateVectorSymbolInfo;

struct _ShumateVectorSymbolInfo
{
  ShumateVectorSymbolDetails *details;

  double x;
  double y;

  ShumateVectorLineString *line;
  ShumateVectorPoint line_size;
  float line_length;
  float line_position;

  /*< private >*/
  guint ref_count;
};

void shumate_vector_symbol_info_set_line_points (ShumateVectorSymbolInfo *self,
                                                 ShumateVectorLineString *linestring,
                                                 float                    position);

GType                        shumate_vector_symbol_info_get_type (void) G_GNUC_CONST;
ShumateVectorSymbolInfo     *shumate_vector_symbol_info_ref      (ShumateVectorSymbolInfo *self);
void                         shumate_vector_symbol_info_unref    (ShumateVectorSymbolInfo *self);


int shumate_vector_symbol_info_compare (ShumateVectorSymbolInfo *a,
                                        ShumateVectorSymbolInfo *b);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (ShumateVectorSymbolDetails, shumate_vector_symbol_details_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC (ShumateVectorSymbolInfo, shumate_vector_symbol_info_unref)

G_END_DECLS

