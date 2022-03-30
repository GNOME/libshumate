#include <gtk/gtk.h>
#include <shumate/shumate.h>
#include "shumate/vector/shumate-vector-collision-private.h"


static void
test_vector_collision_nonoverlapping (void)
{
  ShumateVectorCollision *collision;
  g_autoptr(GList) markers = NULL;

  collision = shumate_vector_collision_new ();

  shumate_vector_collision_insert (collision, 0, 0, 0, -1, 1, TRUE);

  /* Far away markers */
  markers = g_list_prepend (markers, shumate_vector_collision_insert (collision, 0, 10, 10, 1, 1, TRUE));
  markers = g_list_prepend (markers, shumate_vector_collision_insert (collision, 0, 100000, 0, 1, 1, TRUE));
  markers = g_list_prepend (markers, shumate_vector_collision_insert (collision, 0, 0, 100000, 1, 1, TRUE));
  markers = g_list_prepend (markers, shumate_vector_collision_insert (collision, 0, 100000, 100000, 1, 1, TRUE));

  /* Edge only */
  markers = g_list_prepend (markers, shumate_vector_collision_insert (collision, 0, 2, 0, 1, 1, TRUE));
  markers = g_list_prepend (markers, shumate_vector_collision_insert (collision, 0, -2, 0, 1, 1, TRUE));
  markers = g_list_prepend (markers, shumate_vector_collision_insert (collision, 0, 0, 2, 1, 1, TRUE));
  markers = g_list_prepend (markers, shumate_vector_collision_insert (collision, 0, 0, -2, 1, 1, TRUE));

  shumate_vector_collision_recalc (collision, 0, 0);

  for (GList *l = markers; l != NULL; l = l->next)
    g_assert_true (((ShumateVectorCollisionMarker *)l->data)->visible);

  shumate_vector_collision_free (collision);
}


static void
test_vector_collision_overlapping (void)
{
  ShumateVectorCollision *collision;
  ShumateVectorCollisionMarker *visible_marker;
  g_autoptr(GList) markers = NULL;

  collision = shumate_vector_collision_new ();

  markers = g_list_prepend (markers, shumate_vector_collision_insert (collision, 0, 0, 0, 1, 1, TRUE));
  markers = g_list_prepend (markers, shumate_vector_collision_insert (collision, 0, 0, 0, 2, 2, TRUE));
  markers = g_list_prepend (markers, shumate_vector_collision_insert (collision, 0, 1, 1, 1, 1, TRUE));

  /* The current implementation uses g_list_prepend, so the last inserted
   * marker has priority. */
  visible_marker = shumate_vector_collision_insert (collision, 0, 0, 0, 1, 1, TRUE);
  markers = g_list_prepend (markers, visible_marker);

  shumate_vector_collision_recalc (collision, 0, 0);

  for (GList *l = markers; l != NULL; l = l->next)
    {
      if (l->data == visible_marker)
        g_assert_true (((ShumateVectorCollisionMarker *)l->data)->visible);
      else
        g_assert_false (((ShumateVectorCollisionMarker *)l->data)->visible);
    }

  shumate_vector_collision_free (collision);
}


static void
test_vector_collision_zoom (void)
{
  ShumateVectorCollision *collision;
  ShumateVectorCollisionMarker *marker1, *marker2;

  collision = shumate_vector_collision_new ();

  marker1 = shumate_vector_collision_insert (collision, 1, 0, 0, 1, 1, TRUE);
  marker2 = shumate_vector_collision_insert (collision, 2, 2, 2, 1, 1, TRUE);

  shumate_vector_collision_recalc (collision, 0, 1);
  g_assert_false (marker1->visible);
  g_assert_true (marker2->visible);

  shumate_vector_collision_recalc (collision, 0, 2);
  g_assert_true (marker1->visible);
  g_assert_true (marker2->visible);

  shumate_vector_collision_recalc (collision, 0, 1);
  g_assert_false (marker1->visible);
  g_assert_true (marker2->visible);

  shumate_vector_collision_free (collision);
}


static void
test_vector_collision_rotate (void)
{
  ShumateVectorCollision *collision;
  ShumateVectorCollisionMarker *marker1, *marker2;

  collision = shumate_vector_collision_new ();

  marker1 = shumate_vector_collision_insert (collision, 0, 0, 0, 10, 1, TRUE);
  marker2 = shumate_vector_collision_insert (collision, 0, 0, 3, 10, 1, TRUE);

  shumate_vector_collision_recalc (collision, 0, 0);
  g_assert_true (marker1->visible);
  g_assert_true (marker2->visible);

  shumate_vector_collision_recalc (collision, G_PI / 2.0, 0);
  g_assert_false (marker1->visible);
  g_assert_true (marker2->visible);

  shumate_vector_collision_recalc (collision, G_PI, 0);
  g_assert_true (marker1->visible);
  g_assert_true (marker2->visible);

  shumate_vector_collision_free (collision);
}


int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/vector-collision/nonoverlapping", test_vector_collision_nonoverlapping);
  g_test_add_func ("/vector-collision/overlapping", test_vector_collision_overlapping);
  g_test_add_func ("/vector-collision/zoom", test_vector_collision_zoom);
  g_test_add_func ("/vector-collision/rotate", test_vector_collision_rotate);

  return g_test_run ();
}
