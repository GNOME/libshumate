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


G_BEGIN_DECLS

#define SHUMATE_TYPE_VECTOR_SYMBOL_INFO (shumate_vector_symbol_info_get_type ())

typedef struct _ShumateVectorSymbolInfo ShumateVectorSymbolInfo;

struct _ShumateVectorSymbolInfo
{
  char *text;
  GdkRGBA text_color;
  double text_size;
  char *text_font;
  guint line_placement : 1;
  double x;
  double y;

  ShumateVectorLineString line;

  /*< private >*/
  guint ref_count;
};

ShumateVectorSymbolInfo *shumate_vector_symbol_info_new (const char               *text,
                                                         const GdkRGBA            *text_color,
                                                         double                    text_size,
                                                         const char               *text_font,
                                                         gboolean                  line_placement,
                                                         double                    x,
                                                         double                    y);

void shumate_vector_symbol_info_set_line_points (ShumateVectorSymbolInfo *self,
                                                 ShumateVectorLineString *linestring);

GType                        shumate_vector_symbol_info_get_type (void) G_GNUC_CONST;
ShumateVectorSymbolInfo     *shumate_vector_symbol_info_ref      (ShumateVectorSymbolInfo *self);
void                         shumate_vector_symbol_info_unref    (ShumateVectorSymbolInfo *self);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (ShumateVectorSymbolInfo, shumate_vector_symbol_info_unref)

G_END_DECLS

