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

using GLib;
using Gtk;
using Shumate;

public class Launcher : GLib.Object
{
  private const int PADDING = 10;
  private Shumate.View view;
  private Gtk.Window window;

  public Launcher ()
  {
    float width, total_width = 0;

    window = new Gtk.Window ();
    window.title = "Shumate Vala Example";
    window.default_width = 800;
    window.default_height = 600;

    /* Create the map view */
    view = new Shumate.View ();
    //view.set_size_request (800, 600);
    window.add (view);

    /* Create the markers and marker layer */
    var layer = new  DemoLayer ();
    view.add_layer (layer);

    /* Connect to the click event */
    view.reactive = true;
    view.button_release_event.connect (button_release_cb);

    /* Finish initialising the map view */
    view.zoom_level = 7;
    view.kinetic_mode = true;
    view.center_on (45.466, -73.75);
  }

  public void show ()
  {
    window.show ();
  }


  public static int main (string[] args)
  {
    if (Gtk.init (ref args) != InitError.SUCCESS)
      return 1;

    var launcher = new Launcher ();
    launcher.show ();
    Gtk.main ();
    return 0;
  }
}

