#include <gtk/gtk.h>
#include <shumate/shumate.h>
#include "shumate/vector/shumate-vector-expression-interpolate-private.h"
#include "shumate/vector/shumate-vector-expression-filter-private.h"


static void
test_vector_expression_parse (void)
{
  g_autoptr(GError) error = NULL;
  g_autoptr(JsonNode) node1 = json_from_string ("{\"stops\": [[12, 1], [13, 2], [14, 5], [16, 9]]}", NULL);
  g_autoptr(JsonNode) node2 = json_from_string ("1.0", NULL);
  g_autoptr(ShumateVectorExpression) expr1 = NULL;
  g_autoptr(ShumateVectorExpression) expr2 = NULL;
  g_autoptr(ShumateVectorExpression) expr3 = NULL;

  expr1 = shumate_vector_expression_from_json (node1, &error);
  g_assert_no_error (error);
  g_assert_true (SHUMATE_IS_VECTOR_EXPRESSION_INTERPOLATE (expr1));

  expr2 = shumate_vector_expression_from_json (node2, &error);
  g_assert_no_error (error);
  g_assert_true (SHUMATE_IS_VECTOR_EXPRESSION_FILTER (expr2));

  expr3 = shumate_vector_expression_from_json (NULL, &error);
  g_assert_no_error (error);
  g_assert_true (SHUMATE_IS_VECTOR_EXPRESSION_FILTER (expr3));
}


static void
test_vector_expression_literal (void)
{
  g_auto(ShumateVectorValue) value = SHUMATE_VECTOR_VALUE_INIT;
  g_autoptr(ShumateVectorExpression) expr = NULL;
  double result;

  shumate_vector_value_set_number (&value, 3.1415);
  expr = shumate_vector_expression_filter_from_literal (&value);

  result = shumate_vector_expression_eval_number (expr, NULL, -10);
  g_assert_cmpfloat (3.1415, ==, result);
}


