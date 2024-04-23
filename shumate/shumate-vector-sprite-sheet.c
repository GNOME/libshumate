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

#include "shumate-vector-sprite-sheet.h"
#include "shumate-vector-renderer.h"

#ifdef SHUMATE_HAS_VECTOR_RENDERER
#include <json-glib/json-glib.h>
#include "vector/shumate-vector-utils-private.h"
#endif

/**
 * ShumateVectorSpriteSheet:
 *
 * A collection of [class@VectorSprite]s.
 *
 * Sprites are used as icons in symbols or as the pattern for a fill layer.
 *
 * Most MapLibre stylesheets provide their spritesheet as a PNG image and a JSON
 * description of the sprites. This spritesheet can be added using
 * [method@VectorSpriteSheet.add_page]. Sprites can also be added individually
 * using [method@VectorSpriteSheet.add_sprite].
 *
 * Some map styles rely on application code to provide some or all of their
 * sprites. This is supported using a fallback function, which can be set using
 * [method@VectorSpriteSheet.set_fallback]. This function can generate sprites
 * on demand. For example, it could load a symbolic icon from the [class@Gtk.IconTheme]
 * or render a custom highway shield.
 *
 * ## HiDPI support
 *
 * Map styles should provide a double resolution spritesheet for high DPI
 * displays. That spritesheet can be added as a separate page.
 * The [class@VectorSpriteSheet] will pick the best sprites for the display's
 * scale factor.
 *
 * If a fallback function is set, it receives the requested scale factor
 * as an argument. It should use this to generate the sprite at the correct size.
 * For example, if the scale factor is 2, the image should be twice as large
 * (but the *sprite's* width and height should be the same).
 *
 * ## Thread Safety
 *
 * [class@VectorSpriteSheet] is thread safe.
 *
 * Since: 1.1
 */

/**
 * ShumateVectorSpriteFallbackFunc:
 * @sprite_sheet: the [class@VectorSpriteSheet]
 * @name: the name of the sprite to generate
 * @scale: the scale factor of the sprite
 * @user_data: user data passed to [method@VectorSpriteSheet.set_fallback]
 *
 * A function to generate sprites for a [class@VectorSpriteSheet].
 *
 * See [method@VectorSpriteSheet.set_fallback].
 *
 * Returns: (transfer full) (nullable): a [class@VectorSprite] or %NULL
 *
 * Since: 1.1
 */

#define FALLBACK_QUEUE_CAPACITY 100

struct _ShumateVectorSpriteSheet
{
  GObject parent_instance;

  GRecMutex mutex;

  GHashTable *sprite_arrays;

  ShumateVectorSpriteFallbackFunc *fallback;
  gpointer fallback_user_data;
  GDestroyNotify fallback_destroy;
  GHashTable *fallback_sprites;
  GQueue *fallback_queue;
};

G_DEFINE_FINAL_TYPE (ShumateVectorSpriteSheet, shumate_vector_sprite_sheet, G_TYPE_OBJECT)


/**
 * shumate_vector_sprite_sheet_new:
 *
 * Creates a new, empty [class@VectorSpriteSheet].
 *
 * Returns: (transfer full): a new [class@VectorSpriteSheet]
 *
 * Since: 1.1
 */
ShumateVectorSpriteSheet *
shumate_vector_sprite_sheet_new (void)
{
  return g_object_new (SHUMATE_TYPE_VECTOR_SPRITE_SHEET, NULL);
}

static void
shumate_vector_sprite_sheet_finalize (GObject *object)
{
  ShumateVectorSpriteSheet *self = (ShumateVectorSpriteSheet *)object;

  g_clear_pointer (&self->sprite_arrays, g_hash_table_unref);

  if (self->fallback_destroy != NULL)
    self->fallback_destroy (self->fallback_user_data);
  g_clear_pointer (&self->fallback_sprites, g_hash_table_unref);
  if (self->fallback_queue != NULL)
    g_queue_free_full (self->fallback_queue, g_free);

  g_rec_mutex_clear (&self->mutex);

  G_OBJECT_CLASS (shumate_vector_sprite_sheet_parent_class)->finalize (object);
}

static void
shumate_vector_sprite_sheet_class_init (ShumateVectorSpriteSheetClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = shumate_vector_sprite_sheet_finalize;
}

static void
shumate_vector_sprite_sheet_init (ShumateVectorSpriteSheet *self)
{
  g_rec_mutex_init (&self->mutex);
  self->sprite_arrays = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, (GDestroyNotify)g_ptr_array_unref);
}

/**
 * shumate_vector_sprite_sheet_add_sprite:
 * @self: a [class@VectorSpriteSheet]
 * @name: the name of the sprite
 * @sprite: a [class@VectorSprite]
 *
 * Adds a sprite to the spritesheet.
 *
 * Since: 1.1
 */
