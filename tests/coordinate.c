#undef G_DISABLE_ASSERT

#include <gtk/gtk.h>
#include <shumate/shumate.h>

#define ACCEPTABLE_EPSILON 0.0000000000001

static void
test_coordinate_convert (void)
{
  ShumateMapSourceRegistry *registry;
  ShumateMapSource *source;
  double latitude = -73.75f;
  double longitude = 45.466f;
  double zoom_level;

  registry = shumate_map_source_registry_new_with_defaults ();

  g_assert_nonnull (registry);

  source = shumate_map_source_registry_get_by_id (registry, SHUMATE_MAP_SOURCE_OSM_MAPNIK);

  g_assert_nonnull (source);
  
  for (zoom_level = shumate_map_source_get_min_zoom_level (source);
       zoom_level <= shumate_map_source_get_max_zoom_level (source);
       zoom_level += 0.5)
    {
      double x;
      double y;

      x = shumate_map_source_get_x (source, zoom_level, longitude);
      g_assert_cmpfloat_with_epsilon (shumate_map_source_get_longitude (source, zoom_level, x), longitude, ACCEPTABLE_EPSILON);

      y = shumate_map_source_get_y (source, zoom_level, latitude);
      g_assert_cmpfloat_with_epsilon (shumate_map_source_get_latitude (source, zoom_level, y), latitude, ACCEPTABLE_EPSILON);
    }

  g_object_unref (registry);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/coordinate/convert", test_coordinate_convert);

  return g_test_run ();
}
