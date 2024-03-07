#undef G_DISABLE_ASSERT

#include <gtk/gtk.h>
#include <shumate/shumate.h>

static void
test_viewport_zoom_level_min (void)
{
  g_autoptr(ShumateViewport) viewport = NULL;

  viewport = shumate_viewport_new ();
  g_assert_cmpuint (shumate_viewport_get_min_zoom_level (viewport), ==, 0);

  shumate_viewport_set_max_zoom_level (viewport, 20);
  shumate_viewport_set_zoom_level (viewport, 5);

  shumate_viewport_set_min_zoom_level (viewport, 10);
  g_assert_cmpuint (shumate_viewport_get_min_zoom_level (viewport), ==, 10);

  /* Increasing the minimum zoom level must increase the current zoom level too */
  g_assert_cmpuint (shumate_viewport_get_zoom_level (viewport), ==, 10);

  /* But decreasing it may not */
  shumate_viewport_set_zoom_level (viewport, 10);
  shumate_viewport_set_min_zoom_level (viewport, 0);
  g_assert_cmpuint (shumate_viewport_get_zoom_level (viewport), ==, 10);
}

static void
test_viewport_zoom_level_max (void)
{
  g_autoptr(ShumateViewport) viewport = NULL;

  viewport = shumate_viewport_new ();
  g_assert_cmpuint (shumate_viewport_get_max_zoom_level (viewport), ==, 20);

  shumate_viewport_set_max_zoom_level (viewport, 17);
  g_assert_cmpuint (shumate_viewport_get_max_zoom_level (viewport), ==, 17);

  /* Setting the maximum zoom level must update the current zoom level too */
  shumate_viewport_set_zoom_level (viewport, 15);
  shumate_viewport_set_max_zoom_level (viewport, 10);
  g_assert_cmpuint (shumate_viewport_get_zoom_level (viewport), ==, 10);
}

static void
test_viewport_zoom_level_clamp (void)
{
  g_autoptr(ShumateViewport) viewport = NULL;

  viewport = shumate_viewport_new ();
  g_assert_cmpuint (shumate_viewport_get_min_zoom_level (viewport), ==, 0);
  g_assert_cmpuint (shumate_viewport_get_max_zoom_level (viewport), ==, 20);

  /* Can't set min zoom level > max zoom level */
  g_test_expect_message ("shumate",
                         G_LOG_LEVEL_CRITICAL,
                         "shumate_viewport_set_min_zoom_level: assertion 'min_zoom_level <= self->max_zoom_level' failed");

  shumate_viewport_set_min_zoom_level (viewport, 21);
  g_test_assert_expected_messages ();
  g_assert_cmpuint (shumate_viewport_get_min_zoom_level (viewport), ==, 0);

  shumate_viewport_set_min_zoom_level (viewport, 5);
  shumate_viewport_set_max_zoom_level (viewport, 15);

  /* Equally, can't set max zoom level < min zoom level */
  g_test_expect_message ("shumate",
                         G_LOG_LEVEL_CRITICAL,
                         "shumate_viewport_set_max_zoom_level: assertion 'max_zoom_level >= self->min_zoom_level' failed");

  shumate_viewport_set_max_zoom_level (viewport, 0);
  g_test_assert_expected_messages ();
  g_assert_cmpuint (shumate_viewport_get_max_zoom_level (viewport), ==, 15);

  /* shumate_viewport_set_min_zoom_level() must have updated the current zoom level */
  g_assert_cmpuint (shumate_viewport_get_zoom_level (viewport), ==, 5);

  shumate_viewport_set_zoom_level (viewport, 10);
  g_assert_cmpuint (shumate_viewport_get_zoom_level (viewport), ==, 10);

  /* Zoom level below the minimum must clamp */
  shumate_viewport_set_zoom_level (viewport, 0);
  g_assert_cmpuint (shumate_viewport_get_zoom_level (viewport), ==, 5);

  /* Zoom level above the maximum must clamp too */
  shumate_viewport_set_zoom_level (viewport, 20);
  g_assert_cmpuint (shumate_viewport_get_zoom_level (viewport), ==, 15);
}

static guint max_zoom_level_notify_counter = 0;
static guint min_zoom_level_notify_counter = 0;
static guint zoom_level_notify_counter = 0;

static void
on_max_zoom_level_changed_cb (ShumateViewport *viewport,
                              GParamSpec      *pspec,
                              gpointer         user_data)
{
  max_zoom_level_notify_counter++;
}

static void
on_min_zoom_level_changed_cb (ShumateViewport *viewport,
                              GParamSpec      *pspec,
                              gpointer         user_data)
{
  min_zoom_level_notify_counter++;
}

static void
on_zoom_level_changed_cb (ShumateViewport *viewport,
                          GParamSpec      *pspec,
                          gpointer         user_data)
{
  zoom_level_notify_counter++;
}

static void
test_viewport_zoom_level_notify (void)
{
  g_autoptr(ShumateViewport) viewport = NULL;
  guint i;

  viewport = g_object_connect (shumate_viewport_new (),
                               "signal::notify::max-zoom-level", on_max_zoom_level_changed_cb, NULL,
                               "signal::notify::min-zoom-level", on_min_zoom_level_changed_cb, NULL,
                               "signal::notify::zoom-level", on_zoom_level_changed_cb, NULL,
                               NULL);

  /* Max zoom level */
  for (i = 10; i > 5; i--)
    shumate_viewport_set_max_zoom_level (viewport, i);
  g_assert_cmpuint (max_zoom_level_notify_counter, ==, 5);

  /* Only the first should emit notify::max-zoom-level */
  for (i = 0; i < 5; i++)
    shumate_viewport_set_max_zoom_level (viewport, 15);
  g_assert_cmpuint (max_zoom_level_notify_counter, ==, 6);


  /* Zoom level */
  for (i = 14; i >= 10; i--)
    shumate_viewport_set_zoom_level (viewport, i);
  g_assert_cmpuint (zoom_level_notify_counter, ==, 5);

  for (i = 0; i > 5; i--)
    shumate_viewport_set_zoom_level (viewport, 10);
  g_assert_cmpuint (zoom_level_notify_counter, ==, 5);

  /* Min zoom level */
  for (i = 0; i < 5; i++)
    shumate_viewport_set_min_zoom_level (viewport, i);
  g_assert_cmpuint (min_zoom_level_notify_counter, ==, 4);

  /* Only the first should emit notify::min-zoom-level */
  for (i = 0; i < 5; i++)
    shumate_viewport_set_min_zoom_level (viewport, 5);
  g_assert_cmpuint (min_zoom_level_notify_counter, ==, 5);

  g_assert_cmpuint (max_zoom_level_notify_counter, ==, 6);
  g_assert_cmpuint (zoom_level_notify_counter, ==, 5);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/viewport/zoom-level/min", test_viewport_zoom_level_min);
  g_test_add_func ("/viewport/zoom-level/max", test_viewport_zoom_level_max);
  g_test_add_func ("/viewport/zoom-level/clamp", test_viewport_zoom_level_clamp);
  g_test_add_func ("/viewport/zoom-level/notify", test_viewport_zoom_level_notify);

  return g_test_run ();
}
