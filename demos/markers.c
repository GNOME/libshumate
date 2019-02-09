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
  GdkRGBA orange = { 0.95, 0.6, 0.03, 0.75 };

  *path = shumate_path_layer_new ();
  layer = shumate_marker_layer_new_full (SHUMATE_SELECTION_SINGLE);

  marker = shumate_label_new_with_text ("Montréal\n<span size=\"xx-small\">Québec</span>",
        "Serif 14", NULL, NULL);
  shumate_label_set_use_markup (SHUMATE_LABEL (marker), TRUE);
  shumate_label_set_alignment (SHUMATE_LABEL (marker), PANGO_ALIGN_RIGHT);
  shumate_label_set_color (SHUMATE_LABEL (marker), &orange);

  shumate_location_set_location (SHUMATE_LOCATION (marker),
      45.528178, -73.563788);
  shumate_marker_layer_add_marker (layer, SHUMATE_MARKER (marker));
  shumate_path_layer_add_node (*path, SHUMATE_LOCATION (marker));

  marker = shumate_label_new_from_file ("icons/emblem-generic.png", NULL);
  shumate_label_set_text (SHUMATE_LABEL (marker), "New York");
  shumate_location_set_location (SHUMATE_LOCATION (marker), 40.77, -73.98);
  shumate_marker_layer_add_marker (layer, SHUMATE_MARKER (marker));
  shumate_path_layer_add_node (*path, SHUMATE_LOCATION (marker));

  marker = shumate_label_new_from_file ("icons/emblem-important.png", NULL);
  shumate_location_set_location (SHUMATE_LOCATION (marker), 47.130885,
      -70.764141);
  shumate_marker_layer_add_marker (layer, SHUMATE_MARKER (marker));
  shumate_path_layer_add_node (*path, SHUMATE_LOCATION (marker));

  marker = shumate_point_new ();
  shumate_location_set_location (SHUMATE_LOCATION (marker), 45.130885,
      -65.764141);
  shumate_marker_layer_add_marker (layer, SHUMATE_MARKER (marker));
  shumate_path_layer_add_node (*path, SHUMATE_LOCATION (marker));

  marker = shumate_label_new_from_file ("icons/emblem-favorite.png", NULL);
  shumate_label_set_draw_background (SHUMATE_LABEL (marker), FALSE);
  shumate_location_set_location (SHUMATE_LOCATION (marker), 45.41484,
      -71.918907);
  shumate_marker_layer_add_marker (layer, SHUMATE_MARKER (marker));
  shumate_path_layer_add_node (*path, SHUMATE_LOCATION (marker));
  
  shumate_marker_layer_set_all_markers_draggable (layer);

  return layer;
}
