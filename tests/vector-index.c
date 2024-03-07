#undef G_DISABLE_ASSERT

#include <shumate/shumate.h>
#include "shumate/vector/shumate-vector-index-private.h"
#include "shumate/vector/shumate-vector-render-scope-private.h"
#include "shumate/vector/shumate-vector-expression-private.h"
#include "shumate/shumate-vector-reader-iter-private.h"

static void
test_vector_index_bitset (void)
{
  g_autoptr(ShumateVectorIndexBitset) bitset = shumate_vector_index_bitset_new (100);
  g_autoptr(ShumateVectorIndexBitset) bitset2 = NULL;
  g_assert_nonnull (bitset);
  g_assert_cmpint (bitset->len, ==, 100);
  g_assert_nonnull (bitset->bits);

  g_assert_cmpint (shumate_vector_index_bitset_next (bitset, -1), ==, -1);

  shumate_vector_index_bitset_set (bitset, 0);
  g_assert_cmpint (shumate_vector_index_bitset_next (bitset, -1), ==, 0);

  shumate_vector_index_bitset_set (bitset, 32);
  g_assert_cmpint (shumate_vector_index_bitset_next (bitset, 0), ==, 32);

  shumate_vector_index_bitset_clear (bitset, 32);
  shumate_vector_index_bitset_set (bitset, 50);
  g_assert_cmpint (shumate_vector_index_bitset_next (bitset, 0), ==, 50);

  shumate_vector_index_bitset_set (bitset, 99);
  g_assert_cmpint (shumate_vector_index_bitset_next (bitset, 98), ==, 99);
  g_assert_cmpint (shumate_vector_index_bitset_next (bitset, 99), ==, -1);

  bitset2 = shumate_vector_index_bitset_copy (bitset);
  g_assert_nonnull (bitset2);
  g_assert_cmpint (bitset2->len, ==, 100);
  g_assert_nonnull (bitset2->bits);
  g_assert_cmpint (shumate_vector_index_bitset_next (bitset2, 0), ==, 50);

  shumate_vector_index_bitset_set (bitset2, 49);
  shumate_vector_index_bitset_or (bitset, bitset2);
  g_assert_cmpint (shumate_vector_index_bitset_next (bitset, 0), ==, 49);

  shumate_vector_index_bitset_clear (bitset2, 49);
  shumate_vector_index_bitset_and (bitset, bitset2);
  g_assert_cmpint (shumate_vector_index_bitset_next (bitset, 0), ==, 50);
}

static void
test_vector_index_description (void)
{
  ShumateVectorIndexDescription *desc = shumate_vector_index_description_new ();
  g_auto(ShumateVectorValue) value = SHUMATE_VECTOR_VALUE_INIT;

  shumate_vector_value_set_string (&value, "Hello, world!");

  g_assert_nonnull (desc);

  g_assert_false (shumate_vector_index_description_has_layer (desc, "foo"));

  shumate_vector_index_description_add_broad_geometry_type (desc, "foo");
  g_assert_true (shumate_vector_index_description_has_layer (desc, "foo"));
  g_assert_true (shumate_vector_index_description_has_broad_geometry_type (desc, "foo"));
  g_assert_false (shumate_vector_index_description_has_geometry_type (desc, "foo"));

  shumate_vector_index_description_add_geometry_type (desc, "bar");
  g_assert_true (shumate_vector_index_description_has_layer (desc, "bar"));
  g_assert_false (shumate_vector_index_description_has_broad_geometry_type (desc, "bar"));
  g_assert_true (shumate_vector_index_description_has_geometry_type (desc, "bar"));

  shumate_vector_index_description_add_has_index (desc, "baz", "qux");
  g_assert_true (shumate_vector_index_description_has_layer (desc, "baz"));
  g_assert_true (shumate_vector_index_description_has_field (desc, "baz", "qux"));
  g_assert_true (shumate_vector_index_description_has_field_has_index (desc, "baz", "qux"));
  g_assert_false (shumate_vector_index_description_has_value (desc, "baz", "qux", &value));

  shumate_vector_index_description_add (desc, "baz", "aaa", &value);
  g_assert_true (shumate_vector_index_description_has_field (desc, "baz", "aaa"));
  g_assert_false (shumate_vector_index_description_has_field_has_index (desc, "baz", "aaa"));
  g_assert_true (shumate_vector_index_description_has_value (desc, "baz", "aaa", &value));

  shumate_vector_index_description_free (desc);
}

