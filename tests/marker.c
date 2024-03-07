#undef G_DISABLE_ASSERT

#include <gtk/gtk.h>
#include <shumate/shumate.h>

static void
test_marker_child (void)
{
  ShumateMarker *marker;
  GtkWidget *button;

  marker = shumate_marker_new ();
  g_assert_null (gtk_widget_get_first_child (GTK_WIDGET (marker)));

  button = gtk_button_new ();
  shumate_marker_set_child (marker, button);
  g_assert_true (gtk_widget_get_first_child (GTK_WIDGET (marker)) == button);

  shumate_marker_set_child (marker, NULL);
  g_assert_null (gtk_widget_get_first_child (GTK_WIDGET (marker)));
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);
  gtk_init ();

  g_test_add_func ("/marker/child", test_marker_child);

  return g_test_run ();
}