void
shumate_vector_sprite_sheet_add_sprite (ShumateVectorSpriteSheet *self,
                                        const char               *name,
                                        ShumateVectorSprite      *sprite)
{
  g_autoptr(GRecMutexLocker) locker = NULL;
  GPtrArray *array = NULL;

  g_return_if_fail (SHUMATE_IS_VECTOR_SPRITE_SHEET (self));
  g_return_if_fail (name != NULL);
  g_return_if_fail (SHUMATE_IS_VECTOR_SPRITE (sprite));

  locker = g_rec_mutex_locker_new (&self->mutex);

  array = g_hash_table_lookup (self->sprite_arrays, name);
  if (array == NULL)
    {
      array = g_ptr_array_new_with_free_func (g_object_unref);
      g_hash_table_insert (self->sprite_arrays, g_strdup (name), array);
    }

  g_ptr_array_add (array, g_object_ref (sprite));
}

/**
 * shumate_vector_sprite_sheet_add_page:
 * @self: a [class@VectorSpriteSheet]
 * @texture: a [class@Gdk.Texture]
 * @json: a JSON string
 * @default_scale: the default scale factor of the page
 * @error: return location for a [struct@GLib.Error], or %NULL
 *
 * Adds a page to the spritesheet.
 *
 * See <https://maplibre.org/maplibre-gl-js-docs/style-spec/sprite/> for
 * details about the spritesheet format. Most stylesheets provide these files
 * along with the main style JSON.
 *
 * Map styles should provide a double resolution spritesheet for high DPI
 * displays. That spritesheet should be added as its own page, with a
 * @default_scale of 2.
 *
 * Returns: %TRUE if the page was added successfully, %FALSE otherwise
 *
 * Since: 1.1
 */
gboolean
shumate_vector_sprite_sheet_add_page (ShumateVectorSpriteSheet *self,
                                      GdkTexture               *texture,
                                      const char               *json,
                                      double                    default_scale,
                                      GError                  **error)
{
  g_return_val_if_fail (SHUMATE_IS_VECTOR_SPRITE_SHEET (self), FALSE);
  g_return_val_if_fail (GDK_IS_TEXTURE (texture), FALSE);
  g_return_val_if_fail (json != NULL, FALSE);

  /* No mutex lock is needed for this function because it only references @self
     via shumate_vector_sprite_sheet_add_sprite(), which has its own lock. */

#ifdef SHUMATE_HAS_VECTOR_RENDERER
  g_autoptr(JsonNode) json_node = NULL;
  JsonObject *sprites;
  JsonObjectIter iter;
  const char *sprite_name;
  JsonNode *sprite_node;

  json_node = json_from_string (json, error);
  if (json_node == NULL)
    return FALSE;

  if (!shumate_vector_json_get_object (json_node, &sprites, error))
    return FALSE;

  json_object_iter_init (&iter, sprites);
  while (json_object_iter_next (&iter, &sprite_name, &sprite_node))
    {
      JsonObject *sprite_object;
      g_autoptr(ShumateVectorSprite) sprite = NULL;
      int x, y, width, height, pixel_ratio;

      if (!shumate_vector_json_get_object (sprite_node, &sprite_object, error))
        return FALSE;

      x = json_object_get_int_member_with_default (sprite_object, "x", 0);
      y = json_object_get_int_member_with_default (sprite_object, "y", 0);
      width = json_object_get_int_member_with_default (sprite_object, "width", 0);
      height = json_object_get_int_member_with_default (sprite_object, "height", 0);
      pixel_ratio = json_object_get_int_member_with_default (sprite_object, "pixelRatio", default_scale);

      if (x < 0 || y < 0 || width <= 0 || height <= 0)
        {
          g_set_error (error,
                       SHUMATE_STYLE_ERROR,
                       SHUMATE_STYLE_ERROR_MALFORMED_STYLE,
                       "Invalid dimensions for sprite '%s'", sprite_name);
          return FALSE;
        }

      sprite = shumate_vector_sprite_new_full (
        GDK_PAINTABLE (texture),
        width / pixel_ratio,
        height / pixel_ratio,
        pixel_ratio,
        &(GdkRectangle){ x, y, width, height }
      );

      shumate_vector_sprite_sheet_add_sprite (self, sprite_name, sprite);
    }

  return TRUE;
#else
  g_set_error (error,
               SHUMATE_STYLE_ERROR,
               SHUMATE_STYLE_ERROR_SUPPORT_OMITTED,
               "Libshumate was compiled without support for vector tiles.");
  return FALSE;
#endif
}

static ShumateVectorSprite *
search_sprites (GPtrArray *sprites,
                double     scale,
                gboolean   higher,
                gboolean   lower)
{
  double best_scale = higher ? G_MAXDOUBLE : G_MINDOUBLE;
  ShumateVectorSprite *best_sprite = NULL;

  for (int i = 0; i < sprites->len; i++)
    {
      ShumateVectorSprite *sprite = g_ptr_array_index (sprites, i);
      double sprite_scale = shumate_vector_sprite_get_scale_factor (sprite);

      if (sprite_scale > scale)
        {
          if (higher && sprite_scale < best_scale)
            {
              best_scale = sprite_scale;
              best_sprite = sprite;
            }
          else
            continue;
        }
      else if (sprite_scale < scale)
        {
          if (lower && sprite_scale > best_scale)
            {
              best_scale = sprite_scale;
              best_sprite = sprite;
            }
          else
            continue;
        }
      else
        return g_object_ref (sprite);
    }

  if (best_sprite != NULL)
    return g_object_ref (best_sprite);
  else
    return NULL;
}


