/*
 * Copyright (C) 2011-2013 Jiri Techet <techet@gmail.com>
 * Copyright (C) 2019 Marcus Lundblad <ml@update.uu.se>
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
 * ShumateScale:
 *
 * A widget displaying a scale.
 *
 * # CSS nodes
 *
 * ```
 * map-scale
 * ├── label[.metric][.imperial]
 * ```
 *
 * `ShumateScale` uses a single CSS node with name map-scale, it has up to two
 * childs different labels.
 */

#include "shumate-scale.h"
#include "shumate-enum-types.h"

#include <glib-object.h>
#include <glib/gi18n.h>
#include <math.h>
#include <string.h>

enum
{
  PROP_UNIT = 1,
  PROP_MAX_SCALE_WIDTH,
  PROP_VIEWPORT,
  N_PROPERTIES,
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

struct _ShumateScale
{
  GtkWidget parent_instance;

  ShumateUnit unit;
  guint max_scale_width;

  ShumateViewport *viewport;

  GtkWidget *metric_label;
  GtkWidget *imperial_label;
};

G_DEFINE_TYPE (ShumateScale, shumate_scale, GTK_TYPE_WIDGET);

#define FEET_IN_METERS 3.280839895

#define FEET_IN_A_MILE 5280.0

/*
 * shumate_scale_compute_length:
 * @self: a #ShumateScale
 *
 * This loop will find the pretty value to display on the scale.
 * It will be run once for metric units, and twice for imperials
 * so that both feet and miles have pretty numbers.
 */
static gboolean
shumate_scale_compute_length (ShumateScale *self,
                              ShumateUnit   unit,
                              float        *out_scale_width,
                              float        *out_base,
                              gboolean     *out_is_small_unit)
{
  ShumateMapSource *map_source;
  double zoom_level;
  double lat, lon;
  float scale_width;
  float base;
  float factor;
  gboolean is_small_unit = TRUE;
  double m_per_pixel;

  g_assert (SHUMATE_IS_SCALE (self));

  if (out_scale_width)
    *out_scale_width = 0;

  if (out_base)
    *out_base = 1;

  if (out_is_small_unit)
    *out_is_small_unit = TRUE;

  if (!self->viewport)
    return FALSE;

  scale_width = self->max_scale_width;
  zoom_level = shumate_viewport_get_zoom_level (self->viewport);
  map_source = shumate_viewport_get_reference_map_source (self->viewport);
  if (!map_source)
    return FALSE;

  lat = shumate_location_get_latitude (SHUMATE_LOCATION (self->viewport));
  lon = shumate_location_get_longitude (SHUMATE_LOCATION (self->viewport));
  m_per_pixel = shumate_map_source_get_meters_per_pixel (map_source,
                                                         zoom_level,
                                                         lat,
                                                         lon);

  if (unit == SHUMATE_UNIT_IMPERIAL)
    m_per_pixel *= FEET_IN_METERS;  /* m_per_pixel is now in ft */

  do
    {
      /* Keep the previous power of 10 */
      base = floor (log (m_per_pixel * scale_width) / log (10));
      base = pow (10, base);

      /* How many times can it be fitted in our max scale width */
      g_assert (base > 0);
      g_assert (m_per_pixel * scale_width / base > 0);
      scale_width /= m_per_pixel * scale_width / base;
      g_assert (scale_width > 0);
      factor = floor (self->max_scale_width / scale_width);
      base *= factor;
      scale_width *= factor;

      if (unit == SHUMATE_UNIT_METRIC)
        {
          if (base / 1000.0 >= 1)
            {
              base /= 1000.0; /* base is now in km */
              is_small_unit = FALSE;
            }

          break;
        }
      else if (unit == SHUMATE_UNIT_IMPERIAL)
        {
          if (is_small_unit && base / FEET_IN_A_MILE >= 1)
            {
              m_per_pixel /= FEET_IN_A_MILE; /* m_per_pixel is now in miles */
              is_small_unit = FALSE;
              /* we need to recompute the base because 1000 ft != 1 mile */
            }
          else
            break;
        }
    } while (TRUE);


  if (out_scale_width)
    *out_scale_width = scale_width;

  if (out_base)
    *out_base = base;

  if (out_is_small_unit)
    *out_is_small_unit = is_small_unit;

  return TRUE;
}

static void
shumate_scale_on_scale_changed (ShumateScale *self)
{
  float metric_scale_width, metric_base;
  float imperial_scale_width, imperial_base;
  gboolean metric_is_small_unit, imperial_is_small_unit;
  g_autofree char *metric_label = NULL;
  g_autofree char *imperial_label = NULL;

  shumate_scale_compute_length (self, SHUMATE_UNIT_METRIC, &metric_scale_width, &metric_base, &metric_is_small_unit);
  shumate_scale_compute_length (self, SHUMATE_UNIT_IMPERIAL, &imperial_scale_width, &imperial_base, &imperial_is_small_unit);

  gtk_widget_set_size_request (self->metric_label, metric_scale_width, -1);
  gtk_widget_set_size_request (self->imperial_label, imperial_scale_width, -1);

  if (metric_is_small_unit)
    // m is the unit for meters
    metric_label = g_strdup_printf (_("%d m"), (int) metric_base);
  else
    // km is the unit for kilometers
    metric_label = g_strdup_printf (_("%d km"), (int) metric_base);

  gtk_label_set_label (GTK_LABEL (self->metric_label), metric_label);

  if (imperial_is_small_unit)
    // ft is the unit for feet
    imperial_label = g_strdup_printf (_("%d ft"), (int) imperial_base);
  else
    // mi is the unit for miles
    imperial_label = g_strdup_printf (_("%d mi"), (int) imperial_base);

  gtk_label_set_label (GTK_LABEL (self->imperial_label), imperial_label);
  gtk_widget_queue_resize (GTK_WIDGET (self));
}

static void
on_viewport_props_changed (ShumateScale *self,
                           G_GNUC_UNUSED GParamSpec *pspec,
                           ShumateViewport *viewport)
{
  shumate_scale_on_scale_changed (self);
}


static void
shumate_scale_get_property (GObject *object,
    guint prop_id,
    GValue *value,
    GParamSpec *pspec)
{
  ShumateScale *scale = SHUMATE_SCALE (object);

  switch (prop_id)
    {
    case PROP_UNIT:
      g_value_set_enum (value, scale->unit);
      break;

    case PROP_MAX_SCALE_WIDTH:
      g_value_set_uint (value, scale->max_scale_width);
      break;

    case PROP_VIEWPORT:
      g_value_set_object (value, scale->viewport);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}


static void
shumate_scale_set_property (GObject *object,
    guint prop_id,
    const GValue *value,
    GParamSpec *pspec)
{
  ShumateScale *scale = SHUMATE_SCALE (object);

  switch (prop_id)
    {
    case PROP_UNIT:
      shumate_scale_set_unit (scale, g_value_get_enum (value));
      break;

    case PROP_MAX_SCALE_WIDTH:
      shumate_scale_set_max_width (scale, g_value_get_uint (value));
      break;

    case PROP_VIEWPORT:
      shumate_scale_set_viewport (scale, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
shumate_scale_dispose (GObject *object)
{
  ShumateScale *scale = SHUMATE_SCALE (object);

  g_signal_handlers_disconnect_by_data (scale->viewport, scale);
  g_clear_object (&scale->viewport);
  g_clear_pointer (&scale->metric_label, gtk_widget_unparent);
  g_clear_pointer (&scale->imperial_label, gtk_widget_unparent);

  G_OBJECT_CLASS (shumate_scale_parent_class)->dispose (object);
}

static void
shumate_scale_class_init (ShumateScaleClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GdkDisplay *display;

  object_class->dispose = shumate_scale_dispose;
  object_class->get_property = shumate_scale_get_property;
  object_class->set_property = shumate_scale_set_property;

  /**
   * ShumateScale:max-width:
   *
   * The size of the map scale on screen in pixels.
   */
  obj_properties[PROP_MAX_SCALE_WIDTH] =
    g_param_spec_uint ("max-width",
                       "The width of the scale",
                       "The max width of the scale on screen",
                       1, G_MAXUINT, 150,
                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * ShumateScale:unit:
   *
   * The scale's units.
   */
  obj_properties[PROP_UNIT] =
    g_param_spec_enum ("unit",
                       "The scale's unit",
                       "The map scale's unit",
                       SHUMATE_TYPE_UNIT,
                       SHUMATE_UNIT_BOTH,
                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * ShumateScale:viewport:
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

  gtk_widget_class_set_css_name (widget_class, "map-scale");
  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BOX_LAYOUT);

  display = gdk_display_get_default ();
  if (display)
    {
      g_autoptr(GtkCssProvider) provider = gtk_css_provider_new ();
      gtk_css_provider_load_from_resource (provider, "/org/gnome/shumate/scale.css");
      gtk_style_context_add_provider_for_display (display,
                                                  GTK_STYLE_PROVIDER (provider),
                                                  GTK_STYLE_PROVIDER_PRIORITY_FALLBACK);
    }
}

static void
shumate_scale_init (ShumateScale *self)
{
  GtkWidget *self_widget = GTK_WIDGET (self);
  GtkLayoutManager *layout = gtk_widget_get_layout_manager (self_widget);

  self->unit = SHUMATE_UNIT_BOTH;
  self->max_scale_width = 150;

  gtk_orientable_set_orientation (GTK_ORIENTABLE (layout), GTK_ORIENTATION_VERTICAL);
  gtk_widget_add_css_class (self_widget, "vertical");

  self->metric_label = gtk_label_new (NULL);
  g_object_set (G_OBJECT (self->metric_label),
                "xalign", 0.0f,
                "halign", GTK_ALIGN_START,
                "ellipsize", PANGO_ELLIPSIZE_END,
                NULL);
  gtk_widget_add_css_class (self->metric_label, "metric");
  self->imperial_label = gtk_label_new (NULL);
  g_object_set (G_OBJECT (self->imperial_label),
                "xalign", 0.0f,
                "halign", GTK_ALIGN_START,
                "ellipsize", PANGO_ELLIPSIZE_END,
                NULL);
  gtk_widget_add_css_class (self->imperial_label, "imperial");

  gtk_widget_insert_after (self->metric_label, self_widget, NULL);
  gtk_widget_insert_after (self->imperial_label, self_widget, self->metric_label);
}


/**
 * shumate_scale_new:
 * @viewport: (nullable): a #ShumateViewport
 *
 * Creates an instance of #ShumateScale.
 *
 * Returns: a new #ShumateScale.
 */
ShumateScale *
shumate_scale_new (ShumateViewport *viewport)
{
  return SHUMATE_SCALE (g_object_new (SHUMATE_TYPE_SCALE,
                                      "viewport", viewport,
                                      NULL));
}


/**
 * shumate_scale_set_max_width:
 * @scale: a #ShumateScale
 * @value: the number of pixels
 *
 * Sets the maximum width of the scale on the screen in pixels
 */
void
shumate_scale_set_max_width (ShumateScale *scale,
    guint value)
{
  g_return_if_fail (SHUMATE_IS_SCALE (scale));

  if (scale->max_scale_width == value)
    return;

  scale->max_scale_width = value;
  g_object_notify_by_pspec(G_OBJECT (scale), obj_properties[PROP_MAX_SCALE_WIDTH]);
  shumate_scale_on_scale_changed (scale);
}


/**
 * shumate_scale_set_unit:
 * @scale: a #ShumateScale
 * @unit: a #ShumateUnit
 *
 * Sets the scale unit.
 */
void
shumate_scale_set_unit (ShumateScale *scale,
                        ShumateUnit   unit)
{
  g_return_if_fail (SHUMATE_IS_SCALE (scale));

  if (scale->unit == unit)
    return;

  scale->unit = unit;

  gtk_widget_set_visible (scale->metric_label, unit == SHUMATE_UNIT_METRIC || unit == SHUMATE_UNIT_BOTH);
  gtk_widget_set_visible (scale->imperial_label, unit == SHUMATE_UNIT_IMPERIAL || unit == SHUMATE_UNIT_BOTH);

  g_object_notify_by_pspec(G_OBJECT (scale), obj_properties[PROP_UNIT]);
  shumate_scale_on_scale_changed (scale);
}

/**
 * shumate_scale_set_viewport:
 * @scale: a #ShumateScale
 * @viewport: (nullable): a #ShumateViewport
 *
 * Sets the scale viewport.
 */
void
shumate_scale_set_viewport (ShumateScale    *scale,
                            ShumateViewport *viewport)
{
  g_return_if_fail (SHUMATE_IS_SCALE (scale));

  if (scale->viewport)
    g_signal_handlers_disconnect_by_data (scale->viewport, scale);

  if (g_set_object (&scale->viewport, viewport))
    {
      g_object_notify_by_pspec(G_OBJECT (scale), obj_properties[PROP_VIEWPORT]);
      if (scale->viewport)
        {
          g_signal_connect_swapped (scale->viewport, "notify::latitude", G_CALLBACK (on_viewport_props_changed), scale);
          g_signal_connect_swapped (scale->viewport, "notify::zoom-level", G_CALLBACK (on_viewport_props_changed), scale);
          g_signal_connect_swapped (scale->viewport, "notify::reference-map-source", G_CALLBACK (on_viewport_props_changed), scale);
        }

      shumate_scale_on_scale_changed (scale);
    }
}


/**
 * shumate_scale_get_max_width:
 * @scale: a #ShumateScale
 *
 * Gets the maximum scale width.
 *
 * Returns: The maximum scale width in pixels.
 */
guint
shumate_scale_get_max_width (ShumateScale *scale)
{
  g_return_val_if_fail (SHUMATE_IS_SCALE (scale), FALSE);

  return scale->max_scale_width;
}


/**
 * shumate_scale_get_unit:
 * @scale: a #ShumateScale
 *
 * Gets the unit used by the scale.
 *
 * Returns: The unit used by the scale
 */
ShumateUnit
shumate_scale_get_unit (ShumateScale *scale)
{
  g_return_val_if_fail (SHUMATE_IS_SCALE (scale), FALSE);

  return scale->unit;
}

/**
 * shumate_scale_get_viewport:
 * @scale: a #ShumateScale
 *
 * Gets the viewport used by the scale.
 *
 * Returns: (transfer none) (nullable): The #ShumateViewport used by the scale
 */
ShumateViewport *
shumate_scale_get_viewport (ShumateScale *scale)
{
  g_return_val_if_fail (SHUMATE_IS_SCALE (scale), NULL);

  return scale->viewport;
}
