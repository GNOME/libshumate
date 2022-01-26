/*
 * Copyright (C) 2020 Collabora Ltd. (https://www.collabora.com)
 * Copyright (C) 2020 Corentin Noël <corentin.noel@collabora.com>
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
 * ShumateCompass:
 *
 * A widget displaying a compass.
 *
 * # CSS nodes
 *
 * ```
 * map-compass
 * ├── revealer
 * ├──── image
 * ```
 *
 * `ShumateCompass` uses a single CSS node with name map-compass. It also uses an
 * image named "map-compass".
 */

#include "shumate-compass.h"

struct _ShumateCompass
{
  GtkWidget parent_instance;

  ShumateViewport *viewport;
  GtkWidget *revealer;
  GtkWidget *image;
  double rotation;
};

G_DEFINE_TYPE (ShumateCompass, shumate_compass, GTK_TYPE_WIDGET)

enum
{
  PROP_VIEWPORT = 1,
  N_PROPERTIES,
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static void
shumate_compass_on_rotation_changed (ShumateCompass *self)
{
  double rotation = shumate_viewport_get_rotation (self->viewport);
  if (rotation != 0)
    {
      self->rotation = rotation;
      gtk_widget_queue_draw (GTK_WIDGET (self));
    }

  gtk_revealer_set_reveal_child (GTK_REVEALER (self->revealer), rotation != 0);
}

static void
on_viewport_props_changed (ShumateCompass *self,
                           G_GNUC_UNUSED GParamSpec *pspec,
                           ShumateViewport *viewport)
{
  shumate_compass_on_rotation_changed (self);
}

static void
shumate_compass_dispose (GObject *object)
{
  ShumateCompass *self = (ShumateCompass *)object;

  g_clear_object (&self->viewport);
  g_clear_pointer (&self->revealer, gtk_widget_unparent);

  G_OBJECT_CLASS (shumate_compass_parent_class)->dispose (object);
}

static void
shumate_compass_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  ShumateCompass *self = SHUMATE_COMPASS (object);

  switch (prop_id)
    {
    case PROP_VIEWPORT:
      g_value_set_object (value, self->viewport);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
shumate_compass_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  ShumateCompass *self = SHUMATE_COMPASS (object);

  switch (prop_id)
    {
    case PROP_VIEWPORT:
      shumate_compass_set_viewport (self, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
shumate_compass_snapshot (GtkWidget   *widget,
                          GtkSnapshot *snapshot)
{
  ShumateCompass *self = SHUMATE_COMPASS (widget);
  graphene_point_t p;

  p.x = gtk_widget_get_width (widget) / 2;
  p.y = gtk_widget_get_height (widget) / 2;

  gtk_snapshot_save (snapshot);
  gtk_snapshot_translate (snapshot, &p);
  gtk_snapshot_rotate (snapshot, self->rotation * 180 / G_PI);
  p.x = -p.x;
  p.y = -p.y;
  gtk_snapshot_translate (snapshot, &p);
  GTK_WIDGET_CLASS (shumate_compass_parent_class)->snapshot (widget, snapshot);
  gtk_snapshot_restore (snapshot);
}

static void
shumate_compass_class_init (ShumateCompassClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GdkDisplay *display;

  object_class->dispose = shumate_compass_dispose;
  object_class->get_property = shumate_compass_get_property;
  object_class->set_property = shumate_compass_set_property;

  widget_class->snapshot = shumate_compass_snapshot;

  /**
   * ShumateCompass:viewport:
   *
   * The viewport to use.
   */
  obj_properties[PROP_VIEWPORT] =
    g_param_spec_object ("viewport",
                         "The viewport",
                         "The viewport",
                         SHUMATE_TYPE_VIEWPORT,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPERTIES, obj_properties);

  gtk_widget_class_set_css_name (widget_class, "map-compass");
  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);

  display = gdk_display_get_default ();
  if (display)
    {
      GtkIconTheme *icon_theme = gtk_icon_theme_get_for_display (display);
      gtk_icon_theme_add_resource_path (icon_theme, "/org/gnome/shumate/icons");
    }
}

static void
shumate_compass_init (ShumateCompass *self)
{
  self->revealer = gtk_revealer_new ();
  gtk_revealer_set_transition_type (GTK_REVEALER (self->revealer), GTK_REVEALER_TRANSITION_TYPE_CROSSFADE);
  gtk_widget_insert_after (self->revealer, GTK_WIDGET (self), NULL);
  self->image = gtk_image_new_from_icon_name ("map-compass");
  gtk_image_set_icon_size (GTK_IMAGE (self->image), GTK_ICON_SIZE_LARGE);
  gtk_revealer_set_child (GTK_REVEALER (self->revealer), self->image);
}

/**
 * shumate_compass_new:
 * @viewport: (nullable): a #ShumateViewport
 *
 * Creates an instance of #ShumateCompass.
 *
 * Returns: a new #ShumateCompass.
 */
ShumateCompass *
shumate_compass_new (ShumateViewport *viewport)
{
  return SHUMATE_COMPASS (g_object_new (SHUMATE_TYPE_COMPASS,
                                        "viewport", viewport,
                                        NULL));
}

/**
 * shumate_compass_set_viewport:
 * @compass: a [class@Compass]
 * @viewport: (nullable): a [class@Viewport]
 *
 * Sets the compass viewport.
 */
void
shumate_compass_set_viewport (ShumateCompass  *compass,
                              ShumateViewport *viewport)
{
  g_return_if_fail (SHUMATE_IS_COMPASS (compass));
  g_return_if_fail (viewport == NULL || SHUMATE_IS_VIEWPORT (viewport));

  if (compass->viewport == viewport)
    return;

  if (compass->viewport)
    g_signal_handlers_disconnect_by_data (compass->viewport, compass);

  g_set_object (&compass->viewport, viewport);

  if (compass->viewport)
    {
      g_signal_connect_swapped (compass->viewport, "notify::rotation", G_CALLBACK (on_viewport_props_changed), compass);
      shumate_compass_on_rotation_changed (compass);
    }

  g_object_notify_by_pspec (G_OBJECT (compass), obj_properties[PROP_VIEWPORT]);
}

/**
 * shumate_compass_get_viewport:
 * @compass: a #ShumateCompass
 *
 * Gets the viewport used by the compass.
 *
 * Returns: (transfer none) (nullable): The #ShumateViewport used by the compass
 */
ShumateViewport *
shumate_compass_get_viewport (ShumateCompass *compass)
{
  g_return_val_if_fail (SHUMATE_IS_COMPASS (compass), NULL);

  return compass->viewport;
}
