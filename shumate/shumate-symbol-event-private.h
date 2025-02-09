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

#pragma once

#include "shumate-symbol-event.h"

ShumateSymbolEvent *shumate_symbol_event_new (const char *layer,
                                              const char *source_layer,
                                              const char *feature_id,
                                              GHashTable *tags);

ShumateSymbolEvent *shumate_symbol_event_new_with_n_press (const char *layer,
                                                           const char *source_layer,
                                                           const char *feature_id,
                                                           GHashTable *tags,
                                                           gint n_press);

void shumate_symbol_event_set_lat_lon (ShumateSymbolEvent *self,
                                       double              lat,
                                       double              lon);

void shumate_symbol_event_set_n_press (ShumateSymbolEvent *self, gint n_press);
