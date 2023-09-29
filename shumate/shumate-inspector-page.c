/*
 * Copyright (C) 2023 James Westman <james@jwestman.net>
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

#include <glib/gi18n.h>

#include "shumate-inspector-page-private.h"
#include "shumate-inspector-settings-private.h"

struct _ShumateInspectorPage {
  GtkBox parent_instance;

  GObject *object;
};

G_DEFINE_FINAL_TYPE(ShumateInspectorPage, shumate_inspector_page, GTK_TYPE_BOX)

enum {
  PROP_0,
  PROP_TITLE,
  PROP_OBJECT,
  PROP_SETTINGS,
  N_PROPS
};

static GParamSpec *props[N_PROPS];

static void
shumate_inspector_page_dispose (GObject *object)
{
  /* When the inspector is closed, reset all the debug settings */
  shumate_inspector_settings_reset (shumate_inspector_settings_get_default ());

  G_OBJECT_CLASS (shumate_inspector_page_parent_class)->dispose (object);
}

static void
shumate_inspector_get_property (GObject *object,
                                guint prop_id,
                                GValue *value,
                                GParamSpec *pspec)
{
  ShumateInspectorPage *self = SHUMATE_INSPECTOR_PAGE (object);

  switch (prop_id)
    {
    case PROP_TITLE:
      g_value_set_string (value, _("Shumate"));
      break;
    case PROP_OBJECT:
      g_value_set_object (value, self->object);
      break;
    case PROP_SETTINGS:
      g_value_set_object (value, shumate_inspector_settings_get_default ());
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
shumate_inspector_set_property (GObject *object,
                                guint prop_id,
                                const GValue *value,
                                GParamSpec *pspec)
{
  ShumateInspectorPage *self = SHUMATE_INSPECTOR_PAGE (object);

  switch (prop_id)
    {
    case PROP_OBJECT:
      g_set_object (&self->object, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
on_show_debug_overlay (ShumateInspectorPage *self,
                       GParamSpec *pspec,
                       GtkSwitch *widget)
{
  ShumateInspectorSettings *settings = shumate_inspector_settings_get_default ();
  shumate_inspector_settings_set_show_debug_overlay (settings, gtk_switch_get_active (widget));
}

static void
on_show_tile_bounds (ShumateInspectorPage *self,
                     GParamSpec *pspec,
                     GtkSwitch *widget)
{
  ShumateInspectorSettings *settings = shumate_inspector_settings_get_default ();
  shumate_inspector_settings_set_show_tile_bounds (settings, gtk_switch_get_active (widget));
}

static void
on_show_collision_boxes (ShumateInspectorPage *self,
                         GParamSpec *pspec,
                         GtkSwitch *widget)
{
  ShumateInspectorSettings *settings = shumate_inspector_settings_get_default ();
  shumate_inspector_settings_set_show_collision_boxes (settings, gtk_switch_get_active (widget));
}

static void
shumate_inspector_page_class_init (ShumateInspectorPageClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = shumate_inspector_page_dispose;
  object_class->get_property = shumate_inspector_get_property;
  object_class->set_property = shumate_inspector_set_property;

  props[PROP_TITLE] =
    g_param_spec_string ("title",
                         "title",
                         "title",
                         NULL,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  props[PROP_OBJECT] =
    g_param_spec_object ("object",
                         "object",
                         "object",
                         G_TYPE_OBJECT,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  props[PROP_SETTINGS] =
    g_param_spec_object ("settings",
                         "settings",
                         "settings",
                         SHUMATE_TYPE_INSPECTOR_SETTINGS,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, props);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/shumate/shumate-inspector-page.ui");
  gtk_widget_class_bind_template_callback (widget_class, on_show_debug_overlay);
  gtk_widget_class_bind_template_callback (widget_class, on_show_tile_bounds);
  gtk_widget_class_bind_template_callback (widget_class, on_show_collision_boxes);
}

void
shumate_inspector_page_init (ShumateInspectorPage *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

static gboolean registered = FALSE;

void
shumate_inspector_page_ensure_registered (void)
{
  if (registered)
    return;

  if (g_io_extension_point_lookup ("gtk-inspector-page"))
    g_io_extension_point_implement ("gtk-inspector-page",
                                    SHUMATE_TYPE_INSPECTOR_PAGE,
                                    "libshumate",
                                    10);

  registered = TRUE;
}
