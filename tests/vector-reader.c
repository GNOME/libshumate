#undef G_DISABLE_ASSERT

#include <shumate/shumate.h>
#include "shumate/vector/vector_tile.pb-c.h"

#define MOVE_TO 1
#define LINE_TO 2
#define CLOSE_PATH 7
#define CMD(op, rep) ((op & 7) | ((rep) << 3))
#define POINT(x, y) unzigzag (x), unzigzag (y)

static uint32_t
unzigzag (int value)
{
  return (value << 1) ^ (value >> 31);
}

static VectorTile__Tile__Value *
new_string_value (const char *value)
{
  VectorTile__Tile__Value *v = g_new0 (VectorTile__Tile__Value, 1);
  vector_tile__tile__value__init (v);
  v->string_value = g_strdup (value);
  return v;
}

static void
set_feature_geometry (VectorTile__Tile__Feature *feature, VectorTile__Tile__GeomType geom_type, const uint32_t *geometry)
{
  feature->type = geom_type;
  feature->has_type = TRUE;
  for (int i = 0; geometry[i] != G_MAXUINT32; i++)
    feature->n_geometry++;
  feature->geometry = g_new0 (uint32_t, feature->n_geometry);
  for (int i = 0; i < feature->n_geometry; i++)
    feature->geometry[i] = geometry[i];
}

static VectorTile__Tile__Feature *
add_feature (VectorTile__Tile__Layer *layer, uint32_t id, uint32_t *tags, uint32_t n_tags)
{
  VectorTile__Tile__Feature *feature = g_new0 (VectorTile__Tile__Feature, 1);
  vector_tile__tile__feature__init (feature);
  feature->id = id;
  feature->has_id = TRUE;
  feature->n_tags = n_tags;
  feature->tags = g_new0 (uint32_t, n_tags);
  for (int i = 0; i < n_tags; i++)
    feature->tags[i] = tags[i];
  layer->n_features++;
  layer->features = g_renew (VectorTile__Tile__Feature *, layer->features, layer->n_features);
  layer->features[layer->n_features - 1] = feature;
  return feature;
}

static VectorTile__Tile__Layer *
add_layer (VectorTile__Tile *tile, char *name, int extent, char **keys, VectorTile__Tile__Value **values)
{
  VectorTile__Tile__Layer *layer = g_new0 (VectorTile__Tile__Layer, 1);
  vector_tile__tile__layer__init (layer);
  layer->name = g_strdup (name);
  layer->extent = extent;
  layer->has_extent = TRUE;
  layer->version = 2;
  layer->n_keys = g_strv_length (keys);
  layer->keys = g_new0 (char *, layer->n_keys);
  for (int i = 0; i < layer->n_keys; i++)
    layer->keys[i] = g_strdup (keys[i]);
  layer->n_values = g_strv_length ((char **)values);
  layer->values = g_new0 (VectorTile__Tile__Value *, layer->n_values);
  for (int i = 0; i < layer->n_values; i++)
    layer->values[i] = values[i];
  tile->n_layers++;
  tile->layers = g_renew (VectorTile__Tile__Layer *, tile->layers, tile->n_layers);
  tile->layers[tile->n_layers - 1] = layer;
  return layer;
}

