#undef G_DISABLE_ASSERT

#include <gtk/gtk.h>
#include <shumate/shumate.h>

static void
test_marker_layer_new (void)
{
  ShumateMarkerLayer *marker_layer;
  ShumateViewport *viewport;

  viewport = shumate_viewport_new ();
  marker_layer = shumate_marker_layer_new (viewport);
  g_assert_nonnull (marker_layer);
  g_assert_true (SHUMATE_IS_MARKER_LAYER (marker_layer));

  g_object_unref (viewport);
}

static void
test_marker_layer_add_marker (void)
{
  ShumateMarkerLayer *marker_layer;
  ShumateViewport *viewport;
  ShumateMarker *point;

  viewport = shumate_viewport_new ();
  marker_layer = shumate_marker_layer_new (viewport);

  point = shumate_point_new ();
  g_assert_null (gtk_widget_get_parent (GTK_WIDGET (point)));

  shumate_marker_layer_add_marker (marker_layer, point);
  g_assert_true (gtk_widget_get_parent (GTK_WIDGET (point)) == GTK_WIDGET (marker_layer));

  g_object_unref (viewport);
}

static void
test_marker_layer_remove_marker (void)
{
  ShumateMarkerLayer *marker_layer;
  ShumateViewport *viewport;
  ShumateMarker *point;

  viewport = shumate_viewport_new ();
  marker_layer = shumate_marker_layer_new (viewport);

  point = shumate_point_new ();
  shumate_marker_layer_add_marker (marker_layer, point);
  g_assert_true (gtk_widget_get_parent (GTK_WIDGET (point)) == GTK_WIDGET (marker_layer));

  // Add a ref here so that point isn't completely destroyed before checking
  g_object_ref_sink (point);

  shumate_marker_layer_remove_marker (marker_layer, point);
  g_assert_null (gtk_widget_get_parent (GTK_WIDGET (point)));

  g_object_unref (point);

  g_object_unref (viewport);
}

static void
test_marker_layer_remove_all_markers (void)
{
  ShumateMarkerLayer *marker_layer;
  ShumateViewport *viewport;
  int i;

  viewport = shumate_viewport_new ();
  marker_layer = shumate_marker_layer_new (viewport);

  for (i = 0; i < 100; i++)
    {
      ShumateMarker *point;

      point = shumate_point_new ();
      shumate_marker_layer_add_marker (marker_layer, point);
      g_assert_true (gtk_widget_get_parent (GTK_WIDGET (point)) == GTK_WIDGET (marker_layer));
    }

  shumate_marker_layer_remove_all (marker_layer);
  g_assert_null (gtk_widget_get_first_child (GTK_WIDGET (marker_layer)));

  g_object_unref (viewport);
}

static void
test_marker_layer_selection (void)
{
  g_autoptr(ShumateViewport) viewport = shumate_viewport_new ();
  g_autoptr(ShumateMarkerLayer) layer = shumate_marker_layer_new (viewport);
  ShumateMarker *marker1 = shumate_point_new ();
  ShumateMarker *marker2 = shumate_point_new ();

  g_object_ref_sink (layer);

  shumate_marker_layer_add_marker (layer, marker1);
  shumate_marker_layer_add_marker (layer, marker2);

  g_assert_true (shumate_marker_get_selectable (marker1));

  /* Test that no marker is selected initially */
  g_assert_null (shumate_marker_layer_get_selected (layer));

  /* Default selection mode is NONE, so make sure nothing can be  selected */
  g_assert_cmpint (shumate_marker_layer_get_selection_mode (layer), ==, GTK_SELECTION_NONE);
  g_assert_false (shumate_marker_layer_select_marker (layer, marker1));
  g_assert_null (shumate_marker_layer_get_selected (layer));

  /* Now test selection mode GTK_SELECTION_SINGLE */
  shumate_marker_layer_set_selection_mode (layer, GTK_SELECTION_SINGLE);

  /* Test that selecting a marker works */
  g_assert_true (shumate_marker_layer_select_marker (layer, marker1));
  g_assert_true (shumate_marker_is_selected (marker1));

  /* Test that selecting a marker deselects other markers */
  g_assert_true (shumate_marker_layer_select_marker (layer, marker2));
  g_assert_false (shumate_marker_is_selected (marker1));
  g_assert_true (shumate_marker_is_selected (marker2));

  /* Now test selection mode GTK_SELECTION_MULTIPLE */
  shumate_marker_layer_set_selection_mode (layer, GTK_SELECTION_MULTIPLE);

  /* Test that marker2 is still selected */
  g_assert_false (shumate_marker_is_selected (marker1));
  g_assert_true (shumate_marker_is_selected (marker2));

  /* Test that selecting marker1 doesn't deselect marker2 */
  g_assert_true (shumate_marker_layer_select_marker (layer, marker1));
  g_assert_true (shumate_marker_is_selected (marker1));
  g_assert_true (shumate_marker_is_selected (marker2));

  /* Test that switching back to GTK_SELECTION_NONE deselects everything */
  shumate_marker_layer_set_selection_mode (layer, GTK_SELECTION_NONE);
  g_assert_null (shumate_marker_layer_get_selected (layer));

  /* Test that you can't select anything in GTK_SELECTION_NONE mode */
  g_assert_false (shumate_marker_layer_select_marker (layer, marker1));
  g_assert_false (shumate_marker_is_selected (marker1));

  /* Test select_all and unselect_all */
  shumate_marker_layer_set_selection_mode (layer, GTK_SELECTION_MULTIPLE);

  shumate_marker_layer_select_all_markers (layer);
  g_assert_true (shumate_marker_is_selected (marker1));
  g_assert_true (shumate_marker_is_selected (marker2));

  shumate_marker_layer_unselect_all_markers (layer);
  g_assert_false (shumate_marker_is_selected (marker1));
  g_assert_false (shumate_marker_is_selected (marker2));
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);
  gtk_init ();

  g_test_add_func ("/marker-layer/new", test_marker_layer_new);
  g_test_add_func ("/marker-layer/add-marker", test_marker_layer_add_marker);
  g_test_add_func ("/marker-layer/remove-marker", test_marker_layer_remove_marker);
  g_test_add_func ("/marker-layer/remove-all-markers", test_marker_layer_remove_all_markers);
  g_test_add_func ("/marker-layer/selection", test_marker_layer_selection);

  return g_test_run ();
}
