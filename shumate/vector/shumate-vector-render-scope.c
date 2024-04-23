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

#include "shumate-vector-render-scope-private.h"

enum {
  MOVE_TO = 1,
  LINE_TO = 2,
  CLOSE_PATH = 7,
};

static int
zigzag (guint value)
{
  return (value >> 1) ^ (-(value & 1));
}

static void
apply_transforms (ShumateVectorRenderScope *self,
                  float                     extent,
                  float                    *x,
                  float                    *y)
{
  *x = ((*x / extent) - self->overzoom_x) * self->overzoom_scale;
  *y = ((*y / extent) - self->overzoom_y) * self->overzoom_scale;
}

/* Draws the current feature as a path onto the scope's cairo context. */
void
shumate_vector_render_scope_exec_geometry (ShumateVectorRenderScope *self)
{
  VectorTile__Tile__Feature *feature = shumate_vector_reader_iter_get_feature_struct (self->reader);

  g_return_if_fail (feature != NULL);

  cairo_new_path (self->cr);
  cairo_move_to (self->cr, 0, 0);

  for (int i = 0; i < feature->n_geometry; i ++)
    {
      int cmd = feature->geometry[i];
      double dx, dy;

      /* See https://github.com/mapbox/vector-tile-spec/tree/master/2.1#43-geometry-encoding */
      int op = cmd & 0x7;
      int repeat = cmd >> 3;

      for (int j = 0; j < repeat; j ++)
        {
          switch (op) {
          case MOVE_TO:
            g_return_if_fail (i + 2 < feature->n_geometry);
            dx = zigzag (feature->geometry[++i]);
            dy = zigzag (feature->geometry[++i]);
            cairo_rel_move_to (self->cr, dx, dy);
            break;
          case LINE_TO:
            g_return_if_fail (i + 2 < feature->n_geometry);
            dx = zigzag (feature->geometry[++i]);
            dy = zigzag (feature->geometry[++i]);
            cairo_rel_line_to (self->cr, dx, dy);
            break;
          case CLOSE_PATH:
            cairo_get_current_point (self->cr, &dx, &dy);
            cairo_close_path (self->cr);
            cairo_move_to (self->cr, dx, dy);
            break;
          default:
            g_assert_not_reached ();
          }
        }
    }
}


GPtrArray *
shumate_vector_render_scope_get_geometry (ShumateVectorRenderScope *self)
{
  GPtrArray *lines = g_ptr_array_new_with_free_func ((GDestroyNotify)shumate_vector_line_string_free);
  ShumateVectorLineString *current_line = NULL;
  VectorTile__Tile__Feature *feature = shumate_vector_reader_iter_get_feature_struct (self->reader);
  VectorTile__Tile__Layer *layer = shumate_vector_reader_iter_get_layer_struct (self->reader);
  float x = 0, y = 0;
  float x_tf, y_tf;

  g_return_val_if_fail (feature != NULL, NULL);

  for (int i = 0; i < feature->n_geometry; i ++)
    {
      int cmd = feature->geometry[i];
      double start_x = 0, start_y = 0;

      int op = cmd & 0x7;
      int repeat = cmd >> 3;

      if (current_line != NULL)
        current_line->points = g_realloc (current_line->points, (repeat + current_line->n_points) * sizeof (ShumateVectorPoint));

      for (int j = 0; j < repeat; j ++)
        {
          switch (op) {
          case MOVE_TO:
            g_return_val_if_fail (i + 2 < feature->n_geometry, NULL);

            if (current_line != NULL)
              g_ptr_array_add (lines, current_line);

            current_line = g_new (ShumateVectorLineString, 1);
            current_line->points = g_new (ShumateVectorPoint, 1);
            current_line->n_points = 1;

            x += zigzag (feature->geometry[++i]);
            y += zigzag (feature->geometry[++i]);

            x_tf = x;
            y_tf = y;
            apply_transforms (self, layer->extent, &x_tf, &y_tf);

            start_x = x_tf;
            start_y = y_tf;

            current_line->points[0] = (ShumateVectorPoint) {
              .x = x_tf,
              .y = y_tf,
            };
            break;
          case LINE_TO:
            g_return_val_if_fail (i + 2 < feature->n_geometry, NULL);
            g_return_val_if_fail (current_line != NULL, NULL);

            x += zigzag (feature->geometry[++i]);
            y += zigzag (feature->geometry[++i]);

            x_tf = x;
            y_tf = y;
            apply_transforms (self, layer->extent, &x_tf, &y_tf);

            current_line->points[current_line->n_points++] = (ShumateVectorPoint) {
              .x = x_tf,
              .y = y_tf,
            };
            break;
          case CLOSE_PATH:
            g_return_val_if_fail (current_line != NULL, NULL);

            current_line->points[current_line->n_points++] = (ShumateVectorPoint) {
              .x = start_x,
              .y = start_y,
            };
            break;
          default:
            g_assert_not_reached ();
          }
        }
    }

  if (current_line != NULL)
    g_ptr_array_add (lines, current_line);

  return lines;
}


