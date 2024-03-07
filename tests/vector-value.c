#undef G_DISABLE_ASSERT

#include <gtk/gtk.h>
#include <shumate/shumate.h>
#include "shumate/vector/shumate-vector-value-private.h"

static void
test_vector_value_literal (void)
{
  g_auto(ShumateVectorValue) value = SHUMATE_VECTOR_VALUE_INIT;
  double number;
  gboolean boolean;
  const char *string;

  shumate_vector_value_unset (&value);

  shumate_vector_value_set_number (&value, 3.1415);
  g_assert_true (shumate_vector_value_get_number (&value, &number));
  g_assert_cmpfloat (3.1415, ==, number);

  shumate_vector_value_set_boolean (&value, TRUE);
  g_assert_true (shumate_vector_value_get_boolean (&value, &boolean));
  g_assert_true (boolean);

  shumate_vector_value_set_boolean (&value, FALSE);
  g_assert_true (shumate_vector_value_get_boolean (&value, &boolean));
  g_assert_false (boolean);

  shumate_vector_value_set_string (&value, "Hello, world!");
  g_assert_true (shumate_vector_value_get_string (&value, &string));
  g_assert_cmpstr ("Hello, world!", ==, string);
}


static void
test_vector_value_from_gvalue (void)
{
  g_auto(ShumateVectorValue) value = SHUMATE_VECTOR_VALUE_INIT;
  g_auto(GValue) gvalue = G_VALUE_INIT;
  double number;
  gboolean boolean;
  const char *string;

  shumate_vector_value_unset (&value);

  g_value_init (&gvalue, G_TYPE_DOUBLE);
  g_value_set_double (&gvalue, 3.1415);
  g_assert_true (shumate_vector_value_set_from_g_value (&value, &gvalue));
  g_assert_true (shumate_vector_value_get_number (&value, &number));
  g_assert_cmpfloat (3.1415, ==, number);
  g_value_unset (&gvalue);

  g_value_init (&gvalue, G_TYPE_BOOLEAN);
  g_value_set_boolean (&gvalue, TRUE);
  g_assert_true (shumate_vector_value_set_from_g_value (&value, &gvalue));
  g_assert_true (shumate_vector_value_get_boolean (&value, &boolean));
  g_assert_true (boolean);

  g_value_set_boolean (&gvalue, FALSE);
  shumate_vector_value_set_from_g_value (&value, &gvalue);
  g_assert_true (shumate_vector_value_get_boolean (&value, &boolean));
  g_assert_false (boolean);
  g_value_unset (&gvalue);

  g_value_init (&gvalue, G_TYPE_STRING);
  g_value_set_string (&gvalue, "Hello, world!");
  g_assert_true (shumate_vector_value_set_from_g_value (&value, &gvalue));
  g_assert_true (shumate_vector_value_get_string (&value, &string));
  g_assert_cmpstr ("Hello, world!", ==, string);
  g_value_unset (&gvalue);
}

static void
test_vector_value_get_color (void)
{
  g_auto(ShumateVectorValue) value = SHUMATE_VECTOR_VALUE_INIT;
  GdkRGBA color, correct_color;

  gdk_rgba_parse (&correct_color, "goldenrod");
  shumate_vector_value_set_string (&value, "goldenrod");

  g_assert_true (shumate_vector_value_get_color (&value, &color));
  g_assert_true (gdk_rgba_equal (&color, &correct_color));

  /* Try again to make sure caching works */
  g_assert_true (shumate_vector_value_get_color (&value, &color));
  g_assert_true (gdk_rgba_equal (&color, &correct_color));

  shumate_vector_value_set_string (&value, "not a real color");
  g_assert_false (shumate_vector_value_get_color (&value, &color));
  /* Try again to make sure caching works */
  g_assert_false (shumate_vector_value_get_color (&value, &color));
}


