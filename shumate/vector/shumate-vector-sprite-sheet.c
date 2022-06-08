/*
 * Copyright (C) 2022 James Westman <james@jwestman.net>
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

#include <json-glib/json-glib.h>
#include "shumate-vector-sprite-sheet-private.h"
#include "shumate-vector-utils-private.h"

struct _ShumateVectorSpriteSheet
{
  GObject parent_instance;

  char *json;
  GdkPixbuf *pixbuf;
  GHashTable *sprites;
};

enum {
  PROP_0,
  PROP_PIXBUF,
  PROP_JSON,
  N_PROPS,
};

static GParamSpec *properties [N_PROPS];

static void shumate_vector_sprite_sheet_initable_iface_init (GInitableIface *iface);

G_DEFINE_FINAL_TYPE_WITH_CODE (ShumateVectorSpriteSheet, shumate_vector_sprite_sheet, G_TYPE_OBJECT,
                               G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, shumate_vector_sprite_sheet_initable_iface_init))


typedef struct {
  int x, y;
  int width, height;
  GdkPixbuf *pixbuf;
} Sprite;

static void
sprite_free (Sprite *sprite)
{
  g_clear_object (&sprite->pixbuf);
  g_free (sprite);
}


ShumateVectorSpriteSheet *
shumate_vector_sprite_sheet_new (GdkPixbuf     *pixbuf,
                                 const char    *json,
                                 GCancellable  *cancellable,
                                 GError       **error)
{
  return g_initable_new (SHUMATE_TYPE_VECTOR_SPRITE_SHEET, cancellable, error,
                         "json", json,
                         "pixbuf", pixbuf,
                         NULL);
}

static void
shumate_vector_sprite_sheet_get_property (GObject    *object,
                                          guint       prop_id,
                                          GValue     *value,
                                          GParamSpec *pspec)
{
  ShumateVectorSpriteSheet *self = SHUMATE_VECTOR_SPRITE_SHEET (object);

  switch (prop_id)
    {
    case PROP_PIXBUF:
      g_value_set_object (value, self->pixbuf);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
shumate_vector_sprite_sheet_set_property (GObject      *object,
                                          guint         prop_id,
                                          const GValue *value,
                                          GParamSpec   *pspec)
{
  ShumateVectorSpriteSheet *self = SHUMATE_VECTOR_SPRITE_SHEET (object);

  switch (prop_id)
    {
    case PROP_PIXBUF:
      /* Property is construct only, so it should only be set once */
      g_assert (self->pixbuf == NULL);
      self->pixbuf = g_value_dup_object (value);
      break;
    case PROP_JSON:
      /* Property is construct only, so it should only be set once */
      g_assert (self->json == NULL);
      self->json = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
shumate_vector_sprite_sheet_finalize (GObject *object)
{
  ShumateVectorSpriteSheet *self = (ShumateVectorSpriteSheet *)object;

  g_clear_pointer (&self->json, g_free);
  g_clear_object (&self->pixbuf);
  g_clear_pointer (&self->sprites, g_hash_table_unref);

  G_OBJECT_CLASS (shumate_vector_sprite_sheet_parent_class)->finalize (object);
}

static void
shumate_vector_sprite_sheet_class_init (ShumateVectorSpriteSheetClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = shumate_vector_sprite_sheet_finalize;
  object_class->get_property = shumate_vector_sprite_sheet_get_property;
  object_class->set_property = shumate_vector_sprite_sheet_set_property;

  properties[PROP_JSON] =
    g_param_spec_string ("json",
                         "JSON",
                         "JSON",
                         NULL,
                         G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  properties[PROP_PIXBUF] =
    g_param_spec_object ("pixbuf",
                         "Pixbuf",
                         "Pixbuf",
                         GDK_TYPE_PIXBUF,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static gboolean
shumate_vector_sprite_sheet_initable_init (GInitable     *initable,
                                           GCancellable  *cancellable,
                                           GError       **error)
{
  ShumateVectorSpriteSheet *self = (ShumateVectorSpriteSheet *)initable;
  g_autoptr(JsonNode) json_node = NULL;
  JsonObject *sprites;
  JsonObjectIter iter;
  const char *sprite_name;
  JsonNode *sprite_node;

  json_node = json_from_string (self->json, error);
  g_clear_pointer (&self->json, g_free);

  if (json_node == NULL)
    return FALSE;

  if (!shumate_vector_json_get_object (json_node, &sprites, error))
    return FALSE;

  json_object_iter_init (&iter, sprites);
  while (json_object_iter_next (&iter, &sprite_name, &sprite_node))
    {
      Sprite *sprite;
      JsonObject *sprite_object;

      if (!shumate_vector_json_get_object (sprite_node, &sprite_object, error))
        return FALSE;

      sprite = g_new0 (Sprite, 1);
      sprite->x = json_object_get_int_member_with_default (sprite_object, "x", 0);
      sprite->y = json_object_get_int_member_with_default (sprite_object, "y", 0);
      sprite->width = json_object_get_int_member_with_default (sprite_object, "width", 0);
      sprite->height = json_object_get_int_member_with_default (sprite_object, "height", 0);

      g_hash_table_insert (self->sprites, g_strdup (sprite_name), sprite);
    }

  return TRUE;
}

static void
shumate_vector_sprite_sheet_initable_iface_init (GInitableIface *iface)
{
  iface->init = shumate_vector_sprite_sheet_initable_init;
}

static void
shumate_vector_sprite_sheet_init (ShumateVectorSpriteSheet *self)
{
  self->sprites = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, (GDestroyNotify)sprite_free);
}

/*< private >
 * shumate_vector_sprite_sheet_get_icon:
 * @self: a [class@VectorSpriteSheet]
 * @name: an icon name
 *
 * Gets an icon's image from the spritesheet.
 *
 * Returns: (transfer none) (nullable): a [class@Gdk.Pixbuf], or %NULL if the
 * icon does not exist.
 */
GdkPixbuf *
shumate_vector_sprite_sheet_get_icon (ShumateVectorSpriteSheet *self,
                                      const char               *name)
{
  Sprite *sprite;

  sprite = g_hash_table_lookup (self->sprites, name);
  if (sprite == NULL)
    return NULL;

  if (sprite->pixbuf != NULL)
    return sprite->pixbuf;

  sprite->pixbuf = gdk_pixbuf_new_subpixbuf (self->pixbuf,
                                             sprite->x,
                                             sprite->y,
                                             sprite->width,
                                             sprite->height);

  return sprite->pixbuf;
}
