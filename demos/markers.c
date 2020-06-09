/*
 * Copyright (C) 2008 Pierre-Luc Beaudoin <pierre-luc@pierlux.com>
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

#include <shumate/shumate.h>
#include <markers.h>


ShumateMarkerLayer *
create_marker_layer (G_GNUC_UNUSED ShumateView *view, ShumatePathLayer **path)
{
  ShumateMarker *marker;
  ShumateMarkerLayer *layer;
  GtkWidget *label;

  *path = shumate_path_layer_new ();
  layer = shumate_marker_layer_new_full (GTK_SELECTION_SINGLE);

  marker = shumate_marker_new ();
  label = gtk_label_new ("Montréal\n<span size=\"xx-small\">Québec</span>");
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_RIGHT);
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_widget_insert_before (label, GTK_WIDGET (marker), NULL);

  shumate_location_set_location (SHUMATE_LOCATION (marker),
      45.528178, -73.563788);
  shumate_marker_layer_add_marker (layer, marker);
  shumate_path_layer_add_node (*path, SHUMATE_LOCATION (marker));

  marker = shumate_marker_new ();
  gtk_widget_insert_before (gtk_image_new_from_icon_name ("emblem-generic"), GTK_WIDGET (marker), NULL);
  gtk_widget_set_tooltip_text (GTK_WIDGET (marker), "New York");
  shumate_location_set_location (SHUMATE_LOCATION (marker), 40.77, -73.98);
  shumate_marker_layer_add_marker (layer, marker);
  shumate_path_layer_add_node (*path, SHUMATE_LOCATION (marker));

  marker = shumate_marker_new ();
  gtk_widget_insert_before (gtk_image_new_from_icon_name ("emblem-important"), GTK_WIDGET (marker), NULL);
  shumate_location_set_location (SHUMATE_LOCATION (marker), 47.130885,
      -70.764141);
  shumate_marker_layer_add_marker (layer, marker);
  shumate_path_layer_add_node (*path, SHUMATE_LOCATION (marker));

  marker = shumate_point_new ();
  shumate_location_set_location (SHUMATE_LOCATION (marker), 45.130885,
      -65.764141);
  shumate_marker_layer_add_marker (layer, marker);
  shumate_path_layer_add_node (*path, SHUMATE_LOCATION (marker));

  marker = shumate_marker_new ();
  gtk_widget_insert_before (gtk_image_new_from_icon_name ("emblem-favorite"), GTK_WIDGET (marker), NULL);
  shumate_location_set_location (SHUMATE_LOCATION (marker), 45.41484,
      -71.918907);
  shumate_marker_layer_add_marker (layer, marker);
  shumate_path_layer_add_node (*path, SHUMATE_LOCATION (marker));

  shumate_marker_layer_set_all_markers_draggable (layer);

  return layer;
}
