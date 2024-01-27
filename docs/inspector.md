Title: Inspector Page
Slug: inspector-page

# Shumate Inspector Page

Shumate adds a page to the [GTK Inspector](https://developer.gnome.org/documentation/tools/inspector.html) that allows you to toggle several debugging options.

The page only appears if the inspector is opened *after* the [class@Shumate.Map] class has been initialized, so if you launch the application with `GTK_DEBUG=interactive` you may need to close and reopen the inspector.

![The Shumate inspector page with three switches: "Show Debug Overlay", "Show Tile Bounds", and "Show Collision Boxes".](inspector.png)

## Show Debug Overlay

The debug overlay shows information about the viewport, tiles, and layers as an overlay over any Shumate map in the application.

Custom layer implementations can add their own debug information to the overlay by overriding [vfunc@Shumate.Layer.get_debug_text].

![A screenshot of the Shumate demo with debug text in the top left corner of the map.](inspector-overlay.png)

## Show Tile Bounds

When enabled, the grid of tiles that make up the map is drawn with a magenta outline. Each tile's
coordinates are shown in the top left corner, in "Z, X, Y" order.

![A screenshot of the Shumate demo with magenta outlines around each tile.](inspector-tile-bounds.png)

## Show Collision Boxes

When enabled, collision boxes are drawn for all visible symbols. Individual boxes have green outlines, and the buckets of the internal R-tree structure have red outlines. This is useful for debugging symbol placement and overlap detection while developing libshumate itself.

![A screenshot of the Shumate demo with green outlines around each label, with red outlines grouping multiple labels together.](inspector-collision.png)
