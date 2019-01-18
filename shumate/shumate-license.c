/*
 * Copyright (C) 2011-2013 Jiri Techet <techet@gmail.com>
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
 * SECTION:shumate-license
 * @short_description: An actor that displays license text.
 *
 * An actor that displays license text.
 */

#include "config.h"

#include "shumate-license.h"
#include "shumate-defines.h"
#include "shumate-marshal.h"
#include "shumate-private.h"
#include "shumate-enum-types.h"
#include "shumate-view.h"

#include <clutter/clutter.h>
#include <glib.h>
#include <glib-object.h>
#include <cairo.h>
#include <math.h>
#include <string.h>


enum
{
  /* normal signals */
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_LICENSE_EXTRA,
  PROP_ALIGNMENT,
};

/* static guint shumate_license_signals[LAST_SIGNAL] = { 0, }; */

struct _ShumateLicensePrivate
{
  gchar *extra_text; /* Extra license text */
  ClutterActor *license_actor;
  PangoAlignment alignment;

  ShumateView *view;
};

G_DEFINE_TYPE (ShumateLicense, shumate_license, CLUTTER_TYPE_ACTOR);

#define GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), SHUMATE_TYPE_LICENSE, ShumateLicensePrivate))

#define WIDTH_PADDING 10
#define HEIGHT_PADDING 7