static GBytes *
create_test_tile (void)
{
  VectorTile__Tile *tile;
  VectorTile__Tile__Layer *layer;
  VectorTile__Tile__Feature *feature;
  uint8_t *out;
  size_t out_len;

  tile = g_new0 (VectorTile__Tile, 1);
  vector_tile__tile__init (tile);

  layer = add_layer (tile, "helloworld", 4096,
    (char*[]){"hello", NULL},
    (VectorTile__Tile__Value*[]){new_string_value ("world"), NULL}
  );

  /* Point feature*/
  feature = add_feature (layer, 1, (uint32_t[]){0, 0}, 2);
  set_feature_geometry (feature, VECTOR_TILE__TILE__GEOM_TYPE__POINT, (uint32_t[]){CMD (MOVE_TO, 1), POINT (1, 2), G_MAXUINT32});

  /* MultiPoint feature */
  feature = add_feature (layer, 2, (uint32_t[]){}, 0);
  set_feature_geometry (feature, VECTOR_TILE__TILE__GEOM_TYPE__POINT, (uint32_t[]){
      CMD (MOVE_TO, 2),
      POINT (100, 200),
      POINT (300, 400),
      G_MAXUINT32,
  });

  /* LineString feature */
  feature = add_feature (layer, 3, (uint32_t[]){}, 0);
  set_feature_geometry (feature, VECTOR_TILE__TILE__GEOM_TYPE__LINESTRING, (uint32_t[]){
      CMD (MOVE_TO, 1),
      POINT (100, 200),
      CMD (LINE_TO, 2),
      POINT (300, 400),
      POINT (500, 600),
      G_MAXUINT32,
  });

  /* MultiLineString feature */
  feature = add_feature (layer, 4, (uint32_t[]){}, 0);
  set_feature_geometry (feature, VECTOR_TILE__TILE__GEOM_TYPE__LINESTRING, (uint32_t[]){
      CMD (MOVE_TO, 1),
      POINT (100, 200),
      CMD (LINE_TO, 2),
      POINT (300, 400),
      POINT (500, 600),
      CMD (MOVE_TO, 1),
      POINT (100, 200),
      CMD (LINE_TO, 2),
      POINT (300, 400),
      POINT (500, 600),
      G_MAXUINT32,
  });

  /* Polygon feature */
  feature = add_feature (layer, 5, (uint32_t[]){}, 0);
  set_feature_geometry (feature, VECTOR_TILE__TILE__GEOM_TYPE__POLYGON, (uint32_t[]){
      /* Exterior ring */
      CMD (MOVE_TO, 1),
      POINT (100, 200),
      CMD (LINE_TO, 3),
      POINT (200, 0),
      POINT (0, 200),
      POINT (-200, 0),
      CMD (CLOSE_PATH, 1),
      /* Interior ring */
      CMD (MOVE_TO, 1),
      POINT (50, -50),
      CMD (LINE_TO, 3),
      POINT (100, 0),
      POINT (0, -100),
      POINT (-100, 0),
      CMD (CLOSE_PATH, 1),
      G_MAXUINT32,
  });

  /* MultiPolygon feature */
  feature = add_feature (layer, 6, (uint32_t[]){}, 0);
  set_feature_geometry (feature, VECTOR_TILE__TILE__GEOM_TYPE__POLYGON, (uint32_t[]){
      /* Exterior ring 1 (a square) */
      CMD (MOVE_TO, 1),
      POINT (100, 200),
      CMD (LINE_TO, 3),
      POINT (200, 0),
      POINT (0, 200),
      POINT (-200, 0),
      CMD (CLOSE_PATH, 1),
      /* Interior ring (another square)*/
      CMD (MOVE_TO, 1),
      POINT (50, -50),
      CMD (LINE_TO, 3),
      POINT (100, 0),
      POINT (0, -100),
      POINT (-100, 0),
      CMD (CLOSE_PATH, 1),
      /* Exterior ring 2 (square rotated 45 degrees, overlapping ring 1) */
      CMD (MOVE_TO, 1),
      POINT (51, 51),
      CMD (LINE_TO, 3),
      POINT (49, -51),
      POINT (100, 50),
      POINT (-100, 50),
      CMD (CLOSE_PATH, 1),
      G_MAXUINT32,
  });


  layer = add_layer (tile, "helloworld2", 100,
    (char*[]){NULL},
    (VectorTile__Tile__Value*[]){NULL}
  );

  out_len = vector_tile__tile__get_packed_size (tile);
  out = g_new0 (uint8_t, out_len);
  vector_tile__tile__pack (tile, out);

  vector_tile__tile__free_unpacked (tile, NULL);

  return g_bytes_new_take (out, out_len);
}

static void
test_vector_reader_layers (void)
{
  g_autoptr(ShumateVectorReader) reader = NULL;
  g_autoptr(ShumateVectorReaderIter) iter = NULL;
  g_autoptr(GBytes) tile_data = create_test_tile ();
  g_auto(GValue) value = G_VALUE_INIT;

  g_assert_nonnull (tile_data);

  reader = shumate_vector_reader_new (tile_data);
  g_assert_nonnull (reader);

  iter = shumate_vector_reader_iterate (reader);
  g_assert_nonnull (iter);

  g_assert_cmpint (shumate_vector_reader_iter_get_layer_count (iter), ==, 2);

  shumate_vector_reader_iter_read_layer (iter, 0);
  g_assert_cmpstr (shumate_vector_reader_iter_get_layer_name (iter), ==, "helloworld");
  g_assert_cmpint (shumate_vector_reader_iter_get_layer_extent (iter), ==, 4096);
  g_assert_cmpint (shumate_vector_reader_iter_get_layer_feature_count (iter), ==, 6);

  shumate_vector_reader_iter_read_layer (iter, 1);
  g_assert_cmpstr (shumate_vector_reader_iter_get_layer_name (iter), ==, "helloworld2");
  g_assert_cmpint (shumate_vector_reader_iter_get_layer_extent (iter), ==, 100);
  g_assert_cmpint (shumate_vector_reader_iter_get_layer_feature_count (iter), ==, 0);
}

static void
test_vector_reader_tags (void)
{
  g_autoptr(ShumateVectorReader) reader = NULL;
  g_autoptr(ShumateVectorReaderIter) iter = NULL;
  g_autoptr(GBytes) tile_data = create_test_tile ();
  g_auto(GValue) value = G_VALUE_INIT;
  g_autofree const char **keys;

  g_assert_nonnull (tile_data);

  reader = shumate_vector_reader_new (tile_data);
  g_assert_nonnull (reader);

  iter = shumate_vector_reader_iterate (reader);
  g_assert_nonnull (iter);

  g_assert_true (shumate_vector_reader_iter_read_layer_by_name (iter, "helloworld"));
  g_assert_true (shumate_vector_reader_iter_next_feature (iter));
  g_assert_cmpint (shumate_vector_reader_iter_get_feature_id (iter), ==, 1);
  g_assert_true (shumate_vector_reader_iter_get_feature_tag (iter, "hello", &value));

  keys = shumate_vector_reader_iter_get_feature_keys (iter);
  g_assert_cmpint (g_strv_length ((char **)keys), ==, 1);
  g_assert_cmpstr (keys[0], ==, "hello");
  g_assert_null (keys[1]);

  g_assert_cmpstr (g_value_get_string (&value), ==, "world");
}

