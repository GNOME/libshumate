/*
 * Copyright (C) 2021 James Westman <james@jwestman.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <https://www.gnu.org/licenses/>.
 */

#include "shumate-vector-renderer.h"
#include "shumate-vector-utils-private.h"

gboolean
shumate_vector_json_get_object (JsonNode *node, JsonObject **dest, GError **error)
{
  g_assert (node != NULL);
  g_assert (dest != NULL);

  if (!JSON_NODE_HOLDS_OBJECT (node))
    {
      g_set_error (error,
                   SHUMATE_STYLE_ERROR,
                   SHUMATE_STYLE_ERROR_MALFORMED_STYLE,
                   "Expected object, got %s",
                   json_node_type_name (node));
      return FALSE;
    }

  *dest = json_node_get_object (node);
  return TRUE;
}


gboolean
shumate_vector_json_get_array (JsonNode *node, JsonArray **dest, GError **error)
{
  g_assert (node != NULL);
  g_assert (dest != NULL);

  if (!JSON_NODE_HOLDS_ARRAY (node))
    {
      g_set_error (error,
                   SHUMATE_STYLE_ERROR,
                   SHUMATE_STYLE_ERROR_MALFORMED_STYLE,
                   "Expected array, got %s",
                   json_node_type_name (node));
      return FALSE;
    }

  *dest = json_node_get_array (node);
  return TRUE;
}

gboolean
shumate_vector_json_get_string (JsonNode    *node,
                                const char **dest,
                                GError     **error)
{
  g_assert (node != NULL);
  g_assert (dest != NULL);

  if (!JSON_NODE_HOLDS_VALUE (node) || json_node_get_value_type (node) != G_TYPE_STRING)
    {
      g_set_error (error,
                   SHUMATE_STYLE_ERROR,
                   SHUMATE_STYLE_ERROR_MALFORMED_STYLE,
                   "Expected string, got %s",
                   json_node_type_name (node));
      return FALSE;
    }

  *dest = json_node_get_string (node);
  return TRUE;
}


JsonNode *
get_member (JsonObject *object,
            const char *name)
{
  if (object == NULL)
    return FALSE;
  return json_object_get_member (object, name);
}


gboolean
shumate_vector_json_get_object_member (JsonObject  *object,
                                       const char  *name,
                                       JsonObject **dest,
                                       GError     **error)
{
  JsonNode *node;

  g_assert (dest != NULL);

  node = get_member (object, name);
  if (node == NULL)
    {
      *dest = NULL;
      return TRUE;
    }

  return shumate_vector_json_get_object (node, dest, error);
}


gboolean
shumate_vector_json_get_array_member (JsonObject  *object,
                                      const char  *name,
                                      JsonArray  **dest,
                                      GError     **error)
{
  JsonNode *node;

  g_assert (dest != NULL);

  node = get_member (object, name);
  if (node == NULL)
    {
      *dest = NULL;
      return TRUE;
    }

  return shumate_vector_json_get_array (node, dest, error);
}


gboolean
shumate_vector_json_get_string_member (JsonObject  *object,
                                       const char  *name,
                                       const char **dest,
                                       GError     **error)
{
  JsonNode *node;

  g_assert (dest != NULL);

  node = get_member (object, name);
  if (node == NULL)
    {
      *dest = NULL;
      return TRUE;
    }

  return shumate_vector_json_get_string (node, dest, error);
}


void
shumate_vector_point_iter_init (ShumateVectorPointIter  *iter,
                                ShumateVectorLineString *linestring)
{
  *iter = (ShumateVectorPointIter) {
    .num_points = linestring->n_points,
    .points = linestring->points,
    .current_point = 0,
    .distance = 0,
    .reversed = FALSE,
  };
}


gboolean
shumate_vector_point_iter_is_at_end (ShumateVectorPointIter *iter)
{
  if (iter->reversed)
    return iter->current_point == 0;
  else
    return iter->current_point >= iter->num_points - 1;
}


