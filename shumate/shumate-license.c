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
 * ShumateLicense:
 *
 * A widget that displays license text.
 */

#include "shumate-license.h"

enum
{
  PROP_EXTRA_TEXT = 1,
  PROP_XALIGN,
  PROP_MAP,
  N_PROPERTIES,
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

struct _ShumateLicense
{
  GtkWidget parent_instance;

  GtkWidget *extra_text_label;
  GtkWidget *license_label;

  ShumateMap *map;
};

G_DEFINE_TYPE (ShumateLicense, shumate_license, GTK_TYPE_WIDGET);

static void
shumate_license_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  ShumateLicense *self = SHUMATE_LICENSE (object);

  switch (prop_id)
    {
    case PROP_EXTRA_TEXT:
      g_value_set_string (value, gtk_label_get_label (GTK_LABEL (self->extra_text_label)));
      break;

    case PROP_XALIGN:
      g_value_set_float (value, gtk_label_get_xalign (GTK_LABEL (self->license_label)));
      break;

    case PROP_MAP:
      g_value_set_object (value, shumate_license_get_map (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}


static void
shumate_license_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  ShumateLicense *license = SHUMATE_LICENSE (object);

  switch (prop_id)
    {
    case PROP_EXTRA_TEXT:
      shumate_license_set_extra_text (license, g_value_get_string (value));
      break;

    case PROP_XALIGN:
      shumate_license_set_xalign (license, g_value_get_float (value));
      break;

    case PROP_MAP:
      shumate_license_set_map (license, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
shumate_license_sources_changed (ShumateLicense *self)
{
  g_autoptr(GString) license_str = NULL;
  g_autoptr(GList) layers = NULL;
  GList *l;

  g_assert (SHUMATE_IS_LICENSE (self));

  license_str = g_string_new (NULL);

  if (self->map)
    layers = shumate_map_get_layers (self->map);

  for (l = layers; l != NULL; l = l->next)
    {
      ShumateLayer *layer = SHUMATE_LAYER (l->data);
      const char *license = shumate_layer_get_license (layer);

      if (license == NULL)
        continue;

      if (license_str->len == 0)
        g_string_append (license_str, license);
      else
        g_string_append_printf (license_str, "\n%s", license);
    }

  gtk_label_set_label (GTK_LABEL (self->license_label), license_str->str);
}

static void
shumate_license_dispose (GObject *object)
{
  ShumateLicense *self = SHUMATE_LICENSE (object);

  g_clear_pointer (&self->extra_text_label, gtk_widget_unparent);
  g_clear_pointer (&self->license_label, gtk_widget_unparent);
  g_clear_object (&self->map);

  G_OBJECT_CLASS (shumate_license_parent_class)->dispose (object);
}

static void
shumate_license_class_init (ShumateLicenseClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GdkDisplay *display;

  object_class->dispose = shumate_license_dispose;
  object_class->get_property = shumate_license_get_property;
  object_class->set_property = shumate_license_set_property;

  /**
   * ShumateLicense:extra-text:
   *
   * Sets additional text to be displayed in the license area.  The map's
   * license will be added below it. Your text can have multiple lines, just use
   * "\n" in between.
   */
  obj_properties[PROP_EXTRA_TEXT] =
    g_param_spec_string ("extra-text",
                         "Additional license",
                         "Additional license text",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * ShumateLicense:xalign:
   *
   * The license's horizontal alignment
   */
  obj_properties[PROP_XALIGN] =
    g_param_spec_float ("xalign",
                        "Horizontal Alignment",
                        "X alignment of the child",
                        0.0, 1.0, 0.5,
                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * ShumateLicense:map:
   *
   * The map to show license information for
   */
  obj_properties[PROP_MAP] =
    g_param_spec_object ("map",
                         "Map",
                         "Map",
                         SHUMATE_TYPE_MAP,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, N_PROPERTIES, obj_properties);

  gtk_widget_class_set_css_name (widget_class, g_intern_static_string ("map-license"));
  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BOX_LAYOUT);

  display = gdk_display_get_default ();
  if (display)
    {
      g_autoptr(GtkCssProvider) provider = gtk_css_provider_new ();
      gtk_css_provider_load_from_resource (provider, "/org/gnome/shumate/license.css");
      gtk_style_context_add_provider_for_display (display,
                                                  GTK_STYLE_PROVIDER (provider),
                                                  GTK_STYLE_PROVIDER_PRIORITY_FALLBACK);
    }
}


static void
shumate_license_init (ShumateLicense *self)
{
  self->license_label = gtk_label_new (NULL);
  self->extra_text_label = gtk_label_new (NULL);

  g_object_set (gtk_widget_get_layout_manager (GTK_WIDGET (self)),
                "orientation", GTK_ORIENTATION_VERTICAL,
                NULL);
  g_object_set (self->license_label,
                "wrap",   TRUE,
                "xalign", 1.0f,
                NULL);
  g_object_set (self->extra_text_label,
                "visible", FALSE,
                "wrap",   TRUE,
                "xalign", 1.0f,
                NULL);

  gtk_widget_insert_after (self->license_label, GTK_WIDGET (self), NULL);
  gtk_widget_insert_after (self->extra_text_label, GTK_WIDGET (self), self->license_label);
}


/**
 * shumate_license_new:
 *
 * Creates an instance of #ShumateLicense.
 *
 * Returns: a new #ShumateLicense.
 */
ShumateLicense *
shumate_license_new (void)
{
  return SHUMATE_LICENSE (g_object_new (SHUMATE_TYPE_LICENSE, NULL));
}

/**
 * shumate_license_set_extra_text:
 * @license: a #ShumateLicense
 * @text: the additional license text
 *
 * Show the additional license text on the map view.  The text will preceed the
 * map's licence when displayed. Use "\n" to separate the lines.
 */
void
shumate_license_set_extra_text (ShumateLicense *license,
                                const char    *text)
{
  g_return_if_fail (SHUMATE_IS_LICENSE (license));

  gtk_label_set_label (GTK_LABEL (license->extra_text_label), text);
  gtk_widget_set_visible (license->extra_text_label, text != NULL);

  g_object_notify_by_pspec (G_OBJECT (license), obj_properties[PROP_EXTRA_TEXT]);
}


/**
 * shumate_license_get_extra_text:
 * @license: a #ShumateLicense
 *
 * Gets the additional license text.
 *
 * Returns: the additional license text
 */
const char *
shumate_license_get_extra_text (ShumateLicense *license)
{
  g_return_val_if_fail (SHUMATE_IS_LICENSE (license), NULL);

  return gtk_label_get_label (GTK_LABEL (license->extra_text_label));
}


/**
 * shumate_license_set_xalign:
 * @license: a #ShumateLicense
 * @xalign: The license's text horizontal alignment
 *
 * Set the license's text horizontal alignment.
 */
void
shumate_license_set_xalign (ShumateLicense *license,
                            float           xalign)
{
  g_return_if_fail (SHUMATE_IS_LICENSE (license));

  gtk_label_set_xalign (GTK_LABEL (license->license_label), xalign);
  gtk_label_set_xalign (GTK_LABEL (license->extra_text_label), xalign);

  g_object_notify_by_pspec (G_OBJECT (license), obj_properties[PROP_XALIGN]);
}


/**
 * shumate_license_get_xalign:
 * @license: The license
 *
 * Get the license's text horizontal alignment.
 *
 * Returns: the license's text horizontal alignment.
 */
float
shumate_license_get_xalign (ShumateLicense *license)
{
  g_return_val_if_fail (SHUMATE_IS_LICENSE (license), 1.0f);

  return gtk_label_get_xalign (GTK_LABEL (license->license_label));
}

/**
 * shumate_license_get_map:
 * @self: a [class@License]
 *
 * Gets the map that the license is showing information for.
 *
 * Returns: (nullable)(transfer none): the map
 */
ShumateMap *
shumate_license_get_map (ShumateLicense *self)
{
  g_return_val_if_fail (SHUMATE_IS_LICENSE (self), NULL);

  return self->map;
}

static void
on_layers_changed (ShumateLicense *self)
{
  shumate_license_sources_changed (self);
}

/**
 * shumate_license_set_map:
 * @self: a [class@License]
 * @map: (nullable): a [class@Map]
 *
 * Sets a map widget to show license information for. The license text will be
 * collected from the map's layers, if they provide it.
 */
void
shumate_license_set_map (ShumateLicense *self,
                         ShumateMap     *map)
{
  g_return_if_fail (SHUMATE_IS_LICENSE (self));
  g_return_if_fail (map == NULL || SHUMATE_IS_MAP (map));

  if (self->map == map)
    return;

  if (self->map != NULL)
    g_signal_handlers_disconnect_by_data (self->map, self);

  g_set_object (&self->map, map);

  if (self->map != NULL)
    g_signal_connect_object (self->map, "layers-changed", (GCallback)on_layers_changed, G_OBJECT (self), G_CONNECT_SWAPPED);

  shumate_license_sources_changed (self);

  g_object_notify_by_pspec (G_OBJECT (self), obj_properties[PROP_MAP]);
}
