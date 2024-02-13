#include <shumate/shumate.h>
#include "shumate/vector/shumate-vector-index-private.h"

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

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/vector-index/bitset", test_vector_index_bitset);

  return g_test_run ();
}
