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
  };
}


double
shumate_vector_point_iter_next_segment (ShumateVectorPointIter *iter)
{
  double res = shumate_vector_point_iter_get_segment_length (iter) - iter->distance;
  iter->distance = 0;
  iter->current_point ++;
  return res;
}


static double
point_distance (ShumateVectorPoint *a, ShumateVectorPoint *b)
{
  double x = a->x - b->x;
  double y = a->y - b->y;
  return sqrt (x*x + y*y);
}

static ShumateVectorPoint *
get_prev_point (ShumateVectorPointIter *iter)
{
  return &iter->points[iter->current_point];
}

static ShumateVectorPoint *
get_next_point (ShumateVectorPointIter *iter)
{
  if (iter->current_point >= iter->num_points - 1)
    return &iter->points[iter->num_points - 1];
  else
    return &iter->points[iter->current_point + 1];
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
  while (distance > 0)
    {
      if (iter->current_point >= iter->num_points - 1)
        return;

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
  double orig = shumate_vector_point_iter_get_current_angle (iter);
  double orig_distance = remaining_distance;
  double sum = 0.0;

  while (remaining_distance > 0)
    {
      float angle = shumate_vector_point_iter_get_current_angle (&iter2);
      float d = MIN (remaining_distance, shumate_vector_point_iter_next_segment (&iter2));
      sum += d * (angle - orig);
      remaining_distance -= d;
    }

  return sum / orig_distance;
}


void
shumate_vector_line_string_clear (ShumateVectorLineString *linestring)
{
  g_clear_pointer (&linestring->points, g_free);
  linestring->n_points = 0;
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
