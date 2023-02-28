#include <gtk/gtk.h>
#include <shumate/shumate.h>
#include "shumate/vector/shumate-vector-expression-literal-private.h"
#include "shumate/vector/shumate-vector-expression-interpolate-private.h"


static void
test_vector_expression_parse (void)
{
  g_autoptr(GError) error = NULL;
  g_autoptr(JsonNode) node1 = json_from_string ("{\"stops\": [[12, 1], [13, 2], [14, 5], [16, 9]]}", NULL);
  g_autoptr(JsonNode) node2 = json_from_string ("1.0", NULL);
  g_autoptr(ShumateVectorExpression) expr1 = NULL;
  g_autoptr(ShumateVectorExpression) expr2 = NULL;
  g_autoptr(ShumateVectorExpression) expr3 = NULL;

  expr1 = shumate_vector_expression_from_json (node1, NULL, &error);
  g_assert_no_error (error);
  g_assert_true (SHUMATE_IS_VECTOR_EXPRESSION_INTERPOLATE (expr1));

  expr2 = shumate_vector_expression_from_json (node2, NULL, &error);
  g_assert_no_error (error);
  g_assert_true (SHUMATE_IS_VECTOR_EXPRESSION_LITERAL (expr2));

  expr3 = shumate_vector_expression_from_json (NULL, NULL, &error);
  g_assert_no_error (error);
  g_assert_true (SHUMATE_IS_VECTOR_EXPRESSION_LITERAL (expr3));
}


static void
test_vector_expression_literal (void)
{
  g_auto(ShumateVectorValue) value = SHUMATE_VECTOR_VALUE_INIT;
  g_autoptr(ShumateVectorExpression) expr = NULL;
  double result;

  shumate_vector_value_set_number (&value, 3.1415);
  expr = shumate_vector_expression_literal_new (&value);

  result = shumate_vector_expression_eval_number (expr, NULL, -10);
  g_assert_cmpfloat (3.1415, ==, result);
}


static void
test_vector_expression_interpolate (void)
{
  g_autoptr(GError) error = NULL;
  g_autoptr(JsonNode) node = json_from_string ("{\"stops\": [[12, 1], [13, 2], [14, 5], [16, 9]]}", NULL);
  g_autoptr(ShumateVectorExpression) expression;
  ShumateVectorRenderScope scope;

  expression = shumate_vector_expression_from_json (node, NULL, &error);
  g_assert_no_error (error);

  /* Test that exact stop values work */
  scope.zoom_level = 12;
  g_assert_cmpfloat (1.0, ==, shumate_vector_expression_eval_number (expression, &scope, -10000.0));
  scope.zoom_level = 13;
  g_assert_cmpfloat (2.0, ==, shumate_vector_expression_eval_number (expression, &scope, -10000.0));
  scope.zoom_level = 14;
  g_assert_cmpfloat (5.0, ==, shumate_vector_expression_eval_number (expression, &scope, -10000.0));
  scope.zoom_level = 16;
  g_assert_cmpfloat (9.0, ==, shumate_vector_expression_eval_number (expression, &scope, -10000.0));

  /* Test that outlier values work */
  scope.zoom_level = 1;
  g_assert_cmpfloat (1.0, ==, shumate_vector_expression_eval_number (expression, &scope, -10000.0));
  scope.zoom_level = 100;
  g_assert_cmpfloat (9.0, ==, shumate_vector_expression_eval_number (expression, &scope, -10000.0));

  /* Test that in-between values work */
  scope.zoom_level = 12.5;
  g_assert_cmpfloat (1.5, ==, shumate_vector_expression_eval_number (expression, &scope, -10000.0));
  scope.zoom_level = 15;
  g_assert_cmpfloat (7.0, ==, shumate_vector_expression_eval_number (expression, &scope, -10000.0));
}