static void
test_vector_reader_geometry (void)
{
  g_autoptr(ShumateVectorReader) reader = NULL;
  g_autoptr(ShumateVectorReaderIter) iter = NULL;
  g_autoptr(GBytes) tile_data = create_test_tile ();
  g_auto(GValue) value = G_VALUE_INIT;
  double x, y;

  g_assert_nonnull (tile_data);

  reader = shumate_vector_reader_new (tile_data);
  g_assert_nonnull (reader);

  iter = shumate_vector_reader_iterate (reader);
  g_assert_nonnull (iter);

  g_assert_true (shumate_vector_reader_iter_read_layer_by_name (iter, "helloworld"));

  /* Point */
  g_assert_true (shumate_vector_reader_iter_next_feature (iter));
  g_assert_cmpint (shumate_vector_reader_iter_get_feature_geometry_type (iter), ==, SHUMATE_GEOMETRY_TYPE_POINT);
  g_assert_true (shumate_vector_reader_iter_get_feature_point (iter, &x, &y));
  g_assert_cmpfloat (x, ==, 1.0);
  g_assert_cmpfloat (y, ==, 2.0);

  /* MultiPoint */
  g_assert_true (shumate_vector_reader_iter_next_feature (iter));
  g_assert_cmpint (shumate_vector_reader_iter_get_feature_geometry_type (iter), ==, SHUMATE_GEOMETRY_TYPE_MULTIPOINT);

  /* LineString */
  g_assert_true (shumate_vector_reader_iter_next_feature (iter));
  g_assert_cmpint (shumate_vector_reader_iter_get_feature_geometry_type (iter), ==, SHUMATE_GEOMETRY_TYPE_LINESTRING);

  /* MultiLineString */
  g_assert_true (shumate_vector_reader_iter_next_feature (iter));
  g_assert_cmpint (shumate_vector_reader_iter_get_feature_geometry_type (iter), ==, SHUMATE_GEOMETRY_TYPE_MULTILINESTRING);

  /* Polygon */
  g_assert_true (shumate_vector_reader_iter_next_feature (iter));
  g_assert_cmpint (shumate_vector_reader_iter_get_feature_geometry_type (iter), ==, SHUMATE_GEOMETRY_TYPE_POLYGON);
  g_assert_true (shumate_vector_reader_iter_feature_contains_point (iter, 105, 205));
  g_assert_false (shumate_vector_reader_iter_feature_contains_point (iter, 175, 300));
  g_assert_false (shumate_vector_reader_iter_feature_contains_point (iter, 0, 0));

  /* Multipolygon */
  g_assert_true (shumate_vector_reader_iter_next_feature (iter));
  g_assert_cmpint (shumate_vector_reader_iter_get_feature_geometry_type (iter), ==, SHUMATE_GEOMETRY_TYPE_MULTIPOLYGON);
  /* Simple case */
  g_assert_true (shumate_vector_reader_iter_feature_contains_point (iter, 105, 205));
  /* Overlap */
  g_assert_true (shumate_vector_reader_iter_feature_contains_point (iter, 275, 300));
  /* Interior ring + overlap*/
  g_assert_true (shumate_vector_reader_iter_feature_contains_point (iter, 225, 300));
  /* Interior ring */
  g_assert_false (shumate_vector_reader_iter_feature_contains_point (iter, 175, 300));
  /* Outside */
  g_assert_false (shumate_vector_reader_iter_feature_contains_point (iter, 0, 0));
  /* Ray might pass through a corner */
  g_assert_false (shumate_vector_reader_iter_feature_contains_point (iter, 350, 299));
  g_assert_false (shumate_vector_reader_iter_feature_contains_point (iter, 350, 301));
  g_assert_false (shumate_vector_reader_iter_feature_contains_point (iter, 351, 300));
  g_assert_true (shumate_vector_reader_iter_feature_contains_point (iter, 349, 300));
  /* Horizontal/vertical edges */
  g_assert_false (shumate_vector_reader_iter_feature_contains_point (iter, 99, 200));
  g_assert_false (shumate_vector_reader_iter_feature_contains_point (iter, 301, 200));
  g_assert_false (shumate_vector_reader_iter_feature_contains_point (iter, 100, 199));
  g_assert_false (shumate_vector_reader_iter_feature_contains_point (iter, 100, 401));
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/vector-reader/layers", test_vector_reader_layers);
  g_test_add_func ("/vector-reader/tags", test_vector_reader_tags);
  g_test_add_func ("/vector-reader/geometry", test_vector_reader_geometry);

  return g_test_run ();
}
