#undef G_DISABLE_ASSERT

#include <shumate/shumate.h>
#include "shumate/shumate-vector-sprite-sheet.h"

static void
test_vector_sprite_sheet (void)
{
  g_autoptr(GError) error = NULL;
  g_autoptr(ShumateVectorSpriteSheet) sprites = NULL;
  g_autoptr(GdkTexture) texture = NULL;
  g_autoptr(GBytes) json_bytes = NULL;
  g_autoptr(ShumateVectorSprite) sprite = NULL;

  json_bytes = g_resources_lookup_data ("/org/gnome/shumate/Tests/sprites.json", G_RESOURCE_LOOKUP_FLAGS_NONE, &error);
  g_assert_no_error (error);
  texture = gdk_texture_new_from_resource ("/org/gnome/shumate/Tests/sprites.png");

  sprites = shumate_vector_sprite_sheet_new ();

  shumate_vector_sprite_sheet_add_page (sprites, texture, g_bytes_get_data (json_bytes, NULL), 1, &error);
  g_assert_no_error (error);
  g_assert_nonnull (sprites);

  sprite = shumate_vector_sprite_sheet_get_sprite (sprites, "sprite", 1);
  g_assert_true (SHUMATE_IS_VECTOR_SPRITE (sprite));
  g_clear_object (&sprite);

  sprite = shumate_vector_sprite_sheet_get_sprite (sprites, "not-a-sprite", 1);
  g_assert_null (sprite);
  g_clear_object (&sprite);
}

typedef struct {
  char *expected_name;
  gboolean called;
  gboolean return_null;
} FallbackData;

static ShumateVectorSprite *
fallback_func (ShumateVectorSpriteSheet *sprite_sheet,
               const char               *name,
               double                    scale_factor,
               gpointer                  user_data)
{
  FallbackData *data = user_data;
  g_autoptr(GdkPaintable) paintable = NULL;

  g_assert_cmpstr (name, ==, data->expected_name);
  data->called = TRUE;

  /* Test reentrancy */
  g_object_unref (shumate_vector_sprite_sheet_get_sprite (sprite_sheet, "sprite", 1));

  if (data->return_null)
    return NULL;
  else
    {
      paintable = GDK_PAINTABLE (gdk_texture_new_from_resource ("/org/gnome/shumate/Tests/sprites.png"));
      return shumate_vector_sprite_new (paintable);
    }
}


static void
test_vector_sprite_sheet_fallback (void)
{
  g_autoptr(GError) error = NULL;
  g_autoptr(ShumateVectorSpriteSheet) sprites = NULL;
  g_autoptr(GdkTexture) texture = NULL;
  g_autoptr(GBytes) json_bytes = NULL;
  g_autoptr(ShumateVectorSprite) sprite = NULL;
  FallbackData *user_data;

  json_bytes = g_resources_lookup_data ("/org/gnome/shumate/Tests/sprites.json", G_RESOURCE_LOOKUP_FLAGS_NONE, &error);
  g_assert_no_error (error);
  texture = gdk_texture_new_from_resource ("/org/gnome/shumate/Tests/sprites.png");

  sprites = shumate_vector_sprite_sheet_new ();
  shumate_vector_sprite_sheet_add_page (sprites, texture, g_bytes_get_data (json_bytes, NULL), 1, &error);
  g_assert_no_error (error);
  g_assert_nonnull (sprites);

  user_data = g_new0 (FallbackData, 1);
  shumate_vector_sprite_sheet_set_fallback (sprites, fallback_func, user_data, g_free);

  /* the fallback function should not be called for sprites in the sheet */
  sprite = shumate_vector_sprite_sheet_get_sprite (sprites, "sprite", 1);
  g_assert_true (SHUMATE_IS_VECTOR_SPRITE (sprite));
  g_assert_false (user_data->called);
  g_clear_object (&sprite);

  /* the fallback function should provide a sprite */
  user_data->expected_name = "not-a-sprite";
  sprite = shumate_vector_sprite_sheet_get_sprite (sprites, "not-a-sprite", 1);
  g_assert_true (SHUMATE_IS_VECTOR_SPRITE (sprite));
  g_assert_true (user_data->called);

  g_clear_object (&sprite);
  user_data->called = FALSE;

  /* should be cached */
  sprite = shumate_vector_sprite_sheet_get_sprite (sprites, "not-a-sprite", 1);
  g_assert_true (SHUMATE_IS_VECTOR_SPRITE (sprite));
  g_assert_false (user_data->called);
  g_clear_object (&sprite);

  /* test the case when the fallback function returns NULL */
  user_data->expected_name = "not-a-sprite-2";
  user_data->return_null = TRUE;
  sprite = shumate_vector_sprite_sheet_get_sprite (sprites, "not-a-sprite-2", 1);
  g_assert_null (sprite);
  g_assert_true (user_data->called);

  user_data->called = FALSE;

  /* NULL responses should also be cached */
  sprite = shumate_vector_sprite_sheet_get_sprite (sprites, "not-a-sprite-2", 1);
  g_assert_null (sprite);
  g_assert_false (user_data->called);

  /* test cache purging */
  user_data->expected_name = "cached-sprite";
  user_data->return_null = FALSE;
  sprite = shumate_vector_sprite_sheet_get_sprite (sprites, "cached-sprite", 1);
  g_assert_true (SHUMATE_IS_VECTOR_SPRITE (sprite));
  g_clear_object (&sprite);
  for (int i = 0; i < 100; i ++)
    {
      g_autoptr(ShumateVectorSprite) filler_sprite = NULL;
      g_autofree char *name = g_strdup_printf ("filler-sprite-%d", i);
      user_data->expected_name = name;
      filler_sprite = shumate_vector_sprite_sheet_get_sprite (sprites, name, 1);
    }
  user_data->called = FALSE;

  /* should not be cached anymore */
  user_data->expected_name = "cached-sprite";
  sprite = shumate_vector_sprite_sheet_get_sprite (sprites, "cached-sprite", 1);
  g_assert_true (SHUMATE_IS_VECTOR_SPRITE (sprite));
  g_assert_true (user_data->called);
  g_clear_object (&sprite);
}

