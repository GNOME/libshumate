/*
 * Copyright (C) 2010 Simon Wenner <simon@wenner.ch>
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

class DemoLayer : Shumate.MarkerLayer
{
  public DemoLayer (Shumate.Viewport viewport, out Shumate.PathLayer path_layer)
  {
    Object (viewport: viewport);

    path_layer = new Shumate.PathLayer (viewport);
    var marker = new Shumate.Marker ();
    var label = new Gtk.Label ("Montréal\n<span size=\"xx-small\">Québec</span>");
    label.use_markup = true;
    label.insert_after (marker, null);
    marker.set_location (45.528178, -73.563788);
    add_marker (marker);
    path_layer.add_node (marker);

    marker = new Shumate.Marker ();
    var emblem_image = new Gtk.Image.from_icon_name ("emblem-generic");
    emblem_image.tooltip_text = "New York";
    emblem_image.insert_after (marker, null);
    marker.set_location (40.77, -73.98);
    add_marker (marker);
    path_layer.add_node (marker);

    marker = new Shumate.Marker ();
    var important_image = new Gtk.Image.from_icon_name ("emblem-important");
    important_image.insert_after (marker, null);
    marker.set_location (47.130885, -70.764141);
    add_marker (marker);
    path_layer.add_node (marker);

    marker = new Shumate.Marker ();
    var favorite_image = new Gtk.Image.from_icon_name ("emblem-favorite");
    favorite_image.insert_after (marker, null);
    marker.set_location (45.41484, -71.918907);
    add_marker (marker);
    path_layer.add_node (marker);
  }
}

