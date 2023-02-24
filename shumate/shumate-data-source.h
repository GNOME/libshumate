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

#include <gio/gio.h>
#include "shumate-data-source-request.h"

G_BEGIN_DECLS

#define SHUMATE_TYPE_DATA_SOURCE (shumate_data_source_get_type())

G_DECLARE_DERIVABLE_TYPE (ShumateDataSource, shumate_data_source, SHUMATE, DATA_SOURCE, GObject)

struct _ShumateDataSourceClass
{
  GObjectClass parent_class;

  void (*get_tile_data_async)     (ShumateDataSource   *self,
                                   int                  x,
                                   int                  y,
                                   int                  zoom_level,
                                   GCancellable        *cancellable,
                                   GAsyncReadyCallback  callback,
                                   gpointer             user_data);
  GBytes *(*get_tile_data_finish) (ShumateDataSource  *self,
                                   GAsyncResult       *result,
                                   GError            **error);

  ShumateDataSourceRequest *(*start_request) (ShumateDataSource *self,
                                              int                x,
                                              int                y,
                                              int                zoom_level,
                                              GCancellable      *cancellable);

  /*< private >*/
  gpointer padding[15];
};


void    shumate_data_source_get_tile_data_async (ShumateDataSource   *self,
                                                 int                  x,
                                                 int                  y,
                                                 int                  zoom_level,
                                                 GCancellable        *cancellable,
                                                 GAsyncReadyCallback  callback,
                                                 gpointer             user_data);
GBytes *shumate_data_source_get_tile_data_finish (ShumateDataSource  *self,
                                                  GAsyncResult       *result,
                                                  GError            **error);

ShumateDataSourceRequest *shumate_data_source_start_request (ShumateDataSource *self,
                                                             int                x,
                                                             int                y,
                                                             int                zoom_level,
                                                             GCancellable      *cancellable);

guint shumate_data_source_get_min_zoom_level (ShumateDataSource *self);
void shumate_data_source_set_min_zoom_level (ShumateDataSource *self,
                                             guint              zoom_level);
guint shumate_data_source_get_max_zoom_level (ShumateDataSource *self);
void shumate_data_source_set_max_zoom_level (ShumateDataSource *self,
                                             guint              zoom_level);

G_END_DECLS
