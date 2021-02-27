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


#include "shumate-demo-window.h"

struct _ShumateDemoWindow
{
  GtkApplicationWindow parent_instance;

  ShumateView *view;
  GtkOverlay *overlay;
  ShumateLicense *license;
};

G_DEFINE_TYPE (ShumateDemoWindow, shumate_demo_window, GTK_TYPE_APPLICATION_WINDOW)


ShumateDemoWindow *
shumate_demo_window_new (GtkApplication *app)
{
  return g_object_new (SHUMATE_TYPE_DEMO_WINDOW,
                       "application", app,
                       NULL);
}


static void
shumate_demo_window_class_init (ShumateDemoWindowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/Shumate/Demo/ui/shumate-demo-window.ui");
  gtk_widget_class_bind_template_child (widget_class, ShumateDemoWindow, view);
  gtk_widget_class_bind_template_child (widget_class, ShumateDemoWindow, overlay);
  gtk_widget_class_bind_template_child (widget_class, ShumateDemoWindow, license);
}


static void
shumate_demo_window_init (ShumateDemoWindow *self)
{
  g_autoptr(ShumateMapSourceFactory) factory = NULL;
  g_autoptr(ShumateMapSource) map_source = NULL;
  ShumateMapLayer *layer;
  ShumateScale *scale;
  ShumateViewport *viewport;

  gtk_widget_init_template (GTK_WIDGET (self));

  factory = shumate_map_source_factory_dup_default ();
  map_source = shumate_map_source_factory_create_cached_source (factory, SHUMATE_MAP_SOURCE_OSM_MAPNIK);
  shumate_view_set_map_source (self->view, map_source);

  viewport = shumate_view_get_viewport (self->view);
  shumate_viewport_set_reference_map_source (viewport, map_source);

  layer = shumate_map_layer_new (map_source, viewport);
  shumate_view_add_layer (self->view, SHUMATE_LAYER (layer));

  shumate_license_append_map_source (self->license, map_source);

  scale = shumate_scale_new (viewport);
  g_object_set (scale,
                "halign", GTK_ALIGN_START,
                "valign", GTK_ALIGN_END,
                NULL);
  gtk_overlay_add_overlay (self->overlay, GTK_WIDGET (scale));
}