double
shumate_vector_point_iter_next_segment (ShumateVectorPointIter *iter)
{
  double res;

  if (shumate_vector_point_iter_is_at_end (iter))
    return 0;

  res = shumate_vector_point_iter_get_segment_length (iter) - iter->distance;
  iter->distance = 0;

  if (iter->reversed)
    iter->current_point --;
  else
    iter->current_point ++;

  return res;
}


static double
point_distance_sq (ShumateVectorPoint *a,
                   ShumateVectorPoint *b)
{
  double x = a->x - b->x;
  double y = a->y - b->y;
  return x * x + y * y;
}

static double
point_distance (ShumateVectorPoint *a,
                ShumateVectorPoint *b)
{
  return sqrt (point_distance_sq (a, b));
}

static ShumateVectorPoint *
get_prev_point (ShumateVectorPointIter *iter)
{
  g_assert (iter->current_point < iter->num_points);
  return &iter->points[iter->current_point];
}

static ShumateVectorPoint *
get_next_point (ShumateVectorPointIter *iter)
{
  g_assert (iter->current_point < iter->num_points);

  if (iter->reversed)
    {
      if (iter->current_point == 0)
        return &iter->points[0];
      else
        return &iter->points[iter->current_point - 1];
    }
  else
    {
      if (iter->current_point >= iter->num_points - 1)
        return &iter->points[iter->num_points - 1];
      else
        return &iter->points[iter->current_point + 1];
    }
}

static void
normalize (ShumateVectorPoint *point)
{
  double len = sqrt (point->x*point->x + point->y*point->y);
  if (len == 0)
    {
      point->x = 0;
      point->y = 0;
    }
  else
    {
      point->x /= len;
      point->y /= len;
    }
}


double
shumate_vector_point_iter_get_segment_length (ShumateVectorPointIter *iter)
{
  ShumateVectorPoint *prev = get_prev_point (iter), *next = get_next_point (iter);
  return point_distance (prev, next);
}


void
shumate_vector_point_iter_get_segment_center (ShumateVectorPointIter *iter,
                                              double                  remaining_distance,
                                              ShumateVectorPoint     *result)
{
  /* Gets the center point of the rest of the current segment, up to the given remaining distance. */

  ShumateVectorPoint *prev = get_prev_point (iter), *next = get_next_point (iter);
  float distance = MIN (remaining_distance, point_distance (prev, next) - iter->distance) / 2 + iter->distance;

  result->x = next->x - prev->x;
  result->y = next->y - prev->y;
  normalize (result);

  result->x *= distance;
  result->y *= distance;
  result->x += prev->x;
  result->y += prev->y;
}


void
shumate_vector_point_iter_get_current_point (ShumateVectorPointIter *iter,
                                             ShumateVectorPoint     *result)
{
  ShumateVectorPoint *prev = get_prev_point (iter), *next = get_next_point (iter);

  result->x = next->x - prev->x;
  result->y = next->y - prev->y;
  normalize (result);

  result->x *= iter->distance;
  result->y *= iter->distance;
  result->x += prev->x;
  result->y += prev->y;
}


void
shumate_vector_point_iter_advance (ShumateVectorPointIter *iter,
                                   double                  distance)
{
  while (distance > 0 && !shumate_vector_point_iter_is_at_end (iter))
    {
      if (iter->distance + distance > shumate_vector_point_iter_get_segment_length (iter))
        distance -= shumate_vector_point_iter_next_segment (iter);
      else
        {
          iter->distance += distance;
          return;
        }
    }
}


double
shumate_vector_point_iter_get_current_angle (ShumateVectorPointIter *iter)
{
  ShumateVectorPoint *prev = get_prev_point (iter), *next = get_next_point (iter);
  return atan2 (next->y - prev->y, next->x - prev->x);
}