static ShumateVectorIndexBitset *
compute_bitset (ShumateVectorReader *reader, char *json, const char *layer, ShumateVectorIndexDescription **index_description_out)
{
  GError *error = NULL;
  ShumateVectorRenderScope scope;
  g_autoptr(JsonNode) node1 = NULL;
  g_autoptr(ShumateVectorExpression) expr1 = NULL;
  g_autoptr(ShumateVectorIndexBitset) bitset1 = NULL;

  scope.reader = shumate_vector_reader_iterate (reader);
  g_assert_nonnull (scope.reader);

  scope.index = NULL;
  scope.index_description = shumate_vector_index_description_new ();

  scope.zoom_level = 12;

  node1 = json_from_string (json, &error);
  g_assert_no_error (error);
  expr1 = shumate_vector_expression_from_json (node1, &error);
  g_assert_no_error (error);

  shumate_vector_expression_collect_indexes (expr1, layer, scope.index_description);

  shumate_vector_reader_iter_read_layer_by_name (scope.reader, layer);
  scope.source_layer_idx = shumate_vector_reader_iter_get_layer_index (scope.reader);
  shumate_vector_render_scope_index_layer (&scope);
  bitset1 = shumate_vector_expression_eval_bitset (expr1, &scope, NULL);
  g_assert_nonnull (bitset1);

  if (index_description_out)
    *index_description_out = g_steal_pointer (&scope.index_description);
  else
    shumate_vector_index_description_free (scope.index_description);

  g_clear_pointer (&scope.index, shumate_vector_index_free);
  g_clear_object (&scope.reader);

  return g_steal_pointer (&bitset1);
}

static void
test_vector_index_eval (void)
{
  GError *error = NULL;
  g_autoptr(GBytes) vector_data = NULL;
  g_autoptr(ShumateVectorReader) reader = NULL;
  ShumateVectorIndexBitset *bitset = NULL;
  ShumateVectorIndexDescription *index_description = NULL;

  vector_data = g_resources_lookup_data ("/org/gnome/shumate/Tests/0.pbf", G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
  g_assert_no_error (error);

  reader = shumate_vector_reader_new (vector_data);

  /* Test literals in eval_bitset() */
  bitset = compute_bitset (reader, "[\"all\", true, false]", "lines", NULL);
  g_assert_false (shumate_vector_index_bitset_get (bitset, 0));
  g_assert_false (shumate_vector_index_bitset_get (bitset, 1));
  shumate_vector_index_bitset_free (bitset);

  bitset = compute_bitset (reader, "[\"any\", true, false]", "lines", NULL);
  g_assert_true (shumate_vector_index_bitset_get (bitset, 0));
  g_assert_true (shumate_vector_index_bitset_get (bitset, 1));
  shumate_vector_index_bitset_free (bitset);

  /* Test geometry type indexes, both broad and not*/
  bitset = compute_bitset (reader, "[\"in\", [\"geometry-type\"], [\"literal\", [\"LineString\", \"MultiLineString\"]]]", "lines", &index_description);
  g_assert_true (shumate_vector_index_bitset_get (bitset, 0));
  g_assert_true (shumate_vector_index_bitset_get (bitset, 1));
  g_assert_false (shumate_vector_index_description_has_geometry_type (index_description, "lines"));
  g_assert_true (shumate_vector_index_description_has_broad_geometry_type (index_description, "lines"));
  shumate_vector_index_bitset_free (bitset);
  shumate_vector_index_description_free (index_description);

  bitset = compute_bitset (reader, "[\"==\", [\"geometry-type\"], \"LineString\"]", "lines", &index_description);
  g_assert_true (shumate_vector_index_bitset_get (bitset, 0));
  g_assert_true (shumate_vector_index_bitset_get (bitset, 1));
  g_assert_true (shumate_vector_index_description_has_geometry_type (index_description, "lines"));
  g_assert_false (shumate_vector_index_description_has_broad_geometry_type (index_description, "lines"));
  shumate_vector_index_bitset_free (bitset);
  shumate_vector_index_description_free (index_description);

  bitset = compute_bitset (reader, "[\"in\", [\"get\", \"name\"], [\"literal\", [\"test1\", \"helloworld\"]]]", "lines", NULL);
  g_assert_true (shumate_vector_index_bitset_get (bitset, 0));
  g_assert_false (shumate_vector_index_bitset_get (bitset, 1));
  shumate_vector_index_bitset_free (bitset);

  bitset = compute_bitset (reader, "[\"has\", \"name\"]", "lines", NULL);
  g_assert_true (shumate_vector_index_bitset_get (bitset, 0));
  g_assert_false (shumate_vector_index_bitset_get (bitset, 1));
  shumate_vector_index_bitset_free (bitset);

  /* Test fallback with an expression that isn't indexed */
  bitset = compute_bitset (reader, "[\">=\", [\"get\", \"number\"], [\"zoom\"]]", "polygons", NULL);
  g_assert_true (shumate_vector_index_bitset_get (bitset, 0));
  g_assert_false (shumate_vector_index_bitset_get (bitset, 1));
  shumate_vector_index_bitset_free (bitset);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/vector-index/bitset", test_vector_index_bitset);
  g_test_add_func ("/vector-index/description", test_vector_index_description);
  g_test_add_func ("/vector-index/eval", test_vector_index_eval);

  return g_test_run ();
}