void
test_scale_factor (void)
{
  g_autoptr(GError) error = NULL;
  g_autoptr(ShumateVectorSpriteSheet) sprites = NULL;
  g_autoptr(GdkTexture) texture = NULL;
  g_autoptr(GBytes) json_bytes = NULL;
  g_autoptr(GdkTexture) texture_2x = NULL;
  g_autoptr(GBytes) json_bytes_2x = NULL;
  ShumateVectorSprite *sprite;
  g_autoptr(ShumateVectorSprite) new_sprite = NULL;

  sprites = shumate_vector_sprite_sheet_new ();

  json_bytes = g_resources_lookup_data ("/org/gnome/shumate/Tests/sprites.json", G_RESOURCE_LOOKUP_FLAGS_NONE, &error);
  g_assert_no_error (error);
  texture = gdk_texture_new_from_resource ("/org/gnome/shumate/Tests/sprites.png");
  shumate_vector_sprite_sheet_add_page (sprites, texture, g_bytes_get_data (json_bytes, NULL), 1, &error);
  g_assert_no_error (error);

  json_bytes_2x = g_resources_lookup_data ("/org/gnome/shumate/Tests/sprites@2x.json", G_RESOURCE_LOOKUP_FLAGS_NONE, &error);
  g_assert_no_error (error);
  texture_2x = gdk_texture_new_from_resource ("/org/gnome/shumate/Tests/sprites@2x.png");
  g_assert_no_error (error);
  shumate_vector_sprite_sheet_add_page (sprites, texture_2x, g_bytes_get_data (json_bytes_2x, NULL), 2, &error);
  g_assert_no_error (error);

  sprite = shumate_vector_sprite_sheet_get_sprite (sprites, "sprite", 1);
  g_assert_true (SHUMATE_IS_VECTOR_SPRITE (sprite));
  g_assert_cmpint (shumate_vector_sprite_get_scale_factor (sprite), ==, 1);
  g_assert_cmpint (shumate_vector_sprite_get_width (sprite), ==, 16);
  g_assert_cmpfloat (gdk_paintable_get_intrinsic_width (shumate_vector_sprite_get_source_paintable (sprite)), ==, 16);
  g_clear_object (&sprite);

  sprite = shumate_vector_sprite_sheet_get_sprite (sprites, "sprite", 2);
  g_assert_true (SHUMATE_IS_VECTOR_SPRITE (sprite));
  g_assert_cmpint (shumate_vector_sprite_get_scale_factor (sprite), ==, 2);
  g_assert_cmpint (shumate_vector_sprite_get_width (sprite), ==, 16);
  g_assert_cmpfloat (gdk_paintable_get_intrinsic_width (shumate_vector_sprite_get_source_paintable (sprite)), ==, 32);
  g_clear_object (&sprite);

  sprite = shumate_vector_sprite_sheet_get_sprite (sprites, "sprite", 3);
  g_assert_cmpint (shumate_vector_sprite_get_scale_factor (sprite), ==, 2);
  g_clear_object (&sprite);

  sprite = shumate_vector_sprite_sheet_get_sprite (sprites, "sprite", 0.5);
  g_assert_cmpint (shumate_vector_sprite_get_scale_factor (sprite), ==, 1);
  g_clear_object (&sprite);
}


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/vector-sprite-sheet/sprites", test_vector_sprite_sheet);
  g_test_add_func ("/vector-sprite-sheet/fallback", test_vector_sprite_sheet_fallback);
  g_test_add_func ("/vector-sprite-sheet/scale-factor", test_scale_factor);

  return g_test_run ();
}