void
shumate_vector_render_scope_get_bounds (ShumateVectorRenderScope *self,
                                        float                    *min_x,
                                        float                    *min_y,
                                        float                    *max_x,
                                        float                    *max_y)
{
  VectorTile__Tile__Layer *layer = shumate_vector_reader_iter_get_layer_struct (self->reader);
  VectorTile__Tile__Feature *feature = shumate_vector_reader_iter_get_feature_struct (self->reader);
  double x = 0, y = 0;

  *min_x = G_MAXFLOAT;
  *min_y = G_MAXFLOAT;
  *max_x = G_MINFLOAT;
  *max_y = G_MINFLOAT;

  g_return_if_fail (feature != NULL);

  for (int i = 0; i < feature->n_geometry; i ++)
    {
      int cmd = feature->geometry[i];

      /* See https://github.com/mapbox/vector-tile-spec/tree/master/2.1#43-geometry-encoding */
      int op = cmd & 0x7;
      int repeat = cmd >> 3;

      for (int j = 0; j < repeat; j ++)
        {
          switch (op) {
          case 1:
            g_return_if_fail (i + 2 < feature->n_geometry);
            x += zigzag (feature->geometry[++i]);
            y += zigzag (feature->geometry[++i]);
            break;
          case 2:
            g_return_if_fail (i + 2 < feature->n_geometry);
            x += zigzag (feature->geometry[++i]);
            y += zigzag (feature->geometry[++i]);
            break;
          case 7:
            break;
          default:
            g_assert_not_reached ();
          }

          *min_x = MIN (*min_x, x);
          *min_y = MIN (*min_y, y);
          *max_x = MAX (*max_x, x);
          *max_y = MAX (*max_y, y);
        }
    }

  apply_transforms (self, layer->extent, min_x, min_y);
  apply_transforms (self, layer->extent, max_x, max_y);
}


ShumateVectorGeometryType
shumate_vector_render_scope_get_geometry_type (ShumateVectorRenderScope *self)
{
  VectorTile__Tile__Feature *feature = shumate_vector_reader_iter_get_feature_struct (self->reader);
  g_return_val_if_fail (feature != NULL, 0);
  return (ShumateVectorGeometryType) feature->type;
}

void
shumate_vector_render_scope_get_geometry_center (ShumateVectorRenderScope *self,
                                                 double                   *x,
                                                 double                   *y)
{
  float min_x, min_y, max_x, max_y;
  shumate_vector_render_scope_get_bounds (self, &min_x, &min_y, &max_x, &max_y);
  *x = (min_x + max_x) / 2.0;
  *y = (min_y + max_y) / 2.0;
}

static void
convert_vector_value (VectorTile__Tile__Value *v, ShumateVectorValue *value)
{
  if (v->has_int_value)
    shumate_vector_value_set_number (value, v->int_value);
  else if (v->has_uint_value)
    shumate_vector_value_set_number (value, v->uint_value);
  else if (v->has_sint_value)
    shumate_vector_value_set_number (value, v->sint_value);
  else if (v->has_float_value)
    shumate_vector_value_set_number (value, v->float_value);
  else if (v->has_double_value)
    shumate_vector_value_set_number (value, v->double_value);
  else if (v->has_bool_value)
    shumate_vector_value_set_boolean (value, v->bool_value);
  else if (v->string_value != NULL)
    shumate_vector_value_set_string (value, v->string_value);
  else
    shumate_vector_value_unset (value);
}

void
shumate_vector_render_scope_get_variable (ShumateVectorRenderScope *self, const char *variable, ShumateVectorValue *value)
{
  VectorTile__Tile__Layer *layer = shumate_vector_reader_iter_get_layer_struct (self->reader);
  VectorTile__Tile__Feature *feature = shumate_vector_reader_iter_get_feature_struct (self->reader);

  for (int i = 0; i + 1 < feature->n_tags; i += 2)
    {
      if (strcmp (layer->keys[feature->tags[i]], variable) == 0)
        {
          convert_vector_value (layer->values[feature->tags[i + 1]], value);
          return;
        }
    }

  shumate_vector_value_unset (value);
}