static void
shumate_license_get_property (GObject *object,
    guint prop_id,
    GValue *value,
    GParamSpec *pspec)
{
  ShumateLicensePrivate *priv = SHUMATE_LICENSE (object)->priv;

  switch (prop_id)
    {
    case PROP_LICENSE_EXTRA:
      g_value_set_string (value, priv->extra_text);
      break;

    case PROP_ALIGNMENT:
      g_value_set_enum (value, priv->alignment);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}


static void
shumate_license_set_property (GObject *object,
    guint prop_id,
    const GValue *value,
    GParamSpec *pspec)
{
  ShumateLicense *license = SHUMATE_LICENSE (object);

  switch (prop_id)
    {
    case PROP_LICENSE_EXTRA:
      shumate_license_set_extra_text (license, g_value_get_string (value));
      break;

    case PROP_ALIGNMENT:
      shumate_license_set_alignment (license, g_value_get_enum (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}


static void
redraw_license (ShumateLicense *license)
{
  ShumateLicensePrivate *priv = license->priv;
  gchar *text;
  gfloat width, height;
  ShumateMapSource *map_source;
  GList *overlay_sources, *iter;

  if (!priv->view)
    return;

  map_source = shumate_view_get_map_source (priv->view);

  if (!map_source)
    return;

  if (priv->extra_text)
    text = g_strjoin ("\n",
          priv->extra_text,
          shumate_map_source_get_license (map_source),
          NULL);
  else
    text = g_strdup (shumate_map_source_get_license (map_source));

  overlay_sources = shumate_view_get_overlay_sources (priv->view);
  for (iter = overlay_sources; iter; iter = iter->next)
    {
      ShumateMapSource *map_source = iter->data;
      const gchar *overlay_license = shumate_map_source_get_license (map_source);

      if (g_strrstr (text, overlay_license) == NULL)
        {
          gchar *old_text = text;
          text = g_strjoin ("\n",
                text,
                shumate_map_source_get_license (map_source),
                NULL);
          g_free (old_text);
        }
    }
  g_list_free (overlay_sources);

  clutter_text_set_text (CLUTTER_TEXT (priv->license_actor), text);
  clutter_actor_get_size (priv->license_actor, &width, &height);
  clutter_actor_set_size (CLUTTER_ACTOR (license), width + 2 * WIDTH_PADDING, height + 2 * HEIGHT_PADDING);
  clutter_actor_set_position (priv->license_actor, WIDTH_PADDING, HEIGHT_PADDING);

  g_free (text);
}


static void
redraw_license_cb (G_GNUC_UNUSED GObject *gobject,
    G_GNUC_UNUSED GParamSpec *arg1,
    ShumateLicense *license)
{
  redraw_license (license);
}


static void
shumate_license_dispose (GObject *object)
{
  ShumateLicensePrivate *priv = SHUMATE_LICENSE (object)->priv;

  priv->license_actor = NULL;

  if (priv->view)
    {
      shumate_license_disconnect_view (SHUMATE_LICENSE (object));
      priv->view = NULL;
    }

  G_OBJECT_CLASS (shumate_license_parent_class)->dispose (object);
}


static void
shumate_license_finalize (GObject *object)
{
  ShumateLicensePrivate *priv = SHUMATE_LICENSE (object)->priv;

  g_free (priv->extra_text);

  G_OBJECT_CLASS (shumate_license_parent_class)->finalize (object);
}


static void
shumate_license_class_init (ShumateLicenseClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (ShumateLicensePrivate));

  object_class->finalize = shumate_license_finalize;
  object_class->dispose = shumate_license_dispose;
  object_class->get_property = shumate_license_get_property;
  object_class->set_property = shumate_license_set_property;

  /**
   * ShumateLicense:extra-text:
   *
   * Sets additional text to be displayed in the license area.  The map's
   * license will be added below it. Your text can have multiple lines, just use
   * "\n" in between.
   *
   * Since: 0.10
   */
  g_object_class_install_property (object_class,
      PROP_LICENSE_EXTRA,
      g_param_spec_string ("extra-text",
          "Additional license",
          "Additional license text",
          "",
          SHUMATE_PARAM_READWRITE));

  /**
   * ShumateLicense:alignment:
   *
   * The license's alignment
   *
   * Since: 0.10
   */
  g_object_class_install_property (object_class,
      PROP_ALIGNMENT,
      g_param_spec_enum ("alignment",
          "Alignment",
          "The license's alignment",
          PANGO_TYPE_ALIGNMENT,
          PANGO_ALIGN_LEFT,
          SHUMATE_PARAM_READWRITE));
}


static void
shumate_license_init (ShumateLicense *license)
{
  ShumateLicensePrivate *priv = GET_PRIVATE (license);

  license->priv = priv;
  priv->extra_text = NULL;
  priv->view = NULL;
  priv->alignment = PANGO_ALIGN_RIGHT;

  priv->license_actor = clutter_text_new ();
  clutter_text_set_font_name (CLUTTER_TEXT (priv->license_actor), "sans 8");
  clutter_text_set_line_alignment (CLUTTER_TEXT (priv->license_actor), priv->alignment);
  clutter_actor_set_opacity (priv->license_actor, 128);
  clutter_actor_add_child (CLUTTER_ACTOR (license), priv->license_actor);
}


/**
 * shumate_license_new:
 *
 * Creates an instance of #ShumateLicense.
 *
 * Returns: a new #ShumateLicense.
 *
 * Since: 0.10
 */
ClutterActor *
shumate_license_new (void)
{
  return CLUTTER_ACTOR (g_object_new (SHUMATE_TYPE_LICENSE, NULL));
}


/**
 * shumate_license_connect_view:
 * @license: The license
 * @view: a #ShumateView
 *
 * This method connects to the necessary signals of #ShumateView to make the
 * license change automatically when the map source changes.
 *
 * Since: 0.10
 */
void
shumate_license_connect_view (ShumateLicense *license,
    ShumateView *view)
{
  g_return_if_fail (SHUMATE_IS_LICENSE (license));

  license->priv->view = g_object_ref (view);

  g_signal_connect (view, "notify::map-source",
      G_CALLBACK (redraw_license_cb), license);
  g_signal_connect (view, "notify::width",
      G_CALLBACK (redraw_license_cb), license);
  g_signal_connect (view, "notify::height",
      G_CALLBACK (redraw_license_cb), license);
  redraw_license (license);
}


/**
 * shumate_license_disconnect_view:
 * @license: The license
 *
 * This method disconnects from the signals previously connected by shumate_license_connect_view().
 *
 * Since: 0.10
 */
void
shumate_license_disconnect_view (ShumateLicense *license)
{
  g_return_if_fail (SHUMATE_IS_LICENSE (license));

  g_signal_handlers_disconnect_by_func (license->priv->view,
      redraw_license_cb,
      license);
  g_object_unref (license->priv->view);
  license->priv->view = NULL;
}


/**
 * shumate_license_set_extra_text:
 * @license: a #ShumateLicense
 * @text: the additional license text
 *
 * Show the additional license text on the map view.  The text will preceed the
 * map's licence when displayed. Use "\n" to separate the lines.
 *
 * Since: 0.10
 */
void
shumate_license_set_extra_text (ShumateLicense *license,
    const gchar *text)
{
  g_return_if_fail (SHUMATE_IS_LICENSE (license));

  ShumateLicensePrivate *priv = license->priv;

  if (priv->extra_text)
    g_free (priv->extra_text);

  priv->extra_text = g_strdup (text);
  g_object_notify (G_OBJECT (license), "extra-text");
  redraw_license (license);
}


/**
 * shumate_license_get_extra_text:
 * @license: a #ShumateLicense
 *
 * Gets the additional license text.
 *
 * Returns: the additional license text
 *
 * Since: 0.10
 */
const gchar *
shumate_license_get_extra_text (ShumateLicense *license)
{
  g_return_val_if_fail (SHUMATE_IS_LICENSE (license), FALSE);

  return license->priv->extra_text;
}


/**
 * shumate_license_set_alignment:
 * @license: a #ShumateLicense
 * @alignment: The license's text alignment
 *
 * Set the license's text alignment.
 *
 * Since: 0.10
 */
void
shumate_license_set_alignment (ShumateLicense *license,
    PangoAlignment alignment)
{
  g_return_if_fail (SHUMATE_IS_LICENSE (license));

  license->priv->alignment = alignment;
  clutter_text_set_line_alignment (CLUTTER_TEXT (license->priv->license_actor), alignment);
  g_object_notify (G_OBJECT (license), "alignment");
}


/**
 * shumate_license_get_alignment:
 * @license: The license
 *
 * Get the license's text alignment.
 *
 * Returns: the license's text alignment.
 *
 * Since: 0.10
 */
PangoAlignment
shumate_license_get_alignment (ShumateLicense *license)
{
  g_return_val_if_fail (SHUMATE_IS_LICENSE (license), FALSE);

  return license->priv->alignment;
}
