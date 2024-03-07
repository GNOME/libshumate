#undef G_DISABLE_ASSERT

#include <shumate/shumate.h>

void
test_location_distance (void)
{
  g_autoptr(ShumateLocation) a = SHUMATE_LOCATION (shumate_coordinate_new_full (0.0, 0.0));
  g_autoptr(ShumateLocation) b = SHUMATE_LOCATION (shumate_coordinate_new_full (0.0, 0.0));

  g_assert_cmpfloat (shumate_location_distance (a, b), ==, 0.0);

  shumate_location_set_location (a, 1.0, 1.0);
  g_assert_cmpfloat_with_epsilon (shumate_location_distance (a, b),
                                  157425.537108393531, 0.001);

  shumate_location_set_location (a, 0.0, 180.0);
  g_assert_cmpfloat_with_epsilon (shumate_location_distance (a, b),
                                  20037508.342789243, 0.001);

  shumate_location_set_location (a, 1.0, 2.0);
  shumate_location_set_location (b, 3.0, 4.0);
  g_assert_cmpfloat_with_epsilon (shumate_location_distance (a, b),
                                  314755.1553654014, 0.001);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/location/distance", test_location_distance);

  return g_test_run ();
}