GHashTable *
shumate_vector_render_scope_create_tag_table (ShumateVectorRenderScope *self)
{
  g_autoptr(GHashTable) tags = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
  g_auto(ShumateVectorValue) value = SHUMATE_VECTOR_VALUE_INIT;
  VectorTile__Tile__Layer *layer = shumate_vector_reader_iter_get_layer_struct (self->reader);
  VectorTile__Tile__Feature *feature = shumate_vector_reader_iter_get_feature_struct (self->reader);

  for (int i = 1; i < feature->n_tags; i += 2)
    {
      g_auto(ShumateVectorValue) value = SHUMATE_VECTOR_VALUE_INIT;
      int key = feature->tags[i - 1];
      int val = feature->tags[i];

      if (key >= layer->n_keys || val >= layer->n_values)
        continue;

      convert_vector_value (layer->values[val], &value);
      g_hash_table_insert (tags, g_strdup (layer->keys[key]), shumate_vector_value_as_string (&value));
    }

  return g_steal_pointer (&tags);
}

ShumateVectorIndexBitset *
shumate_vector_render_scope_get_bitset (ShumateVectorRenderScope *self,
                                        const char               *field,
                                        ShumateVectorValue       *value)
{
  return shumate_vector_index_get_bitset (self->index, self->layer_idx, field, value);
}

/* Temporary data for shumate_vector_render_scope_index_layer */
typedef struct {
  ShumateVectorIndexBitset *used_values;
  ShumateVectorIndexBitset *unused_values;
  GHashTable *indexes;
  ShumateVectorIndexBitset *has_index;
  guint8 is_unused;
} FieldIndexingData;

