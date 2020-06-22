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

public class Launcher : Gtk.Application {
    public Launcher () {
        Object(
            application_id: "org.gnome.Shumate.ValaExample",
            flags: ApplicationFlags.FLAGS_NONE
        );
    }

    protected override void activate () {
        Shumate.PathLayer path_layer;
        // Create the window of this application and show it
        Gtk.ApplicationWindow window = new Gtk.ApplicationWindow (this);
        window.set_default_size (800, 600);
        window.title = "Shumate Vala Example";

        /* Create the map view */
        var view = new Shumate.View.simple ();
        window.set_child (view);

        /* Create the markers and marker layer */
        var layer = new DemoLayer (view.get_viewport (), out path_layer);
        view.add_layer (path_layer);
        view.add_layer (layer);

        /* Finish initialising the map view */
        view.get_viewport ().zoom_level = 7;
        view.kinetic_mode = true;
        view.center_on (45.466, -73.75);

        window.present ();
    }

    public static int main (string[] args) {
        var launcher = new Launcher ();
        return launcher.run (args);
    }
}

