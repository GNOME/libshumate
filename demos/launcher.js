#!/usr/bin/env gjs
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

/**
 * Shumate launcher example in Javascript.
 *
 * Dependencies:
 *  * gobject-introspection
 *  * Build Shumate with '--enable-introspection'
 *  * Gjs: http://live.gnome.org/Gjs
 *
 * If you installed Shumate in /usr/local you have to run:
 * export GI_TYPELIB_PATH=$GI_TYPELIB_PATH:/usr/local/lib/girepository-1.0/
 */

imports.gi.versions.Gtk = '4.0';
imports.gi.versions.Gdk = '4.0';

const Gdk = imports.gi.Gdk;
const Gtk = imports.gi.Gtk;
const Shumate = imports.gi.Shumate;

Gtk.init ();


function map_view_button_release_cb (event)
{
  let lat = view.y_to_latitude (event.button.y);
  let lon = view.x_to_longitude (event.button.x);

  log ("Map clicked at %f, %f \n", lat, lon);

  return true;
}

let window = new Gtk.Window (Gtk.Window.TOPLEVEL);
window.title = "Shumate Javascript Example";
window.set_size_request (800, 600);

/* Create the map view */
let view = new Shumate.View();
window.add (view);

/* Create the buttons */
let buttons = new Clutter.Actor ();
let total_width = 0;
buttons.set_position (PADDING, PADDING);


/* Create the markers and marker layer */
let orange= Clutter.Color.new(0xf3,0x94,0x07,0xbb);
let layer=new Shumate.MarkerLayer();
let marker=Shumate.Label.new_with_text("Sample Marker","Serif 14",null,orange);
marker.set_location(45.466, -73.75);
layer.add_marker(marker);
marker.set_reactive(true);
layer.show();
view.add_layer(layer);

view.connect('button-pressed-event', (w, event) => map_view_button_release_cb (event));

/* Finish initialising the map view */
view.zoom_level = 12;
view.kinetic_mode = true;
view.center_on (45.466, -73.75);

window.connect ("destroy", Gtk.main_quit);

window.show ();
Gtk.main ();
