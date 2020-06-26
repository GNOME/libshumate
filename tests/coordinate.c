#include <gtk/gtk.h>
#include <shumate/shumate.h>

#define ACCEPTABLE_EPSILON 0.0000000000001

static void
test_coordinate_convert (void)
{
  ShumateMapSourceFactory *factory;
  ShumateMapSource *source;
  double latitude = -73.75f;
  double longitude = 45.466f;
  guint zoom_level;

  factory = shumate_map_source_factory_dup_default ();

  g_assert_nonnull (factory);

  source = shumate_map_source_factory_create_cached_source (factory, SHUMATE_MAP_SOURCE_OSM_MAPNIK);

  g_assert_nonnull (source);
  
  for (zoom_level = shumate_map_source_get_min_zoom_level (source);
       zoom_level <= shumate_map_source_get_max_zoom_level (source);
       zoom_level++)
    {
      double x;
      double y;

      x = shumate_map_source_get_x (source, zoom_level, longitude);
      g_assert_cmpfloat_with_epsilon (shumate_map_source_get_longitude (source, zoom_level, x), longitude, ACCEPTABLE_EPSILON);

      y = shumate_map_source_get_y (source, zoom_level, latitude);
      g_assert_cmpfloat_with_epsilon (shumate_map_source_get_latitude (source, zoom_level, y), latitude, ACCEPTABLE_EPSILON);
    }

  g_object_unref (source);
  g_object_unref (factory);
}

int
main (int argc, char *argv[])
{
  gtk_test_init (&argc, &argv);

  g_test_add_func ("/coordinate/convert", test_coordinate_convert);

  return g_test_run ();
}