double
shumate_vector_point_iter_get_average_angle (ShumateVectorPointIter *iter,
                                             double                  remaining_distance)
{
  ShumateVectorPointIter iter2 = *iter;
  double sum_x = 0.0, sum_y = 0.0;

  while (remaining_distance > 0 && !shumate_vector_point_iter_is_at_end (&iter2))
    {
      double len = shumate_vector_point_iter_get_segment_length (&iter2);
      double scale;

      if (len != 0)
        {
          scale = MIN (remaining_distance, len - iter2.distance) / len;
          sum_x += (get_next_point (&iter2)->x - get_prev_point (&iter2)->x) * scale;
          sum_y += (get_next_point (&iter2)->y - get_prev_point (&iter2)->y) * scale;
        }

      remaining_distance -= shumate_vector_point_iter_next_segment (&iter2);
    }

  return atan2 (sum_y, sum_x);
}


ShumateVectorLineString *
shumate_vector_line_string_copy (ShumateVectorLineString *linestring)
{
  ShumateVectorLineString *copy = g_new0 (ShumateVectorLineString, 1);
  copy->n_points = linestring->n_points;
  copy->points = g_memdup2 (linestring->points, linestring->n_points * sizeof (ShumateVectorPoint));
  return copy;
}

void
shumate_vector_line_string_free (ShumateVectorLineString *linestring)
{
  g_clear_pointer (&linestring->points, g_free);
  g_free (linestring);
}


double
shumate_vector_line_string_length (ShumateVectorLineString *linestring)
{
  ShumateVectorPointIter iter;
  double len, sum = 0;

  shumate_vector_point_iter_init (&iter, linestring);

  while ((len = shumate_vector_point_iter_next_segment (&iter)))
    sum += len;

  return sum;
}


void
shumate_vector_line_string_bounds (ShumateVectorLineString *linestring,
                                   ShumateVectorPoint      *radius_out,
                                   ShumateVectorPoint      *center_out)
{
  guint i;
  float min_x, max_x, min_y, max_y;

  g_return_if_fail (linestring->n_points > 0);

  min_x = max_x = linestring->points[0].x;
  min_y = max_y = linestring->points[0].y;

  for (i = 1; i < linestring->n_points; i ++)
    {
      min_x = MIN (min_x, linestring->points[i].x);
      max_x = MAX (max_x, linestring->points[i].x);
      min_y = MIN (min_y, linestring->points[i].y);
      max_y = MAX (max_y, linestring->points[i].y);
    }

  radius_out->x = (max_x - min_x) / 2.0;
  radius_out->y = (max_y - min_y) / 2.0;
  center_out->x = (max_x + min_x) / 2.0;
  center_out->y = (max_y + min_y) / 2.0;
}


