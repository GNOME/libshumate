/*
 * Copyright (C) 2023 James Westman <james@jwestman.net>
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

#if !defined (__SHUMATE_SHUMATE_H_INSIDE__) && !defined (SHUMATE_COMPILATION)
#error "Only <shumate/shumate.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#define SHUMATE_TYPE_DATA_SOURCE_REQUEST (shumate_data_source_request_get_type())
G_DECLARE_DERIVABLE_TYPE (ShumateDataSourceRequest, shumate_data_source_request, SHUMATE, DATA_SOURCE_REQUEST, GObject)


struct _ShumateDataSourceRequestClass
{
  GObjectClass parent_class;

  /*< private >*/
  gpointer padding[16];
};

ShumateDataSourceRequest *shumate_data_source_request_new (int x,
                                                           int y,
                                                           int zoom_level);

int shumate_data_source_request_get_x (ShumateDataSourceRequest *self);
int shumate_data_source_request_get_y (ShumateDataSourceRequest *self);
int shumate_data_source_request_get_zoom_level (ShumateDataSourceRequest *self);

GBytes *shumate_data_source_request_get_data (ShumateDataSourceRequest *self);
void shumate_data_source_request_emit_data (ShumateDataSourceRequest *self,
                                            GBytes                   *data,
                                            gboolean                  complete);

GError *shumate_data_source_request_get_error (ShumateDataSourceRequest *self);
void shumate_data_source_request_emit_error (ShumateDataSourceRequest *self,
                                             const GError             *error);

gboolean shumate_data_source_request_is_completed (ShumateDataSourceRequest *self);
void shumate_data_source_request_complete (ShumateDataSourceRequest *self);

G_END_DECLS
