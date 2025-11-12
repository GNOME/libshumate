#undef G_DISABLE_ASSERT

#include <gtk/gtk.h>
#include <shumate/shumate.h>

static void
emit_double_click (ShumateMap *map)
{
  g_autoptr(GListModel) controllers = gtk_widget_observe_controllers (GTK_WIDGET (map));
  for (guint i = 0; i < g_list_model_get_n_items (controllers); i++) {
      g_autoptr(GtkEventController) controller = g_list_model_get_item (controllers, i);

      if (GTK_IS_GESTURE_CLICK (controller)) {
          // emit a double click
          g_signal_emit_by_name (controller, "pressed", 2, 10.0, 20.0);
      }
  }
}

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

static void
test_map_zoom_on_double_click_switch (void)
{
  ShumateMap *map = shumate_map_new ();
  ShumateViewport *viewport = shumate_map_get_viewport (map);
  // initial zoom is 10
  double initial = 10;

  // test: zoom on double click should NOT work
  shumate_viewport_set_zoom_level (viewport, initial);
  shumate_map_set_zoom_on_double_click (map, FALSE);
  emit_double_click (map);
  double actual = shumate_viewport_get_zoom_level (viewport);
  g_assert_cmpfloat_with_epsilon (actual, initial, 0.0001);

  // test: zoom on double click should work
  shumate_viewport_set_zoom_level (viewport, initial);
  shumate_map_set_zoom_on_double_click (map, TRUE);
  emit_double_click (map);
  actual = shumate_viewport_get_zoom_level (viewport);
  g_assert_true (actual > initial);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);
  gtk_init ();

  g_test_add_func ("/map/add-layers", test_map_add_layers);
  g_test_add_func ("/map/zoom_on_double_click_switch", test_map_zoom_on_double_click_switch);

  return g_test_run ();
}