GPtrArray *
shumate_vector_line_string_simplify (ShumateVectorLineString *linestring)
{
  gsize i;
  GPtrArray *result = g_ptr_array_new ();

  g_ptr_array_add (result, linestring);

  if (linestring->n_points <= 2)
    return result;

  /* The glyph layout algorithm for line symbols does not handle high detail
   * very well. Lots of short segments with different angles cause it to place
   * glyphs too close together and with "random" rotations, which makes text
   * illegible.
   *
   * I tried several solutions. Simplification (such as the Visvalingam-Whyatt
   * algorithm) creates too many sharp angles, which produces poor results.
   * I also tried a smoothing algorithm which averages each point with its
   * neighbors, and while that produced good results with natural lines like
   * rivers, it deformed street labels that already looked fine, causing them
   * not to line up with the street anymore.
   *
   * The following algorithm reduces detail only where it exists. It works by
   * repeatedly merging the closest pair of neighboring points until no two
   * points in the line are closer than a threshold.
   * */

  while (TRUE)
    {
      gint min_idx = -1;

      /* Square the threshold because we compare it to the square of the
       * distance (saving a sqrt() call). The unit is the size of a tile. */
      float min_distance = 0.025 * 0.025;

      /* Find the closest pair of points, excepting the first and last pair
       * because we don't want to change the endpoints */
      for (i = 1; i < linestring->n_points - 2; i ++)
        {
          float distance = point_distance_sq (&linestring->points[i], &linestring->points[i + 1]);
          if (distance < min_distance)
            {
              min_idx = i;
              min_distance = distance;
            }
        }

      if (min_idx == -1 || linestring->n_points <= 2)
        break;

      /* Set the first point in the pair to the average of the two */
      linestring->points[min_idx] = (ShumateVectorPoint) {
        (linestring->points[min_idx].x + linestring->points[min_idx + 1].x) / 2.0,
        (linestring->points[min_idx].y + linestring->points[min_idx + 1].y) / 2.0,
      };

      /* Remove the second point by shifting everything after it */
      linestring->n_points --;
      for (i = min_idx + 1; i < linestring->n_points; i ++)
        linestring->points[i] = linestring->points[i + 1];
    }

  /* Line labels also don't look good if there's sharp angles. To fix that, we
   * split the line wherever one occurs. */

  for (i = linestring->n_points - 2; i >= 1; i --)
    {
      /* Angle between three points. See https://math.stackexchange.com/a/3427603 */

      /* Dot product of points[i] to points[i - 1], and points[i] to points[i + 1] */
      float dot = (linestring->points[i].x - linestring->points[i + 1].x)
                    * (linestring->points[i].x - linestring->points[i - 1].x)
                  + ((linestring->points[i].y - linestring->points[i + 1].y)
                    * (linestring->points[i].y - linestring->points[i - 1].y));

      float len = sqrt (point_distance_sq (&linestring->points[i], &linestring->points[i + 1])
                        * point_distance_sq (&linestring->points[i], &linestring->points[i - 1]));

      float angle = fabs (acos (dot / len));

      if (angle < 120 * G_PI / 180)
        {
          ShumateVectorLineString *new_line = g_new (ShumateVectorLineString, 1);

          /* Copy from the current point until the end of the line */
          new_line->n_points = linestring->n_points - i;
          new_line->points = g_memdup2 (&linestring->points[i], new_line->n_points * sizeof (ShumateVectorPoint));

          linestring->n_points = i + 1;

          g_ptr_array_add (result, new_line);
        }
    }

  return result;
}

static int
zigzag (guint value)
{
  return (value >> 1) ^ (-(value & 1));
}

gboolean
shumate_vector_geometry_iter (ShumateVectorGeometryIter *iter)
{
  int cmd;

  if (iter->j >= iter->repeat)
    {
      iter->j = 0;

      if (iter->i >= iter->feature->n_geometry)
        return FALSE;

      cmd = iter->feature->geometry[iter->i];
      iter->i ++;

      iter->op = cmd & 0x7;
      iter->repeat = cmd >> 3;
    }

  switch (iter->op)
    {
    case SHUMATE_VECTOR_GEOMETRY_OP_MOVE_TO:
    case SHUMATE_VECTOR_GEOMETRY_OP_LINE_TO:
      if (iter->i + 1 >= iter->feature->n_geometry)
        return FALSE;

      iter->dx = zigzag (iter->feature->geometry[iter->i]);
      iter->dy = zigzag (iter->feature->geometry[iter->i + 1]);
      iter->cursor_x += iter->dx;
      iter->cursor_y += iter->dy;
      iter->x = iter->cursor_x;
      iter->y = iter->cursor_y;

      if (iter->op == SHUMATE_VECTOR_GEOMETRY_OP_MOVE_TO)
        {
          iter->start_x = iter->x;
          iter->start_y = iter->y;
        }

      iter->i += 2;
      break;

    case SHUMATE_VECTOR_GEOMETRY_OP_CLOSE_PATH:
      iter->dx = iter->start_x - iter->x;
      iter->dy = iter->start_y - iter->y;
      iter->x = iter->start_x;
      iter->y = iter->start_y;
      break;
    }

  iter->j ++;
  return TRUE;
}
