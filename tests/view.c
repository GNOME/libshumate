#include <gtk/gtk.h>
#include <shumate/shumate.h>

static void
test_view_add_layers (void)
{
  ShumateView *view = shumate_view_new ();
  ShumateViewport *viewport = shumate_view_get_viewport (view);
  ShumateLayer *layer1 = SHUMATE_LAYER (shumate_path_layer_new (viewport));
  ShumateLayer *layer2 = SHUMATE_LAYER (shumate_path_layer_new (viewport));
  ShumateLayer *layer3;

  // Add layer1
  shumate_view_add_layer (view, layer1);
  g_assert_true (gtk_widget_get_first_child (GTK_WIDGET (view)) == GTK_WIDGET (layer1));

  // Add layer2, should end up on top
  shumate_view_add_layer (view, layer2);
  g_assert_true (gtk_widget_get_last_child (GTK_WIDGET (view)) == GTK_WIDGET (layer2));

  // Add layer3 above layer2
  layer3 = SHUMATE_LAYER (shumate_path_layer_new (viewport));
  shumate_view_insert_layer_above (view, layer3, layer2);
  g_assert_true (gtk_widget_get_last_child (GTK_WIDGET (view)) == GTK_WIDGET (layer3));

  // Remove layer3
  shumate_view_remove_layer (view, layer3);
  g_assert_true (gtk_widget_get_last_child (GTK_WIDGET (view)) == GTK_WIDGET (layer2));

  // Add layer3 behind layer1
  layer3 = SHUMATE_LAYER (shumate_path_layer_new (viewport));
  shumate_view_insert_layer_behind (view, layer3, layer1);
  g_assert_true (gtk_widget_get_first_child (GTK_WIDGET (view)) == GTK_WIDGET (layer3));

  // Remove layer3
  shumate_view_remove_layer (view, layer3);
  g_assert_true (gtk_widget_get_first_child (GTK_WIDGET (view)) == GTK_WIDGET (layer1));

  // Add layer3 behind NULL
  layer3 = SHUMATE_LAYER (shumate_path_layer_new (viewport));
  shumate_view_insert_layer_behind (view, layer3, NULL);
  g_assert_true (gtk_widget_get_last_child (GTK_WIDGET (view)) == GTK_WIDGET (layer3));
  shumate_view_remove_layer (view, layer3);

  // Add layer3 above NULL
  layer3 = SHUMATE_LAYER (shumate_path_layer_new (viewport));
  shumate_view_insert_layer_above (view, layer3, NULL);
  g_assert_true (gtk_widget_get_first_child (GTK_WIDGET (view)) == GTK_WIDGET (layer3));
  shumate_view_remove_layer (view, layer3);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);
  gtk_init ();

  g_test_add_func ("/view/add-layers", test_view_add_layers);

  return g_test_run ();
}