static void
test_vector_value_equal (void)
{
  g_auto(ShumateVectorValue) value1 = SHUMATE_VECTOR_VALUE_INIT;
  g_auto(ShumateVectorValue) value2 = SHUMATE_VECTOR_VALUE_INIT;
  GdkRGBA color;

  /* Both are initialized to NULL so they should be equal */
  g_assert_true (shumate_vector_value_equal (&value1, &value2));

  shumate_vector_value_set_number (&value1, 1.0);
  shumate_vector_value_set_number (&value2, 1.0);
  g_assert_true (shumate_vector_value_equal (&value1, &value2));

  shumate_vector_value_set_number (&value1, 1.0);
  shumate_vector_value_set_number (&value2, 2.0);
  g_assert_false (shumate_vector_value_equal (&value1, &value2));

  shumate_vector_value_set_boolean (&value1, TRUE);
  shumate_vector_value_set_boolean (&value2, TRUE);
  g_assert_true (shumate_vector_value_equal (&value1, &value2));

  shumate_vector_value_set_number (&value1, FALSE);
  shumate_vector_value_set_number (&value2, TRUE);
  g_assert_false (shumate_vector_value_equal (&value1, &value2));

  shumate_vector_value_set_string (&value1, "Hello, world!");
  shumate_vector_value_set_string (&value2, "Hello, world!");
  g_assert_true (shumate_vector_value_equal (&value1, &value2));

  shumate_vector_value_set_string (&value1, "Hello, world!");
  shumate_vector_value_set_string (&value2, "Goodbye, world!");
  g_assert_false (shumate_vector_value_equal (&value1, &value2));

  gdk_rgba_parse (&color, "magenta");
  shumate_vector_value_set_color (&value1, &color);
  shumate_vector_value_set_color (&value2, &color);
  g_assert_true (shumate_vector_value_equal (&value1, &value2));

  shumate_vector_value_set_color (&value1, &color);
  gdk_rgba_parse (&color, "purple");
  shumate_vector_value_set_color (&value2, &color);
  g_assert_false (shumate_vector_value_equal (&value1, &value2));

  shumate_vector_value_set_string (&value1, "Hello, world!");
  shumate_vector_value_set_number (&value2, 1.0);
  g_assert_false (shumate_vector_value_equal (&value1, &value2));

  shumate_vector_value_set_number (&value1, TRUE);
  shumate_vector_value_set_boolean (&value2, 1.0);
  g_assert_false (shumate_vector_value_equal (&value1, &value2));

  shumate_vector_value_unset (&value1);
  shumate_vector_value_set_number (&value2, 0.0);
  g_assert_false (shumate_vector_value_equal (&value1, &value2));
}


static void
test_vector_value_copy (void)
{
  g_auto(ShumateVectorValue) value1 = SHUMATE_VECTOR_VALUE_INIT;
  g_auto(ShumateVectorValue) value2 = SHUMATE_VECTOR_VALUE_INIT;
  GdkRGBA color;

  shumate_vector_value_copy (&value1, &value2);
  g_assert_true (shumate_vector_value_equal (&value1, &value2));

  gdk_rgba_parse (&color, "red");
  shumate_vector_value_set_color (&value1, &color);
  shumate_vector_value_copy (&value1, &value2);
  g_assert_true (shumate_vector_value_equal (&value1, &value2));

  shumate_vector_value_set_string (&value1, "Hello, world!");
  shumate_vector_value_copy (&value1, &value2);
  g_assert_true (shumate_vector_value_equal (&value1, &value2));
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/vector/value/literal", test_vector_value_literal);
  g_test_add_func ("/vector/value/from-gvalue", test_vector_value_from_gvalue);
  g_test_add_func ("/vector/value/get-color", test_vector_value_get_color);
  g_test_add_func ("/vector/value/equal", test_vector_value_equal);
  g_test_add_func ("/vector/value/copy", test_vector_value_copy);

  return g_test_run ();
}
