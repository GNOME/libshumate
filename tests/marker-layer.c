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

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);
  gtk_init ();

  g_test_add_func ("/marker-layer/new", test_marker_layer_new);
  g_test_add_func ("/marker-layer/add-marker", test_marker_layer_add_marker);
  g_test_add_func ("/marker-layer/remove-marker", test_marker_layer_remove_marker);
  g_test_add_func ("/marker-layer/remove-all-markers", test_marker_layer_remove_all_markers);

  return g_test_run ();
}
