/*
 * Copyright (C) 2011-2013 Jiri Techet <techet@gmail.com>
 * Copyright (C) 2019 Marcus Lundblad <ml@update.uu.se>
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/**
 * ShumatePoint:
 *
 * A simple variant of [class@Marker] showing the location of the point as a
 * circle on the map.
 *
 * ## CSS nodes
 *
 * `ShumatePoint` has a single CSS node with the name “map-point”.
 */

#include "shumate-point.h"

struct _ShumatePoint
{
  ShumateMarker parent_instance;
};

G_DEFINE_TYPE (ShumatePoint, shumate_point, SHUMATE_TYPE_MARKER);

static void
shumate_point_class_init (ShumatePointClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GdkDisplay *display;

  gtk_widget_class_set_css_name (widget_class, "map-point");

  display = gdk_display_get_default ();
  if (display)
    {
      g_autoptr(GtkCssProvider) provider = gtk_css_provider_new ();
      gtk_css_provider_load_from_resource (provider, "/org/gnome/shumate/point.css");
      gtk_style_context_add_provider_for_display (display,
                                                  GTK_STYLE_PROVIDER (provider),
                                                  GTK_STYLE_PROVIDER_PRIORITY_FALLBACK);
    }
}

static void
shumate_point_init (ShumatePoint *point)
{
  g_object_set (point,
                "halign", GTK_ALIGN_CENTER,
                "valign", GTK_ALIGN_CENTER,
                NULL);
}

/**
 * shumate_point_new:
 *
 * Creates an instance of #ShumatePoint.
 *
 * Returns: (type ShumatePoint): a new #ShumatePoint.
 */
ShumateMarker *
shumate_point_new (void)
{
  return SHUMATE_MARKER (g_object_new (SHUMATE_TYPE_POINT, NULL));
}
