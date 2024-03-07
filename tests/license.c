#undef G_DISABLE_ASSERT

#include <gtk/gtk.h>
#include <shumate/shumate.h>

static void
test_license_map ()
{
  g_autoptr(ShumateLicense) license = shumate_license_new ();
  g_autoptr(ShumateMap) map1 = shumate_map_new ();
  ShumateViewport *viewport1 = shumate_map_get_viewport (map1);
  g_autoptr(ShumateRasterRenderer) source1 = NULL;
  ShumateLayer *layer1 = NULL;
  g_autoptr(ShumateMap) map2 = shumate_map_new ();
  ShumateViewport *viewport2 = shumate_map_get_viewport (map2);
  g_autoptr(ShumateRasterRenderer) source2 = NULL;
  ShumateLayer *layer2 = NULL;
  g_autoptr(ShumateRasterRenderer) source2b = NULL;
  ShumateLayer *layer2b = NULL;
  GtkWidget *label;

  g_object_ref_sink (license);
  g_object_ref_sink (map1);
  g_object_ref_sink (map2);

  label = gtk_widget_get_first_child (GTK_WIDGET (license));
  g_assert_true (GTK_IS_LABEL (label));

  g_assert_cmpstr (gtk_label_get_text (GTK_LABEL (label)), ==, "");

  shumate_license_set_map (license, map1);
  g_assert_cmpstr (gtk_label_get_text (GTK_LABEL (label)), ==, "");

  source1 = shumate_raster_renderer_new_full_from_url ("test",
                                                       "test",
                                                       "Hello, world!",
                                                       NULL, 0, 0, 256,
                                                       SHUMATE_MAP_PROJECTION_MERCATOR,
                                                       "https://localhost");
  layer1 = SHUMATE_LAYER (shumate_map_layer_new (SHUMATE_MAP_SOURCE (source1), viewport1));
  shumate_map_add_layer (map1, layer1);
  g_assert_cmpstr (gtk_label_get_text (GTK_LABEL (label)), ==, "Hello, world!");

  shumate_map_add_layer (map1, SHUMATE_LAYER (shumate_path_layer_new (viewport1)));
  g_assert_cmpstr (gtk_label_get_text (GTK_LABEL (label)), ==, "Hello, world!");

  shumate_map_remove_layer (map1, layer1);
  g_assert_cmpstr (gtk_label_get_text (GTK_LABEL (label)), ==, "");

  source2 = shumate_raster_renderer_new_full_from_url ("test",
                                                       "test",
                                                       "Goodbye, world!",
                                                       NULL, 0, 0, 256,
                                                       SHUMATE_MAP_PROJECTION_MERCATOR,
                                                       "https://localhost");
  layer2 = SHUMATE_LAYER (shumate_map_layer_new (SHUMATE_MAP_SOURCE (source2), viewport2));
  shumate_map_add_layer (map2, layer2);

  shumate_license_set_map (license, map2);
  g_assert_cmpstr (gtk_label_get_text (GTK_LABEL (label)), ==, "Goodbye, world!");

  source2b = shumate_raster_renderer_new_full_from_url ("test",
                                                        "test",
                                                        "source2b || !source2b",
                                                        NULL, 0, 0, 256,
                                                        SHUMATE_MAP_PROJECTION_MERCATOR,
                                                        "https://localhost");
  layer2b = SHUMATE_LAYER (shumate_map_layer_new (SHUMATE_MAP_SOURCE (source2b), viewport2));
  shumate_map_insert_layer_above (map2, layer2b, NULL);

  shumate_license_set_map (license, map2);
  g_assert_cmpstr (gtk_label_get_text (GTK_LABEL (label)), ==, "source2b || !source2b\nGoodbye, world!");

  shumate_license_set_map (license, NULL);
  g_assert_cmpstr (gtk_label_get_text (GTK_LABEL (label)), ==, "");
}

static void
test_license_extra_text ()
{
  g_autoptr(ShumateLicense) license = shumate_license_new ();
  GtkWidget *label;

  g_object_ref_sink (license);

  shumate_license_set_extra_text (license, "Hello, world!");
  g_assert_cmpstr (shumate_license_get_extra_text (license), ==, "Hello, world!");

  label = gtk_widget_get_last_child (GTK_WIDGET (license));
  g_assert_true (GTK_IS_LABEL (label));

  g_assert_cmpstr (gtk_label_get_text (GTK_LABEL (label)), ==, "Hello, world!");
}


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);
  gtk_init ();

  g_test_add_func ("/license/map", test_license_map);
  g_test_add_func ("/license/extra-text", test_license_extra_text);

  return g_test_run ();
}
