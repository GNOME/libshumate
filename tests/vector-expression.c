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

  expr1 = shumate_vector_expression_from_json (node1, &error);
  g_assert_no_error (error);
  g_assert_true (SHUMATE_IS_VECTOR_EXPRESSION_INTERPOLATE (expr1));

  expr2 = shumate_vector_expression_from_json (node2, &error);
  g_assert_no_error (error);
  g_assert_true (SHUMATE_IS_VECTOR_EXPRESSION_LITERAL (expr2));

  expr3 = shumate_vector_expression_from_json (NULL, &error);
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

  expression = shumate_vector_expression_from_json (node, &error);
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

  expression = shumate_vector_expression_from_json (node, &error);
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


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/vector/expression/parse", test_vector_expression_parse);
  g_test_add_func ("/vector/expression/literal", test_vector_expression_literal);
  g_test_add_func ("/vector/expression/interpolate", test_vector_expression_interpolate);
  g_test_add_func ("/vector/expression/interpolate-color", test_vector_expression_interpolate_color);

  return g_test_run ();
}
