#undef G_DISABLE_ASSERT

#include <gtk/gtk.h>
#include <shumate/shumate.h>

static void
test_map_add_layers (void)
{
  ShumateMap *map = shumate_map_new ();
  ShumateViewport *viewport = shumate_map_get_viewport (map);
  ShumateLayer *layer1 = SHUMATE_LAYER (shumate_path_layer_new (viewport));
  ShumateLayer *layer2 = SHUMATE_LAYER (shumate_path_layer_new (viewport));
  ShumateLayer *layer3;

  // Add layer1
  shumate_map_add_layer (map, layer1);
  g_assert_true (gtk_widget_get_first_child (GTK_WIDGET (map)) == GTK_WIDGET (layer1));

  // Add layer2, should end up on top
  shumate_map_add_layer (map, layer2);
  g_assert_true (gtk_widget_get_last_child (GTK_WIDGET (map)) == GTK_WIDGET (layer2));

  // Add layer3 above layer2
  layer3 = SHUMATE_LAYER (shumate_path_layer_new (viewport));
  shumate_map_insert_layer_above (map, layer3, layer2);
  g_assert_true (gtk_widget_get_last_child (GTK_WIDGET (map)) == GTK_WIDGET (layer3));

  // Remove layer3
  shumate_map_remove_layer (map, layer3);
  g_assert_true (gtk_widget_get_last_child (GTK_WIDGET (map)) == GTK_WIDGET (layer2));

  // Add layer3 behind layer1
  layer3 = SHUMATE_LAYER (shumate_path_layer_new (viewport));
  shumate_map_insert_layer_behind (map, layer3, layer1);
  g_assert_true (gtk_widget_get_first_child (GTK_WIDGET (map)) == GTK_WIDGET (layer3));

  // Remove layer3
  shumate_map_remove_layer (map, layer3);
  g_assert_true (gtk_widget_get_first_child (GTK_WIDGET (map)) == GTK_WIDGET (layer1));

  // Add layer3 behind NULL
  layer3 = SHUMATE_LAYER (shumate_path_layer_new (viewport));
  shumate_map_insert_layer_behind (map, layer3, NULL);
  g_assert_true (gtk_widget_get_last_child (GTK_WIDGET (map)) == GTK_WIDGET (layer3));
  shumate_map_remove_layer (map, layer3);

  // Add layer3 above NULL
  layer3 = SHUMATE_LAYER (shumate_path_layer_new (viewport));
  shumate_map_insert_layer_above (map, layer3, NULL);
  g_assert_true (gtk_widget_get_first_child (GTK_WIDGET (map)) == GTK_WIDGET (layer3));
  shumate_map_remove_layer (map, layer3);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);
  gtk_init ();

  g_test_add_func ("/map/add-layers", test_map_add_layers);

  return g_test_run ();
}