static void
test_vector_expression_interpolate_color (void)
{
  g_autoptr(GError) error = NULL;
  g_autoptr(JsonNode) node = json_from_string ("{\"stops\": [[12, \"#00224466\"], [13, \"#88AACCEE\"]]}", NULL);
  g_autoptr(ShumateVectorExpression) expression;
  ShumateVectorRenderScope scope;
  GdkRGBA color, correct_color;

  expression = shumate_vector_expression_from_json (node, NULL, &error);
  g_assert_no_error (error);

  /* Test that exact stop values work */
  scope.zoom_level = 12;
  shumate_vector_expression_eval_color (expression, &scope, &color);
  gdk_rgba_parse (&correct_color, "#00224466");
  g_assert_true (gdk_rgba_equal (&color, &correct_color));

  scope.zoom_level = 12.5;
  shumate_vector_expression_eval_color (expression, &scope, &color);
  gdk_rgba_parse (&correct_color, "#446688AA");
  g_assert_true (gdk_rgba_equal (&color, &correct_color));

  scope.zoom_level = 13;
  shumate_vector_expression_eval_color (expression, &scope, &color);
  gdk_rgba_parse (&correct_color, "#88AACCEE");
  g_assert_true (gdk_rgba_equal (&color, &correct_color));
}


static gboolean
filter_with_scope (ShumateVectorRenderScope *scope, const char *filter)
{
  g_autoptr(GError) error = NULL;
  g_autoptr(JsonNode) node = NULL;
  g_autoptr(ShumateVectorExpression) expression = NULL;

  node = json_from_string (filter, &error);
  g_assert_no_error (error);

  expression = shumate_vector_expression_from_json (node, NULL, &error);
  g_assert_no_error (error);

  return shumate_vector_expression_eval_boolean (expression, scope, FALSE);
}


static gboolean
filter (const char *filter)
{
  return filter_with_scope (NULL, filter);
}


static void
test_vector_expression_basic_filter (void)
{
  g_assert_true  (filter ("true"));
  g_assert_false (filter ("false"));
  g_assert_false (filter ("[\"!\", true]"));
  g_assert_true  (filter ("[\"!\", false]"));
  g_assert_true  (filter ("[\"any\", false, true]"));
  g_assert_false (filter ("[\"any\", false, false]"));
  g_assert_true  (filter ("[\"none\", false, false]"));
  g_assert_false (filter ("[\"none\", true, false]"));
  g_assert_true  (filter ("[\"all\", true, true]"));
  g_assert_false (filter ("[\"all\", false, true]"));

  g_assert_false (filter ("[\"any\"]"));
  g_assert_true  (filter ("[\"none\"]"));
  g_assert_true  (filter ("[\"all\"]"));

  g_assert_true  (filter ("[\"in\", 10, 20, 10, 13]"));
  g_assert_true  (filter ("[\"!in\", 10, 20, 0, 13]"));
  g_assert_true  (filter ("[\"==\", [\"literal\", []], [\"literal\", []]]"));
  g_assert_true  (filter ("[\"==\", [\"literal\", [10, true, \"A\", null]], [\"literal\", [10, true, \"A\", null]]]"));
  g_assert_true  (filter ("[\"in\", 13, [\"literal\", [10, 20, 0, 13]]]"));

  g_assert_true  (filter ("[\"==\", 10, 10]"));
  g_assert_false (filter ("[\"==\", 10, 20]"));
  g_assert_false (filter ("[\"==\", 10, \"10\"]"));
  g_assert_false (filter ("[\"!=\", 10, 10]"));
  g_assert_true  (filter ("[\"!=\", 10, 20]"));
  g_assert_true  (filter ("[\"!=\", 10, \"10\"]"));
  g_assert_true  (filter ("[\">\", 20, 10]"));
  g_assert_false (filter ("[\">\", 10, 10]"));
  g_assert_false (filter ("[\">\", 5, 10]"));
  g_assert_true  (filter ("[\"<\", 10, 20]"));
  g_assert_false (filter ("[\"<\", 10, 10]"));
  g_assert_false (filter ("[\"<\", 10, 5]"));
  g_assert_true  (filter ("[\">=\", 20, 10]"));
  g_assert_true  (filter ("[\">=\", 10, 10]"));
  g_assert_false (filter ("[\">=\", 5, 10]"));
  g_assert_true  (filter ("[\"<=\", 10, 20]"));
  g_assert_true  (filter ("[\"<=\", 10, 10]"));
  g_assert_false (filter ("[\"<=\", 10, 5]"));

  g_assert_true (filter ("[\"==\", [\"case\", true, 0, 1], 0]"));
  g_assert_true (filter ("[\"==\", [\"case\", false, 0, 1], 1]"));
  g_assert_true (filter ("[\"==\", [\"case\", false, 0, true, 2], 2]"));

  g_assert_true (filter ("[\"==\", [\"+\", 3, 1, 7], 11]"));
  g_assert_true (filter ("[\"==\", [\"-\", 3, 1], 2]"));
  g_assert_true (filter ("[\"==\", [\"-\", 1], -1]"));
  g_assert_true (filter ("[\"==\", [\"*\", 5, 6, 7], 210]"));
  g_assert_true (filter ("[\"==\", [\"/\", 10, 4], 2.5]"));

  g_assert_true (filter ("[\"==\", [\"coalesce\", null, [\"*\", 0, \"b\"], 2, 3], 2]"));
  g_assert_true (filter ("[\"==\", [\"coalesce\", null, [\"*\", 0, \"b\"]], null]"));
}