void
shumate_vector_render_scope_index_layer (ShumateVectorRenderScope *self)
{
  const char *layer_name = shumate_vector_reader_iter_get_layer_name (self->reader);
  VectorTile__Tile__Layer *layer;
  FieldIndexingData *fields;
  int feature_idx = 0;
  ShumateVectorIndexBitset *broad_geometry_indexes[3] = { NULL, NULL, NULL };
  ShumateVectorIndexBitset *geometry_indexes[7] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL };

  if (self->index == NULL)
    self->index = shumate_vector_index_new ();

  if (shumate_vector_index_has_layer (self->index, self->source_layer_idx))
    return;

  if (!shumate_vector_index_description_has_layer (self->index_description, layer_name))
    return;

  layer = shumate_vector_reader_iter_get_layer_struct (self->reader);
  fields = g_new0 (FieldIndexingData, layer->n_keys);

  if (shumate_vector_index_description_has_broad_geometry_type (self->index_description, layer_name))
    {
      for (int i = 0; i < 3; i ++)
        broad_geometry_indexes[i] = shumate_vector_index_bitset_new (layer->n_features);

      shumate_vector_index_add_bitset_broad_geometry_type (self->index, self->source_layer_idx, SHUMATE_GEOMETRY_TYPE_POINT, broad_geometry_indexes[0]);
      shumate_vector_index_add_bitset_broad_geometry_type (self->index, self->source_layer_idx, SHUMATE_GEOMETRY_TYPE_LINESTRING, broad_geometry_indexes[1]);
      shumate_vector_index_add_bitset_broad_geometry_type (self->index, self->source_layer_idx, SHUMATE_GEOMETRY_TYPE_POLYGON, broad_geometry_indexes[2]);
    }

  if (shumate_vector_index_description_has_geometry_type (self->index_description, layer_name))
    {
      for (int i = 1; i < 7; i ++)
        {
          geometry_indexes[i] = shumate_vector_index_bitset_new (layer->n_features);
          shumate_vector_index_add_bitset_geometry_type (self->index, self->source_layer_idx, i, geometry_indexes[i]);
        }
    }

  /* Read all of the features and build every requested index */
  shumate_vector_reader_iter_read_feature (self->reader, 0);
  while (TRUE)
    {
      VectorTile__Tile__Feature *feature = shumate_vector_reader_iter_get_feature_struct (self->reader);

      if (broad_geometry_indexes[0] != NULL)
        {
          switch (feature->type)
            {
            case VECTOR_TILE__TILE__GEOM_TYPE__POINT:
              shumate_vector_index_bitset_set (broad_geometry_indexes[0], feature_idx);
              break;
            case VECTOR_TILE__TILE__GEOM_TYPE__LINESTRING:
              shumate_vector_index_bitset_set (broad_geometry_indexes[1], feature_idx);
              break;
            case VECTOR_TILE__TILE__GEOM_TYPE__POLYGON:
              shumate_vector_index_bitset_set (broad_geometry_indexes[2], feature_idx);
              break;
            default:
              break;
            }
        }

      if (geometry_indexes[feature->type] != NULL)
        {
          ShumateGeometryType type = shumate_vector_reader_iter_get_feature_geometry_type (self->reader);
          shumate_vector_index_bitset_set (geometry_indexes[type], feature_idx);
        }

      for (int i = 1; i < feature->n_tags; i += 2)
        {
          ShumateVectorIndexBitset *bitset;
          int key = feature->tags[i - 1];
          int val = feature->tags[i];

          if (key >= layer->n_keys || val >= layer->n_values)
            continue;

          if (fields[key].indexes == NULL)
            {
              if (fields[key].is_unused)
                /* We have already looked up this field, and it is not in the description */
                continue;

              if (shumate_vector_index_description_has_field (self->index_description, layer_name, layer->keys[key]))
                {
                  fields[key].indexes = g_hash_table_new (g_direct_hash, g_direct_equal);
                  fields[key].used_values = shumate_vector_index_bitset_new (layer->n_values);
                  fields[key].unused_values = shumate_vector_index_bitset_new (layer->n_values);
                  if (shumate_vector_index_description_has_field_has_index (self->index_description, layer_name, layer->keys[key]))
                    fields[key].has_index = shumate_vector_index_bitset_new (layer->n_features);
                }
              else
                {
                  fields[key].is_unused = TRUE;
                  continue;
                }
            }

          if (fields[key].has_index != NULL)
            shumate_vector_index_bitset_set (fields[key].has_index, feature_idx);

          if (shumate_vector_index_bitset_get (fields[key].unused_values, val))
            continue;

          if (shumate_vector_index_bitset_get (fields[key].used_values, val))
            bitset = g_hash_table_lookup (fields[key].indexes, GINT_TO_POINTER (val));
          else
            {
              g_auto(ShumateVectorValue) value = SHUMATE_VECTOR_VALUE_INIT;
              VectorTile__Tile__Value *v = layer->values[val];
              const char *field_name = layer->keys[key];

              convert_vector_value (v, &value);

              if (shumate_vector_index_description_has_value (self->index_description, layer_name, field_name, &value))
                bitset = shumate_vector_index_bitset_new (layer->n_features);
              else
                {
                  shumate_vector_index_bitset_set (fields[key].unused_values, val);
                  continue;
                }

              g_hash_table_insert (fields[key].indexes, GINT_TO_POINTER (val), bitset);
              shumate_vector_index_bitset_set (fields[key].used_values, val);
            }

          shumate_vector_index_bitset_set (bitset, feature_idx);
        }

      if (!shumate_vector_reader_iter_next_feature (self->reader))
        break;
      feature_idx ++;
    }

  for (int key = 0; key < layer->n_keys; key ++)
    {
      gpointer valp;
      const char *field_name;
      GHashTableIter field_iter;
      ShumateVectorIndexBitset *bitset;
      FieldIndexingData *field = &fields[key];

      if (field->indexes == NULL)
        continue;

      field_name = layer->keys[key];

      g_hash_table_iter_init (&field_iter, field->indexes);
      while (g_hash_table_iter_next (&field_iter, &valp, (gpointer *)&bitset))
        {
          int val = GPOINTER_TO_INT(valp);
          g_auto(ShumateVectorValue) value = SHUMATE_VECTOR_VALUE_INIT;
          VectorTile__Tile__Value *v;

          v = layer->values[val];
          convert_vector_value (v, &value);

          shumate_vector_index_add_bitset (self->index, self->source_layer_idx, field_name, &value, bitset);
        }

      g_hash_table_destroy (field->indexes);
      shumate_vector_index_bitset_free (field->used_values);
      shumate_vector_index_bitset_free (field->unused_values);

      if (field->has_index != NULL)
        shumate_vector_index_add_bitset_has (self->index, self->source_layer_idx, field_name, field->has_index);
    }

  g_free (fields);
}
