/*
 * Copyright (C) 2022 James Westman <james@jwestman.net>
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

#if !defined (__SHUMATE_SHUMATE_H_INSIDE__) && !defined (SHUMATE_COMPILATION)
#error "Only <shumate/shumate.h> can be included directly."
#endif

#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define SHUMATE_TYPE_SYMBOL_EVENT (shumate_symbol_event_get_type())
G_DECLARE_FINAL_TYPE (ShumateSymbolEvent, shumate_symbol_event, SHUMATE, SYMBOL_EVENT, GObject)

const char *shumate_symbol_event_get_layer (ShumateSymbolEvent *self);
const char *shumate_symbol_event_get_source_layer (ShumateSymbolEvent *self);
const char *shumate_symbol_event_get_feature_id (ShumateSymbolEvent *self);
const GStrv shumate_symbol_event_get_keys (ShumateSymbolEvent *self);
const char *shumate_symbol_event_get_tag (ShumateSymbolEvent *self,
                                          const char         *tag_name);
gint shumate_symbol_event_get_n_press (ShumateSymbolEvent *self);

G_END_DECLS