static void
test_vector_expression_variable_binding (void)
{
  g_assert_true  (filter ("[\"let\", \"a\", [\"-\", 15, 5], \"b\", 20, [\"==\", 30, [\"+\", [\"var\", \"a\"], [\"var\", \"b\"]]]]"));

  /* Test nesting */
  g_assert_true  (filter ("[\"let\", \"a\", 10, [\"==\", 20, [\"let\", \"a\", 20, [\"var\", \"a\"]]]]"));
}


static void
test_vector_expression_feature_filter (void)
{
  GError *error = NULL;
  g_autoptr(GBytes) vector_data = NULL;
  gconstpointer data;
  gsize len;
  ShumateVectorRenderScope scope;

  vector_data = g_resources_lookup_data ("/org/gnome/shumate/Tests/0.pbf", G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
  g_assert_no_error (error);

  data = g_bytes_get_data (vector_data, &len);
  scope.tile = vector_tile__tile__unpack (NULL, len, data);
  g_assert_nonnull (scope.tile);

  scope.zoom_level = 10;

  g_assert_true (shumate_vector_render_scope_find_layer (&scope, "helloworld"));
  scope.feature = scope.layer->features[0];

  g_assert_true  (filter_with_scope (&scope, "[\"==\", \"name\", \"Hello, world!\"]"));
  g_assert_true  (filter_with_scope (&scope, "[\"==\", [\"get\", \"name\"], \"Hello, world!\"]"));
  g_assert_false (filter_with_scope (&scope, "[\"==\", \"name\", \"Goodbye, world!\"]"));
  g_assert_true  (filter_with_scope (&scope, "[\"has\", \"name\"]"));
  g_assert_false (filter_with_scope (&scope, "[\"!has\", \"name\"]"));
  g_assert_false (filter_with_scope (&scope, "[\"has\", \"name:en\"]"));
  g_assert_true  (filter_with_scope (&scope, "[\"!has\", \"name:en\"]"));
  g_assert_true  (filter_with_scope (&scope, "[\"==\", \"$type\", \"Point\"]"));
  g_assert_true  (filter_with_scope (&scope, "[\"==\", \"zoom\", 10]"));

  vector_tile__tile__free_unpacked (scope.tile, NULL);
}


static void
filter_expect_error (const char *filter)
{
  g_autoptr(GError) error = NULL;
  g_autoptr(JsonNode) node = json_from_string (filter, NULL);
  g_autoptr(ShumateVectorExpression) expression = shumate_vector_expression_from_json (node, NULL, &error);

  g_assert_error (error, SHUMATE_STYLE_ERROR, SHUMATE_STYLE_ERROR_INVALID_EXPRESSION);
  g_assert_null (expression);
}

static void
test_vector_expression_filter_errors (void)
{
  filter_expect_error ("[\"not an operator\"]");
  filter_expect_error ("[\"in\"]");
  filter_expect_error ("[\"==\", 0, 1, 2]");
  filter_expect_error ("[]");
  filter_expect_error ("[[]]");
}


static void
test_vector_expression_format ()
{
  GError *error = NULL;
  g_autoptr(GBytes) vector_data = NULL;
  gconstpointer data;
  gsize len;
  ShumateVectorRenderScope scope;
  g_autoptr(JsonNode) node = json_from_string ("\"{name}\"", NULL);
  g_autoptr(ShumateVectorExpression) expression;
  g_autofree char *result = NULL;

  expression = shumate_vector_expression_from_json (node, NULL, &error);
  g_assert_no_error (error);

  vector_data = g_resources_lookup_data ("/org/gnome/shumate/Tests/0.pbf", G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
  g_assert_no_error (error);

  data = g_bytes_get_data (vector_data, &len);
  scope.tile = vector_tile__tile__unpack (NULL, len, data);
  g_assert_nonnull (scope.tile);

  scope.zoom_level = 10;

  g_assert_true (shumate_vector_render_scope_find_layer (&scope, "helloworld"));
  scope.feature = scope.layer->features[0];

  result = shumate_vector_expression_eval_string (expression, &scope, NULL);
  g_assert_cmpstr (result, ==, "Hello, world!");

  vector_tile__tile__free_unpacked (scope.tile, NULL);
}


static void
test_vector_expression_array ()
{
  g_autofree char *string = NULL;
  g_auto(ShumateVectorValue) element1 = SHUMATE_VECTOR_VALUE_INIT;
  g_auto(ShumateVectorValue) element2 = SHUMATE_VECTOR_VALUE_INIT;
  g_auto(ShumateVectorValue) array1 = SHUMATE_VECTOR_VALUE_INIT;
  g_auto(ShumateVectorValue) array2 = SHUMATE_VECTOR_VALUE_INIT;

  g_autoptr(GError) error = NULL;
  g_autoptr(JsonNode) node = NULL;
  g_autoptr(ShumateVectorExpression) expression = NULL;
  g_auto(ShumateVectorValue) eval = SHUMATE_VECTOR_VALUE_INIT;

  shumate_vector_value_set_string (&element1, "Hello, world!");
  shumate_vector_value_set_boolean (&element2, TRUE);

  shumate_vector_value_start_array (&array1);
  shumate_vector_value_array_append (&array1, &element1);
  g_assert_false (shumate_vector_value_array_contains (&array1, &element2));

  shumate_vector_value_array_append (&array1, &element2);
  g_assert_true (shumate_vector_value_array_contains (&array1, &element2));

  string = shumate_vector_value_as_string (&array1);
  g_assert_cmpstr (string, ==, "[Hello, world!, true]");

  shumate_vector_value_start_array (&array2);
  shumate_vector_value_array_append (&array2, &element1);
  shumate_vector_value_array_append (&array2, &element2);

  g_assert_true (shumate_vector_value_equal (&array1, &array2));

  shumate_vector_value_array_append (&array2, &element1);
  g_assert_false (shumate_vector_value_equal (&array1, &array2));

  node = json_from_string ("[\"literal\", [\"Hello, world!\", true, \"Hello, world!\"]]", NULL);
  expression = shumate_vector_expression_from_json (node, NULL, &error);
  g_assert_no_error (error);
  shumate_vector_expression_eval (expression, NULL, &eval);

  g_assert_true (shumate_vector_value_equal (&eval, &array2));
}


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/vector/expression/parse", test_vector_expression_parse);
  g_test_add_func ("/vector/expression/literal", test_vector_expression_literal);
  g_test_add_func ("/vector/expression/interpolate", test_vector_expression_interpolate);
  g_test_add_func ("/vector/expression/interpolate-color", test_vector_expression_interpolate_color);
  g_test_add_func ("/vector/expression/basic-filter", test_vector_expression_basic_filter);
  g_test_add_func ("/vector/expression/variable-binding", test_vector_expression_variable_binding);
  g_test_add_func ("/vector/expression/feature-filter", test_vector_expression_feature_filter);
  g_test_add_func ("/vector/expression/filter-errors", test_vector_expression_filter_errors);
  g_test_add_func ("/vector/expression/format", test_vector_expression_format);
  g_test_add_func ("/vector/expression/array", test_vector_expression_array);

  return g_test_run ();
}