/**
 * shumate_vector_sprite_sheet_get_sprite:
 * @self: a [class@VectorSpriteSheet]
 * @name: an icon name
 * @scale: the scale factor of the icon
 *
 * Gets a sprite from the spritesheet.
 *
 * The returned sprite might not be at the requested scale factor if an exact
 * match is not found.
 *
 * Returns: (transfer full) (nullable): a [class@VectorSprite], or %NULL if the
 * icon does not exist.
 *
 * Since: 1.1
 */
ShumateVectorSprite *
shumate_vector_sprite_sheet_get_sprite (ShumateVectorSpriteSheet *self,
                                        const char               *name,
                                        double                    scale)
{
  g_autoptr(GRecMutexLocker) locker = NULL;
  ShumateVectorSprite *sprite;
  GPtrArray *sprite_array;

  g_return_val_if_fail (SHUMATE_IS_VECTOR_SPRITE_SHEET (self), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  locker = g_rec_mutex_locker_new (&self->mutex);

  sprite_array = g_hash_table_lookup (self->sprite_arrays, name);
  if (sprite_array != NULL)
    {
      /* Search the exact scale, then higher scales, then lower scales */
      if ((sprite = search_sprites (sprite_array, scale, FALSE, FALSE)) != NULL)
        return sprite;
      if ((sprite = search_sprites (sprite_array, scale, TRUE, FALSE)) != NULL)
        return sprite;
      if ((sprite = search_sprites (sprite_array, scale, FALSE, TRUE)) != NULL)
        return sprite;
    }

  if (self->fallback)
    {
      if (g_hash_table_lookup_extended (self->fallback_sprites, name, NULL, (gpointer *)&sprite))
        {
          if (sprite != NULL)
            return g_object_ref (sprite);
          else
            return NULL;
        }

      sprite = self->fallback (self, name, scale, self->fallback_user_data);

      g_hash_table_insert (self->fallback_sprites, g_strdup (name), sprite);
      g_queue_push_tail (self->fallback_queue, g_strdup (name));
      if (g_queue_get_length (self->fallback_queue) > FALLBACK_QUEUE_CAPACITY)
        {
          g_autofree char *old_name = g_queue_pop_head (self->fallback_queue);
          g_hash_table_remove (self->fallback_sprites, old_name);
        }

      if (sprite != NULL)
        return g_object_ref (sprite);
    }

  return NULL;
}


static void
object_unref0 (gpointer object)
{
  if (object != NULL)
    g_object_unref (object);
}


/**
 * shumate_vector_sprite_sheet_set_fallback:
 * @self: a [class@VectorSpriteSheet]
 * @fallback: (nullable): a [callback@ShumateVectorSpriteFallbackFunc] or %NULL
 * @user_data: user data to pass to @fallback
 * @notify: a [callback@GLib.DestroyNotify] for @user_data
 *
 * Sets a fallback function to generate sprites.
 *
 * The fallback function is called when a texture is not found in the sprite
 * sheet. It receives the icon name and scale factor, and should return a
 * [class@VectorSprite], or %NULL if the icon could not be generated.
 * It may be called in a different thread, and it may be called multiple times
 * for the same icon name.
 *
 * If a previous fallback function was set, it will be replaced and any sprites
 * it generated will be cleared.
 *
 * @fallback may be %NULL to clear the fallback function.
 *
 * Since: 1.1
 */
void
shumate_vector_sprite_sheet_set_fallback (ShumateVectorSpriteSheet        *self,
                                          ShumateVectorSpriteFallbackFunc  fallback,
                                          gpointer                         user_data,
                                          GDestroyNotify                   notify)
{
  g_autoptr(GRecMutexLocker) locker = NULL;

  g_return_if_fail (SHUMATE_IS_VECTOR_SPRITE_SHEET (self));
  g_return_if_fail (!(fallback == NULL && user_data != NULL));

  locker = g_rec_mutex_locker_new (&self->mutex);

  /* Clear any previous fallback func */
  if (self->fallback_destroy != NULL)
    self->fallback_destroy (self->fallback_user_data);

  self->fallback = NULL;
  self->fallback_user_data = NULL;
  self->fallback_destroy = NULL;
  g_clear_pointer (&self->fallback_sprites, g_hash_table_unref);
  if (self->fallback_queue != NULL)
    g_queue_free_full (self->fallback_queue, g_free);

  if (fallback != NULL)
    {
      self->fallback = fallback;
      self->fallback_user_data = user_data;
      self->fallback_destroy = notify;

      self->fallback_sprites = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, object_unref0);
      self->fallback_queue = g_queue_new ();
    }
}