static void
check_interpolate (ShumateVectorExpression *expression)
{
  ShumateVectorRenderScope scope;

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
test_vector_expression_interpolate (void)
{
  g_autoptr(GError) error = NULL;
  g_autoptr(JsonNode) node = json_from_string ("{\"stops\": [[12, 1], [13, 2], [14, 5], [16, 9]]}", NULL);
  g_autoptr(ShumateVectorExpression) expression;

  g_assert_nonnull (node);

  expression = shumate_vector_expression_from_json (node, &error);
  g_assert_no_error (error);

  check_interpolate (expression);
}

static void
test_vector_expression_interpolate_filter (void)
{
  g_autoptr(GError) error = NULL;
  g_autoptr(JsonNode) node = json_from_string ("[\"interpolate\", [\"linear\"], [\"zoom\"], 12, 1, 13, 2, 14, 5, 16, 9]", NULL);
  g_autoptr(ShumateVectorExpression) expression;

  g_assert_nonnull (node);

  expression = shumate_vector_expression_from_json (node, &error);
  g_assert_no_error (error);

  check_interpolate (expression);
}


static void
check_interpolate_color (ShumateVectorExpression *expression)
{
  ShumateVectorRenderScope scope;
  GdkRGBA color, correct_color;

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

static void
test_vector_expression_interpolate_color (void)
{
  g_autoptr(GError) error = NULL;
  g_autoptr(JsonNode) node = json_from_string ("{\"stops\": [[12, \"#00224466\"], [13, \"#88AACCEE\"]]}", NULL);
  g_autoptr(ShumateVectorExpression) expression;

  g_assert_nonnull (node);

  expression = shumate_vector_expression_from_json (node, &error);
  g_assert_no_error (error);

  check_interpolate_color (expression);
}

static void
test_vector_expression_interpolate_color_filter (void)
{
  g_autoptr(GError) error = NULL;
  g_autoptr(JsonNode) node = json_from_string ("[\"interpolate\", [\"linear\"], [\"zoom\"], 12, \"#00224466\", 13, \"#88AACCEE\"]", NULL);
  g_autoptr(ShumateVectorExpression) expression;

  g_assert_nonnull (node);

  expression = shumate_vector_expression_from_json (node, &error);
  g_assert_no_error (error);

  check_interpolate_color (expression);
}


static gboolean
filter_with_scope (ShumateVectorRenderScope *scope, const char *filter)
{
  g_autoptr(GError) error = NULL;
  g_autoptr(JsonNode) node = NULL;
  g_autoptr(ShumateVectorExpression) expression = NULL;

  node = json_from_string (filter, &error);
  g_assert_no_error (error);

  expression = shumate_vector_expression_from_json (node, &error);
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

  g_assert_true  (filter ("[\"==\", null, null]"));
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
  g_assert_true (filter ("[\"==\", [\"match\", \"a\", \"b\", 2, \"c\", 3, \"a\", 1, 0], 1]"));
  g_assert_true (filter ("[\"==\", [\"match\", \"b\", 2], 2]"));
  g_assert_true (filter ("[\"==\", [\"match\", 3, [1, 2], \"x\", [3, 4, 5], \"y\", \"z\"], \"y\"]"));

  g_assert_true (filter ("[\"==\", [\"+\", 3, 1, 7], 11]"));
  g_assert_true (filter ("[\"==\", [\"-\", 3, 1], 2]"));
  g_assert_true (filter ("[\"==\", [\"-\", 1], -1]"));
  g_assert_true (filter ("[\"==\", [\"*\", 5, 6, 7], 210]"));
  g_assert_true (filter ("[\"==\", [\"/\", 10, 4], 2.5]"));
  g_assert_true (filter ("[\"==\", -1, [\"%\", -21, 4]]"));

  g_assert_true (filter ("[\">=\", 2, [\"^\", [\"e\"], [\"ln2\"]]]"));
  g_assert_true (filter ("[\"<=\", 1.9999999999, [\"^\", [\"e\"], [\"ln2\"]]]"));
  g_assert_true (filter ("[\"==\", 1, [\"abs\", -1]]"));
  g_assert_true (filter ("[\"==\", 1, [\"abs\", 1]]"));
  g_assert_true (filter ("[\"==\", 0, [\"acos\", 1]]"));
  g_assert_true (filter ("[\"==\", 0, [\"asin\", 0]]"));
  g_assert_true (filter ("[\"==\", 0, [\"atan\", 0]]"));
  g_assert_true (filter ("[\"==\", 0, [\"ceil\", -0.5]]"));
  g_assert_true (filter ("[\"==\", -1, [\"cos\", [\"pi\"]]]"));
  g_assert_true (filter ("[\"==\", -1, [\"floor\", -0.5]]"));
  g_assert_true (filter ("[\"==\", 1, [\"ln\", [\"e\"]]]"));
  g_assert_true (filter ("[\"==\", 2, [\"log10\", 100]]"));
  g_assert_true (filter ("[\"==\", 6, [\"log2\", 64]]"));
  g_assert_true (filter ("[\"==\", 6, [\"max\", -10, 3, 6, -10000]]"));
  g_assert_true (filter ("[\"==\", -10000, [\"min\", -10, 3, 6, -10000]]"));
  g_assert_true (filter ("[\"==\", 0, [\"round\", 0.49999]]"));
  g_assert_true (filter ("[\"==\", 1, [\"round\", 0.5]]"));
  g_assert_true (filter ("[\">=\", 0.0000000000001, [\"sin\", [\"pi\"]]]"));
  g_assert_true (filter ("[\"==\", 12, [\"sqrt\", 144]]"));
  g_assert_true (filter ("[\">=\", 0.0000000000001, [\"tan\", [\"pi\"]]]"));

  g_assert_true (filter ("[\"==\", [\"coalesce\", null, [\"*\", 0, \"b\"], 2, 3], 2]"));
  g_assert_true (filter ("[\"==\", [\"coalesce\", null, [\"*\", 0, \"b\"]], null]"));
  g_assert_true (filter ("[\"==\", [\"coalesce\", \"red\"], \"red\"]"));

  g_assert_true (filter ("[\"==\", [\"concat\", \"hello\", 10, \"world\", true], \"hello10worldtrue\"]"));
  g_assert_true (filter ("[\"==\", [\"downcase\", \"HeLlO, WoRlD!\"], \"hello, world!\"]"));
  g_assert_true (filter ("[\"==\", [\"upcase\", \"HeLlO, WoRlD!\"], \"HELLO, WORLD!\"]"));
  g_assert_true (filter ("[\">\", [\"literal\", \"oranges\"], \"apples\"]"));
  g_assert_true (filter ("[\"<\", [\"literal\", \"apples\"], \"oranges\"]"));
  g_assert_true (filter ("[\">=\", [\"literal\", \"oranges\"], \"apples\"]"));
  g_assert_true (filter ("[\"<=\", [\"literal\", \"apples\"], \"oranges\"]"));
  g_assert_true (filter ("[\">=\", [\"literal\", \"apples\"], \"apples\"]"));
  g_assert_true (filter ("[\"<=\", [\"literal\", \"oranges\"], \"oranges\"]"));

  g_assert_true (filter ("[\"==\", [\"at\", 0, [\"literal\", [\"a\", \"b\", \"c\"]]], \"a\"]"));
  g_assert_true (filter ("[\"==\", [\"at\", 1, [\"literal\", [\"a\", \"b\", \"c\"]]], \"b\"]"));
  g_assert_false (filter ("[\"==\", [\"at\", 3, [\"literal\", [\"a\", \"b\", \"c\"]]], null]"));
  g_assert_true (filter ("[\"==\", [\"index-of\", 2, [\"literal\", [1, 2, 3]]], 1]"));
  g_assert_true (filter ("[\"==\", [\"index-of\", 4, [\"literal\", [1, 2, 3]]], -1]"));
  g_assert_true (filter ("[\"==\", [\"index-of\", \"!\", \"Hello, \U0001F30E!\"], 8]"));
  g_assert_true (filter ("[\"==\", [\"index-of\", \"world\", \"Hello, world!\"], 7]"));
  g_assert_true (filter ("[\"==\", [\"index-of\", \"WORLD\", \"Hello, world!\"], -1]"));
  g_assert_true (filter ("[\"==\", [\"index-of\", \"Hello\", \"Hello, world!\", 1], -1]"));
  g_assert_true (filter ("[\"==\", [\"length\", [\"literal\", []]], 0]"));
  g_assert_true (filter ("[\"==\", [\"length\", [\"literal\", [\"a\", \"b\", \"c\"]]], 3]"));
  g_assert_true (filter ("[\"==\", [\"length\", \"Hello, \U0001F30E!\"], 9]"));
  g_assert_true (filter ("[\"==\", [\"slice\", [\"literal\", [\"a\", \"b\", \"c\"]], 0, 2], [\"literal\", [\"a\", \"b\"]]]"));
  g_assert_true (filter ("[\"==\", [\"slice\", [\"literal\", [\"a\", \"b\", \"c\"]], 1, 2], [\"literal\", [\"b\"]]]"));
  g_assert_true (filter ("[\"==\", [\"slice\", \"Hello, \U0001F30E!\", 7], \"\U0001F30E!\"]"));
  g_assert_true (filter ("[\"==\", [\"slice\", \"Hello, \U0001F30E!\", 7, 8], \"\U0001F30E\"]"));

  g_assert_true (filter ("[\"==\", [\"literal\", \"hello\"], \"HELLO\", [\"collator\", {\"case-sensitive\": false}]]"));
  g_assert_true (filter ("[\"!=\", [\"literal\", \"hello\"], \"HELLO\", [\"collator\", {\"case-sensitive\": true}]]"));
  g_assert_true (filter ("[\">\", [\"literal\", \"hello\"], \"a\", [\"collator\", {}]]"));
  g_assert_true (filter ("[\"<\", [\"literal\", \"a\"], \"hello\", [\"collator\", {}]]"));
  g_assert_true (filter ("[\"!=\", [\"resolved-locale\", [\"collator\", {}]], \"foo\"]"));
}


static void
test_vector_expression_variable_binding (void)
{
  g_assert_true  (filter ("[\"let\", \"a\", [\"-\", 15, 5], \"b\", 20, [\"==\", 30, [\"+\", [\"var\", \"a\"], [\"var\", \"b\"]]]]"));

  /* Test nesting */
  g_assert_true  (filter ("[\"let\", \"a\", 10, [\"==\", 20, [\"let\", \"a\", 20, [\"var\", \"a\"]]]]"));
}


static void
test_vector_expression_image (void)
{
  g_autoptr(GdkTexture) texture = NULL;
  g_autoptr(GBytes) json_data = NULL;
  GError *error = NULL;
  ShumateVectorRenderScope scope;

  texture = gdk_texture_new_from_resource ("/org/gnome/shumate/Tests/sprites.png");
  g_assert_no_error (error);
  json_data = g_resources_lookup_data ("/org/gnome/shumate/Tests/sprites.json", G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
  g_assert_no_error (error);

  scope.sprites = shumate_vector_sprite_sheet_new ();
  scope.scale_factor = 1;
  shumate_vector_sprite_sheet_add_page (scope.sprites, texture, g_bytes_get_data (json_data, NULL), 1, &error);
  g_assert_no_error (error);

  g_assert_true (filter_with_scope (&scope, "[\"!=\", null, [\"image\", \"sprite\"]]"));
  g_assert_true (filter_with_scope (&scope, "[\"==\", null, [\"image\", \"does-not-exist\"]]"));

  g_clear_object (&scope.sprites);
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
  g_assert_true  (filter_with_scope (&scope, "[\"!=\", [\"get\", \"name\"], \"HELLO, WORLD!\"]"));
  g_assert_false (filter_with_scope (&scope, "[\"==\", \"name\", \"Goodbye, world!\"]"));
  g_assert_true  (filter_with_scope (&scope, "[\"has\", \"name\"]"));
  // Use concat to avoid optimizations and test the regular code path
  g_assert_true  (filter_with_scope (&scope, "[\"==\", [\"get\", [\"concat\", \"name\"]], \"Hello, world!\"]"));
  g_assert_true  (filter_with_scope (&scope, "[\"has\", [\"concat\", \"name\"]]"));
  g_assert_false (filter_with_scope (&scope, "[\"!has\", \"name\"]"));
  g_assert_false (filter_with_scope (&scope, "[\"!has\", [\"concat\", \"name\"]]"));
  g_assert_false (filter_with_scope (&scope, "[\"has\", \"name:en\"]"));
  g_assert_true  (filter_with_scope (&scope, "[\"!has\", \"name:en\"]"));
  g_assert_false (filter_with_scope (&scope, "[\"has\", [\"concat\", \"name:en\"]]"));
  g_assert_true  (filter_with_scope (&scope, "[\"!has\", [\"concat\", \"name:en\"]]"));
  g_assert_true  (filter_with_scope (&scope, "[\"==\", \"$type\", \"Point\"]"));
  g_assert_true  (filter_with_scope (&scope, "[\"!=\", \"$type\", \"Polygon\"]"));
  g_assert_true  (filter_with_scope (&scope, "[\"!=\", \"$type\", \"NotAShape\"]"));
  g_assert_true  (filter_with_scope (&scope, "[\"==\", [\"geometry-type\"], [\"concat\", \"Point\"]]"));
  g_assert_true  (filter_with_scope (&scope, "[\"!=\", [\"geometry-type\"], [\"concat\", \"Polygon\"]]"));
  g_assert_true  (filter_with_scope (&scope, "[\"==\", \"zoom\", 10]"));
  g_assert_true  (filter_with_scope (&scope, "[\"in\", \"name\", [\"literal\", [\"Hello, world!\", true, 3]]]"));
  g_assert_true  (filter_with_scope (&scope, "[\"!in\", \"name\", [\"literal\", [\"HELLO, WORLD!\", true, 3]]]"));
  g_assert_true  (filter_with_scope (&scope, "[\"==\", [\"concat\", \"Hello, world!\"], \"Hello, world!\"]"));
  g_assert_true  (filter_with_scope (&scope, "[\"!=\", [\"concat\", \"Hello, world!\"], \"HELLO, WORLD!\"]"));

  vector_tile__tile__free_unpacked (scope.tile, NULL);
}


static void
filter_expect_error (const char *filter)
{
  g_autoptr(GError) error = NULL;
  g_autoptr(JsonNode) node = json_from_string (filter, NULL);
  g_autoptr(ShumateVectorExpression) expression = shumate_vector_expression_from_json (node, &error);

  g_assert_error (error, SHUMATE_STYLE_ERROR, SHUMATE_STYLE_ERROR_INVALID_EXPRESSION);
  g_assert_null (expression);
}

static void
test_vector_expression_filter_errors (void)
{
  filter_expect_error ("[\"not an operator\"]");
  filter_expect_error ("[\"in\"]");
  filter_expect_error ("[\"==\", 0, 1, 2, 3]");
  filter_expect_error ("[]");
  filter_expect_error ("[[]]");

  filter_expect_error ("[\"+\"]");
  filter_expect_error ("[\"-\", 1, 2, 3]");
  filter_expect_error ("[\"/\", 1, 2, 3]");
  filter_expect_error ("[\"abs\", 1, 2]");
  filter_expect_error ("[\"%\", 1]");
}


static void
test_vector_expression_format ()
{
  GError *error = NULL;
  g_autoptr(GBytes) vector_data = NULL;
  gconstpointer data;
  gsize len;
  ShumateVectorRenderScope scope;
  g_autoptr(JsonNode) node = json_from_string ("\"***** {name} *****\"", NULL);
  g_autoptr(ShumateVectorExpression) expression;
  g_autofree char *result = NULL;

  expression = shumate_vector_expression_from_json (node, &error);
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
  g_assert_cmpstr (result, ==, "***** Hello, world! *****");

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
  shumate_vector_value_array_append (&array1, &element2);

  string = shumate_vector_value_as_string (&array1);
  g_assert_cmpstr (string, ==, "[Hello, world!, true]");

  shumate_vector_value_start_array (&array2);
  shumate_vector_value_array_append (&array2, &element1);
  shumate_vector_value_array_append (&array2, &element2);

  g_assert_true (shumate_vector_value_equal (&array1, &array2));

  shumate_vector_value_array_append (&array2, &element1);
  g_assert_false (shumate_vector_value_equal (&array1, &array2));

  node = json_from_string ("[\"literal\", [\"Hello, world!\", true, \"Hello, world!\"]]", NULL);
  expression = shumate_vector_expression_from_json (node, &error);
  g_assert_no_error (error);
  shumate_vector_expression_eval (expression, NULL, &eval);

  g_assert_true (shumate_vector_value_equal (&eval, &array2));
}


static void
test_vector_expression_formatted_string ()
{
  g_autoptr(GError) error = NULL;
  g_autoptr(JsonNode) node = NULL;
  g_autoptr(ShumateVectorExpression) expression = NULL;
  g_auto(ShumateVectorValue) eval = SHUMATE_VECTOR_VALUE_INIT;
  g_autofree char *as_string = NULL;
  GPtrArray *format_parts = NULL;
  ShumateVectorFormatPart *part = NULL;

  node = json_from_string ("[\"format\",\
    \"Hello \",\
    [\"concat\", \"world\", \"!\"], {\"font-scale\": 0.1},\
    \"\n\", {\"text-color\": [\"coalesce\", \"red\"]},\
    null,\
    \"test\"\
    ]", &error);
  g_assert_no_error (error);
  expression = shumate_vector_expression_from_json (node, &error);
  g_assert_no_error (error);

  g_assert_true (shumate_vector_expression_eval (expression, NULL, &eval));
  g_assert_true (shumate_vector_value_get_formatted (&eval, &format_parts));
  g_assert_cmpint (format_parts->len, ==, 4);

  as_string = shumate_vector_value_as_string (&eval);
  g_assert_cmpstr (as_string, ==, "Hello world!\ntest");

  part = format_parts->pdata[0];
  g_assert_cmpstr (part->string, ==, "Hello ");
  g_assert_null (part->sprite);
  g_assert_false (part->has_font_scale);
  g_assert_false (part->has_text_color);

  part = format_parts->pdata[1];
  g_assert_cmpstr (part->string, ==, "world!");
  g_assert_null (part->sprite);
  g_assert_true (part->has_font_scale);
  g_assert_cmpfloat (part->font_scale, ==, 0.1);
  g_assert_false (part->has_text_color);

  part = format_parts->pdata[2];
  g_assert_cmpstr (part->string, ==, "\n");
  g_assert_null (part->sprite);
  g_assert_false (part->has_font_scale);
  g_assert_true (part->has_text_color);

  part = format_parts->pdata[3];
  g_assert_cmpstr (part->string, ==, "test");
  g_assert_null (part->sprite);
  g_assert_false (part->has_font_scale);
  g_assert_false (part->has_text_color);
}


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/vector/expression/parse", test_vector_expression_parse);
  g_test_add_func ("/vector/expression/literal", test_vector_expression_literal);
  g_test_add_func ("/vector/expression/interpolate", test_vector_expression_interpolate);
  g_test_add_func ("/vector/expression/interpolate-filter", test_vector_expression_interpolate_filter);
  g_test_add_func ("/vector/expression/interpolate-color", test_vector_expression_interpolate_color);
  g_test_add_func ("/vector/expression/interpolate-color-filter", test_vector_expression_interpolate_color_filter);
  g_test_add_func ("/vector/expression/basic-filter", test_vector_expression_basic_filter);
  g_test_add_func ("/vector/expression/variable-binding", test_vector_expression_variable_binding);
  g_test_add_func ("/vector/expression/image", test_vector_expression_image);
  g_test_add_func ("/vector/expression/feature-filter", test_vector_expression_feature_filter);
  g_test_add_func ("/vector/expression/filter-errors", test_vector_expression_filter_errors);
  g_test_add_func ("/vector/expression/format", test_vector_expression_format);
  g_test_add_func ("/vector/expression/array", test_vector_expression_array);
  g_test_add_func ("/vector/expression/formatted-string", test_vector_expression_formatted_string);

  return g_test_run ();
}
