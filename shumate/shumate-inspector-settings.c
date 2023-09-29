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

#include "shumate-inspector-settings-private.h"

struct _ShumateInspectorSettings {
  GObject parent;

  gboolean show_debug_overlay : 1;
  gboolean show_tile_bounds : 1;
  gboolean show_collision_boxes : 1;
};

G_DEFINE_TYPE(ShumateInspectorSettings, shumate_inspector_settings, G_TYPE_OBJECT)

enum {
  PROP_0,
  PROP_SHOW_DEBUG_OVERLAY,
  PROP_SHOW_TILE_BOUNDS,
  PROP_SHOW_COLLISION_BOXES,
  PROP_LAST
};

static GParamSpec *properties[PROP_LAST];

static ShumateInspectorSettings *default_settings = NULL;

ShumateInspectorSettings *
shumate_inspector_settings_get_default (void)
{
  if (G_UNLIKELY (default_settings == NULL))
    default_settings = g_object_new (SHUMATE_TYPE_INSPECTOR_SETTINGS, NULL);

  return default_settings;
}

static void
shumate_inspector_settings_get_property (GObject *object,
                                         guint prop_id,
                                         GValue *value,
                                         GParamSpec *pspec)
{
  ShumateInspectorSettings *self = SHUMATE_INSPECTOR_SETTINGS (object);

  switch (prop_id)
    {
    case PROP_SHOW_DEBUG_OVERLAY:
      g_value_set_boolean (value, self->show_debug_overlay);
      break;
    case PROP_SHOW_TILE_BOUNDS:
      g_value_set_boolean (value, self->show_tile_bounds);
      break;
    case PROP_SHOW_COLLISION_BOXES:
      g_value_set_boolean (value, self->show_collision_boxes);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
shumate_inspector_settings_set_property (GObject *object,
                                         guint prop_id,
                                         const GValue *value,
                                         GParamSpec *pspec)
{
  ShumateInspectorSettings *self = SHUMATE_INSPECTOR_SETTINGS (object);

  switch (prop_id)
    {
    case PROP_SHOW_DEBUG_OVERLAY:
      shumate_inspector_settings_set_show_debug_overlay (self, g_value_get_boolean (value));
      break;
    case PROP_SHOW_TILE_BOUNDS:
      shumate_inspector_settings_set_show_tile_bounds (self, g_value_get_boolean (value));
      break;
    case PROP_SHOW_COLLISION_BOXES:
      shumate_inspector_settings_set_show_collision_boxes (self, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
shumate_inspector_settings_class_init (ShumateInspectorSettingsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = shumate_inspector_settings_get_property;
  object_class->set_property = shumate_inspector_settings_set_property;

  properties[PROP_SHOW_DEBUG_OVERLAY] =
    g_param_spec_boolean ("show-debug-overlay",
                          "show-debug-overlay",
                          "show-debug-overlay",
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  properties[PROP_SHOW_TILE_BOUNDS] =
    g_param_spec_boolean ("show-tile-bounds",
                          "show-tile-bounds",
                          "show-tile-bounds",
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  properties[PROP_SHOW_COLLISION_BOXES] =
    g_param_spec_boolean ("show-collision-boxes",
                          "show-collision-boxes",
                          "show-collision-boxes",
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST, properties);
}

static void
shumate_inspector_settings_init (ShumateInspectorSettings *self)
{
}

void
shumate_inspector_settings_reset (ShumateInspectorSettings *self)
{
  g_return_if_fail (SHUMATE_IS_INSPECTOR_SETTINGS (self));

  shumate_inspector_settings_set_show_debug_overlay (self, FALSE);
  shumate_inspector_settings_set_show_tile_bounds (self, FALSE);
  shumate_inspector_settings_set_show_collision_boxes (self, FALSE);
}

gboolean
shumate_inspector_settings_get_show_debug_overlay (ShumateInspectorSettings *self)
{
  g_return_val_if_fail (SHUMATE_IS_INSPECTOR_SETTINGS (self), FALSE);
  return self->show_debug_overlay;
}

void
shumate_inspector_settings_set_show_debug_overlay (ShumateInspectorSettings *self,
                                                   gboolean show_debug_overlay)
{
  g_return_if_fail (SHUMATE_IS_INSPECTOR_SETTINGS (self));

  show_debug_overlay = !!show_debug_overlay;

  if (self->show_debug_overlay != show_debug_overlay)
    {
      self->show_debug_overlay = show_debug_overlay;
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SHOW_DEBUG_OVERLAY]);
    }
}

gboolean
shumate_inspector_settings_get_show_tile_bounds (ShumateInspectorSettings *self)
{
  g_return_val_if_fail (SHUMATE_IS_INSPECTOR_SETTINGS (self), FALSE);
  return self->show_tile_bounds;
}

void
shumate_inspector_settings_set_show_tile_bounds (ShumateInspectorSettings *self,
                                                 gboolean show_tile_bounds)
{
  g_return_if_fail (SHUMATE_IS_INSPECTOR_SETTINGS (self));

  show_tile_bounds = !!show_tile_bounds;

  if (self->show_tile_bounds != show_tile_bounds)
    {
      self->show_tile_bounds = show_tile_bounds;
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SHOW_TILE_BOUNDS]);
    }
}

gboolean
shumate_inspector_settings_get_show_collision_boxes (ShumateInspectorSettings *self)
{
  g_return_val_if_fail (SHUMATE_IS_INSPECTOR_SETTINGS (self), FALSE);
  return self->show_collision_boxes;
}

void
shumate_inspector_settings_set_show_collision_boxes (ShumateInspectorSettings *self,
                                                     gboolean show_collision_boxes)
{
  g_return_if_fail (SHUMATE_IS_INSPECTOR_SETTINGS (self));

  show_collision_boxes = !!show_collision_boxes;

  if (self->show_collision_boxes != show_collision_boxes)
    {
      self->show_collision_boxes = show_collision_boxes;
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SHOW_COLLISION_BOXES]);
    }
}
