/*
 * Copyright (C) 2023 James Westman <james@jwestman.net>
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

#include "shumate-vector-reader-private.h"
#include "shumate-vector-reader-iter-private.h"

#ifdef SHUMATE_HAS_VECTOR_RENDERER
#include "vector/shumate-vector-utils-private.h"
#endif

/**
 * ShumateVectorReaderIter:
 *
 * Reads the layers and features of a vector tile.
 *
 * To create a new [class@VectorReaderIter], use [method@VectorReader.iterate].
 *
 * A vector tile consists of named layers, which contain features. Each feature
 * has an ID, a geometry, and a set of key/value tags. The meanings of
 * the IDs and tags depends on the data source that the tile came from. The
 * [OpenMapTiles schema](https://openmaptiles.org/schema/) is a common schema
 * for vector tiles.
 *
 * To read all layers in a tile, use [method@VectorReaderIter.get_layer_count] and
 * [method@VectorReaderIter.read_layer]. If you know the name of the layer you
 * want, you can also use [method@VectorReaderIter.read_layer_by_name].
 * Once the iterator is reading a layer, you can call
 * [method@VectorReaderIter.next_feature] in a loop to read all the features in
 * the layer.
 *
 * A [class@VectorReaderIter] is not thread-safe, but iterators created
 * from the same [class@VectorReader] can be used in different threads.
 *
 * See [the Mapbox Vector Tile specification](https://github.com/mapbox/vector-tile-spec/tree/master/2.1)
 * for more information about the vector tile format.
 *
 * Since: 1.2
 */

struct _ShumateVectorReaderIter
{
  GObject parent_instance;

  ShumateVectorReader *reader;

#ifdef SHUMATE_HAS_VECTOR_RENDERER
  VectorTile__Tile__Layer *layer;
  VectorTile__Tile__Feature *feature;
  int feature_index;
#endif
};

enum {
  PROP_0,
  PROP_READER,
  N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES];

G_DEFINE_TYPE (ShumateVectorReaderIter, shumate_vector_reader_iter, G_TYPE_OBJECT)

static void
shumate_vector_reader_iter_get_property (GObject    *object,
                                         guint       prop_id,
                                         GValue     *value,
                                         GParamSpec *pspec)
{
  ShumateVectorReaderIter *self = SHUMATE_VECTOR_READER_ITER (object);

  switch (prop_id)
    {
    case PROP_READER:
      g_value_set_object (value, self->reader);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
shumate_vector_reader_iter_set_property (GObject      *object,
                                         guint         prop_id,
                                         const GValue *value,
                                         GParamSpec   *pspec)
{
  ShumateVectorReaderIter *self = SHUMATE_VECTOR_READER_ITER (object);

  switch (prop_id)
    {
    case PROP_READER:
      self->reader = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
shumate_vector_reader_iter_finalize (GObject *object)
{
  ShumateVectorReaderIter *self = SHUMATE_VECTOR_READER_ITER (object);

  g_clear_object (&self->reader);

  G_OBJECT_CLASS (shumate_vector_reader_iter_parent_class)->finalize (object);
}

static void
shumate_vector_reader_iter_class_init (ShumateVectorReaderIterClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = shumate_vector_reader_iter_finalize;
  object_class->get_property = shumate_vector_reader_iter_get_property;
  object_class->set_property = shumate_vector_reader_iter_set_property;

  /**
   * VectorReaderIter:reader:
   *
   * The [class@VectorReader] that the iterator is iterating over.
   *
   * Since: 1.2
   */
  properties[PROP_READER] =
    g_param_spec_object ("reader",
                         "reader",
                         "reader",
                         SHUMATE_TYPE_VECTOR_READER,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPERTIES, properties);
}

static void
shumate_vector_reader_iter_init (ShumateVectorReaderIter *self)
{
}

/**
 * shumate_vector_reader_iter_get_reader:
 * @self: A [class@VectorReaderIter]
 *
 * Gets the reader that the iterator is iterating over.
 *
 * Returns: (transfer none): The reader that the iterator is iterating over.
 *
 * Since: 1.2
 */
ShumateVectorReader *
shumate_vector_reader_iter_get_reader (ShumateVectorReaderIter *self)
{
  g_return_val_if_fail (SHUMATE_IS_VECTOR_READER_ITER (self), NULL);
  return self->reader;
}

/**
 * shumate_vector_reader_iter_get_layer_count:
 * @self: A #ShumateVectorReader.
 *
 * Gets the number of layers in the vector tile.
 *
 * Returns: The number of layers.
 *
 * Since: 1.2
 */
guint
shumate_vector_reader_iter_get_layer_count (ShumateVectorReaderIter *self)
{
  g_return_val_if_fail (SHUMATE_IS_VECTOR_READER_ITER (self), 0);

#ifdef SHUMATE_HAS_VECTOR_RENDERER
  return self->reader->tile->n_layers;
#else
  return 0;
#endif
}

/**
 * shumate_vector_reader_iter_read_layer:
 * @self: A #ShumateVectorReader.
 * @index: The index of the layer to read.
 *
 * Sets the current layer of the reader to the layer at the given index.
 *
 * Since: 1.2
 */
void
shumate_vector_reader_iter_read_layer (ShumateVectorReaderIter *self, int index)
{
  g_return_if_fail (SHUMATE_IS_VECTOR_READER_ITER (self));

#ifdef SHUMATE_HAS_VECTOR_RENDERER
  g_return_if_fail (index >= 0);
  g_return_if_fail (index < self->reader->tile->n_layers);

  self->layer = self->reader->tile->layers[index];
  self->feature = NULL;
#endif
}

/**
 * shumate_vector_reader_iter_read_layer_by_name:
 * @self: A #ShumateVectorReader.
 *
 * Moves the iterator to the layer with the given name, if present.
 *
 * If the layer is not found, the current layer will be set to %NULL and the
 * function will return %FALSE. Layers are typically omitted if they are empty,
 * so don't assume that a layer in the schema will always be present.
 *
 * The iterator's current feature will be %NULL after calling this function;
 * use [method@VectorReaderIter.next_feature] to advance to the first feature
 * in the layer.
 *
 * Returns: %TRUE if the layer was found, %FALSE otherwise.
 *
 * Since: 1.2
 */
gboolean
shumate_vector_reader_iter_read_layer_by_name (ShumateVectorReaderIter *self,
                                               const char              *name)
{
  g_return_val_if_fail (SHUMATE_IS_VECTOR_READER_ITER (self), FALSE);

#ifdef SHUMATE_HAS_VECTOR_RENDERER
  self->layer = NULL;
  self->feature = NULL;

  for (int i = 0; i < self->reader->tile->n_layers; i++)
    {
      if (strcmp (self->reader->tile->layers[i]->name, name) == 0)
        {
          self->layer = self->reader->tile->layers[i];
          self->feature = NULL;
          return TRUE;
        }
    }
#endif

  return FALSE;
}

/*< private >
 * shumate_vector_reader_iter_get_layer_index:
 * @self: A #ShumateVectorReaderIter.
 *
 * Gets the index of the current layer.
 *
 * Returns: The index of the current layer.
 *
 * Since: 1.3
 */
int
shumate_vector_reader_iter_get_layer_index (ShumateVectorReaderIter *self)
{
  g_return_val_if_fail (SHUMATE_IS_VECTOR_READER_ITER (self), -1);

#ifdef SHUMATE_HAS_VECTOR_RENDERER
  for (int i = 0; i < self->reader->tile->n_layers; i++)
  {
    if (self->layer == self->reader->tile->layers[i])
    {
      return i;
    }
  }
#endif

  return -1;
}

/**
 * shumate_vector_reader_iter_get_layer_name:
 * @self: A #ShumateVectorReader.
 *
 * Gets the name of the current layer.
 *
 * Returns: (transfer none): The name of the current layer.
 *
 * Since: 1.2
 */
const char *
shumate_vector_reader_iter_get_layer_name (ShumateVectorReaderIter *self)
{
  g_return_val_if_fail (SHUMATE_IS_VECTOR_READER_ITER (self), NULL);

#ifdef SHUMATE_HAS_VECTOR_RENDERER
  g_return_val_if_fail (self->layer != NULL, NULL);
  return self->layer->name;
#else
  return NULL;
#endif
}

/**
 * shumate_vector_reader_iter_get_layer_feature_count:
 * @self: A #ShumateVectorReader.
 *
 * Gets the number of features in the current layer.
 *
 * You can loop over all features in the current layer by calling
 * [method@VectorReaderIter.read_feature] with each index from 0 to
 * the feature count, but it might be easier to use
 * [method@VectorReaderIter.next_feature] instead.
 *
 * Returns: The number of features in the current layer.
 *
 * Since: 1.2
 */
guint
shumate_vector_reader_iter_get_layer_feature_count (ShumateVectorReaderIter *self)
{
  g_return_val_if_fail (SHUMATE_IS_VECTOR_READER_ITER (self), 0);

#ifdef SHUMATE_HAS_VECTOR_RENDERER
  g_return_val_if_fail (self->layer != NULL, 0);
  return self->layer->n_features;
#else
  return 0;
#endif
}

/**
 * shumate_vector_reader_iter_get_layer_extent:
 * @self: A [class@VectorReaderIter].
 *
 * Gets the extent for coordinates in the current layer.
 *
 * 0 represents the top and left edges of the tile, and this value
 * represents the bottom and right edges. Feature geometries may extend
 * outside of this range, since tiles often include some margin.
 *
 * Tiles do not contain metadata about the location of the tile within
 * the world, so it is up to the caller to know the tile's coordinates
 * and convert latitude/longitude to tile-space coordinates.
 *
 * Returns: The layer's extent
 *
 * Since: 1.2
 */
guint
shumate_vector_reader_iter_get_layer_extent (ShumateVectorReaderIter *self)
{
  g_return_val_if_fail (SHUMATE_IS_VECTOR_READER_ITER (self), 0);

#ifdef SHUMATE_HAS_VECTOR_RENDERER
  g_return_val_if_fail (self->layer != NULL, 0);
  return self->layer->extent;
#else
  return 0;
#endif
}

/**
 * shumate_vector_reader_iter_read_feature:
 * @self: A #ShumateVectorReader.
 * @index: The index of the feature to read.
 *
 * Moves the iterator to the feature at the given index in the current layer.
 *
 * You can get the number of features in the current layer with
 * [method@VectorReaderIter.get_layer_feature_count].
 *
 * Since: 1.2
 */
void
shumate_vector_reader_iter_read_feature (ShumateVectorReaderIter *self,
                                         int                      index)
{
  g_return_if_fail (SHUMATE_IS_VECTOR_READER_ITER (self));

#ifdef SHUMATE_HAS_VECTOR_RENDERER
  g_return_if_fail (index >= 0);
  g_return_if_fail (self->layer != NULL);
  g_return_if_fail (index < self->layer->n_features);

  self->feature = self->layer->features[index];
  self->feature_index = index;
#endif
}

/**
 * shumate_vector_reader_iter_next_feature:
 * @self: A #ShumateVectorReader.
 *
 * Advances the iterator to the next feature in the current layer.
 *
 * Returns: %TRUE if there is a next feature, %FALSE otherwise.
 *
 * Since: 1.2
 */
gboolean
shumate_vector_reader_iter_next_feature (ShumateVectorReaderIter *self)
{
  int next_index = 0;

  g_return_val_if_fail (SHUMATE_IS_VECTOR_READER_ITER (self), FALSE);

#ifdef SHUMATE_HAS_VECTOR_RENDERER
  g_return_val_if_fail (self->layer != NULL, FALSE);

  if (self->feature == NULL)
    next_index = 0;
  else
    next_index = self->feature_index + 1;

  if (next_index < self->layer->n_features)
    {
      self->feature = self->layer->features[next_index];
      self->feature_index = next_index;
      return TRUE;
    }
  else
    return FALSE;
#else
  return FALSE;
#endif
}

/*< private >
 * shumate_vector_reader_iter_get_feature_index:
 * @self: A [class@VectorReaderIter].
 *
 * Gets the index of the current feature.
 *
 * Returns: The index of the current feature.
 *
 * Since: 1.3
 */
int
shumate_vector_reader_iter_get_feature_index (ShumateVectorReaderIter *self)
{
  g_return_val_if_fail (SHUMATE_IS_VECTOR_READER_ITER (self), 0);

#ifdef SHUMATE_HAS_VECTOR_RENDERER
  return self->feature_index;
#else
  return 0;
#endif
}

/**
 * shumate_vector_reader_iter_get_feature_id:
 * @self: A [class@VectorReaderIter].
 *
 * Gets the ID of the current feature.
 *
 * Returns: The ID of the current feature.
 *
 * Since: 1.2
 */
guint64
shumate_vector_reader_iter_get_feature_id (ShumateVectorReaderIter *self)
{
  g_return_val_if_fail (SHUMATE_IS_VECTOR_READER_ITER (self), 0);

#ifdef SHUMATE_HAS_VECTOR_RENDERER
  g_return_val_if_fail (self->feature != NULL, 0);
  return self->feature->id;
#else
  return 0;
#endif
}

/**
 * shumate_vector_reader_iter_get_feature_tag:
 * @self: A [class@VectorReaderIter].
 * @key: The key of the tag to get.
 * @value: (out): The value of the tag.
 *
 * Gets the value of the tag with the given key.
 *
 * Returns: %TRUE if the tag was found, %FALSE otherwise.
 *
 * Since: 1.2
 */
gboolean
shumate_vector_reader_iter_get_feature_tag (ShumateVectorReaderIter *self,
                                            const char              *key,
                                            GValue                  *value)
{
  g_return_val_if_fail (SHUMATE_IS_VECTOR_READER_ITER (self), FALSE);

#ifdef SHUMATE_HAS_VECTOR_RENDERER
  g_return_val_if_fail (self->feature != NULL, FALSE);

  if (key == NULL)
    return FALSE;

  for (int i = 0; i + 1 < self->feature->n_tags; i += 2)
    {
      if (strcmp (self->layer->keys[self->feature->tags[i]], key) == 0)
        {
          VectorTile__Tile__Value *v = self->layer->values[self->feature->tags[i + 1]];

          if (v->has_int_value)
            {
              g_value_init (value, G_TYPE_INT64);
              g_value_set_int64 (value, v->int_value);
            }
          else if (v->has_uint_value)
            {
              g_value_init (value, G_TYPE_UINT64);
              g_value_set_uint64 (value, v->uint_value);
            }
          else if (v->has_sint_value)
            {
              g_value_init (value, G_TYPE_INT64);
              g_value_set_int64 (value, v->sint_value);
            }
          else if (v->has_float_value)
            {
              g_value_init (value, G_TYPE_FLOAT);
              g_value_set_float (value, v->float_value);
            }
          else if (v->has_double_value)
            {
              g_value_init (value, G_TYPE_DOUBLE);
              g_value_set_double (value, v->double_value);
            }
          else if (v->has_bool_value)
            {
              g_value_init (value, G_TYPE_BOOLEAN);
              g_value_set_boolean (value, v->bool_value);
            }
          else if (v->string_value != NULL)
            {
              g_value_init (value, G_TYPE_STRING);
              g_value_set_string (value, v->string_value);
            }
          else
            g_value_unset (value);

          return TRUE;
        }
    }
#endif

  return FALSE;
}

/**
 * shumate_vector_reader_iter_get_feature_keys:
 * @self: A [class@VectorReaderIter].
 *
 * Gets the keys of the tags of the current feature.
 *
 * Returns: (transfer container) (array zero-terminated=1): The keys of the tags of the current feature.
 *
 * Since: 1.2
 */
const char **
shumate_vector_reader_iter_get_feature_keys (ShumateVectorReaderIter *self)
{
  g_return_val_if_fail (SHUMATE_IS_VECTOR_READER_ITER (self), NULL);

#ifdef SHUMATE_HAS_VECTOR_RENDERER
  const char **keys;
  uint32_t n_keys;

  g_return_val_if_fail (self->feature != NULL, NULL);

  n_keys = self->feature->n_tags / 2;
  keys = g_new (const char *, n_keys + 1);
  for (int i = 0; i < n_keys; i ++)
    keys[i] = self->layer->keys[self->feature->tags[i * 2]];
  keys[n_keys] = NULL;

  return keys;
#else
  return NULL;
#endif
}

/**
 * shumate_vector_reader_iter_get_feature_geometry_type:
 * @self: A [class@VectorReaderIter].
 *
 * Gets the geometry type of the current feature.
 *
 * Returns: The geometry type of the current feature.
 *
 * Since: 1.2
 */
ShumateGeometryType
shumate_vector_reader_iter_get_feature_geometry_type (ShumateVectorReaderIter *self)
{
  /* MVT doesn't distinguish between multi geometries and single geometries, but
     we do, so this function contains a bunch of extra logic to determine whether
     the geometry data contains multiple geometries. */

  g_return_val_if_fail (SHUMATE_IS_VECTOR_READER_ITER (self), SHUMATE_GEOMETRY_TYPE_UNKNOWN);

#ifdef SHUMATE_HAS_VECTOR_RENDERER
  g_return_val_if_fail (self->feature != NULL, SHUMATE_GEOMETRY_TYPE_UNKNOWN);

  switch (self->feature->type)
    {
    case VECTOR_TILE__TILE__GEOM_TYPE__UNKNOWN:
      return SHUMATE_GEOMETRY_TYPE_UNKNOWN;

    case VECTOR_TILE__TILE__GEOM_TYPE__POINT:
      if (self->feature->n_geometry == 3)
        return SHUMATE_GEOMETRY_TYPE_POINT;
      else
        return SHUMATE_GEOMETRY_TYPE_MULTIPOINT;

    case VECTOR_TILE__TILE__GEOM_TYPE__LINESTRING:
      {
        ShumateVectorGeometryIter iter = { .feature = self->feature };
        int move_tos = 0;
        while (shumate_vector_geometry_iter (&iter))
          {
            if (iter.op == 1)
              {
                move_tos ++;
                if (move_tos > 1)
                  return SHUMATE_GEOMETRY_TYPE_MULTILINESTRING;
              }
          }
        return SHUMATE_GEOMETRY_TYPE_LINESTRING;
      }

    case VECTOR_TILE__TILE__GEOM_TYPE__POLYGON:
      {
        /* Use the shoelace formula to determine whether each ring is
           exterior or interior. A single polygon can have interior rings
           in addition to its exterior ring; a multipolygon has multiple
           exterior rings. */

        ShumateVectorGeometryIter iter = { .feature = self->feature };
        int prev_x = 0, prev_y = 0;
        double area = 0;
        int exterior_rings = 0;

        while (shumate_vector_geometry_iter (&iter))
          {
            /* See https://en.wikipedia.org/wiki/Shoelace_formula#Triangle_formula */
            if (iter.op == SHUMATE_VECTOR_GEOMETRY_OP_LINE_TO
                || iter.op == SHUMATE_VECTOR_GEOMETRY_OP_CLOSE_PATH)
              area += (double) prev_x * iter.y - (double) iter.x * prev_y;

            if (iter.op == SHUMATE_VECTOR_GEOMETRY_OP_CLOSE_PATH)
              {
                if (area > 0)
                  exterior_rings ++;

                if (exterior_rings > 1)
                  return SHUMATE_GEOMETRY_TYPE_MULTIPOLYGON;

                area = 0;
              }

            prev_x = iter.x;
            prev_y = iter.y;
          }
      }
      return SHUMATE_GEOMETRY_TYPE_POLYGON;

    default:
      return SHUMATE_GEOMETRY_TYPE_UNKNOWN;
    }
#else
  return SHUMATE_GEOMETRY_TYPE_UNKNOWN;
#endif
}

static int
zigzag (guint value)
{
  return (value >> 1) ^ (-(value & 1));
}

/**
 * shumate_vector_reader_iter_get_feature_point:
 * @self: A [class@VectorReaderIter].
 * @x: (out) (optional): The x coordinate of the point.
 * @y: (out) (optional): The y coordinate of the point.
 *
 * Gets the coordinates of the current feature in tile space, if the
 * feature is a single point.
 *
 * See [method@VectorReaderIter.get_layer_extent] to get the range
 * of the coordinates.
 *
 * It is an error to call this function if the feature is not a single point.
 * Use [method@VectorReaderIter.get_feature_geometry_type] to check
 * the feature's geometry type.
 *
 * Returns: %TRUE if the feature is a point, %FALSE otherwise.
 *
 * Since: 1.2
 */
gboolean
shumate_vector_reader_iter_get_feature_point (ShumateVectorReaderIter *self,
                                              double                  *x,
                                              double                  *y)
{
  g_return_val_if_fail (SHUMATE_IS_VECTOR_READER_ITER (self), FALSE);

#ifdef SHUMATE_HAS_VECTOR_RENDERER
  g_return_val_if_fail (self->feature != NULL, FALSE);
  g_return_val_if_fail (self->feature->type == VECTOR_TILE__TILE__GEOM_TYPE__POINT, FALSE);
  g_return_val_if_fail (self->feature->n_geometry == 3, FALSE);

  if (x != NULL)
    *x = zigzag (self->feature->geometry[1]);
  if (y != NULL)
    *y = zigzag (self->feature->geometry[2]);

  return TRUE;
#else
  return FALSE;
#endif
}

/**
 * shumate_vector_reader_iter_feature_contains_point:
 * @self: A [class@VectorReaderIter].
 *
 * Determines whether the current feature contains the given point.
 *
 * The point must be specified in tile space. See
 * [method@VectorReaderIter.get_layer_extent] to get the range of the
 * coordinates.
 *
 * Only polygon or multipolygon features can contain a point. For all
 * other feature types, this function returns %FALSE.
 *
 * If the point is on the border of the polygon, this function may return
 * either %TRUE or %FALSE.
 *
 * Returns: %TRUE if the feature contains the point, %FALSE otherwise.
 *
 * Since: 1.2
 */
gboolean
shumate_vector_reader_iter_feature_contains_point (ShumateVectorReaderIter *self,
                                                   double                   x,
                                                   double                   y)
{
  g_return_val_if_fail (SHUMATE_IS_VECTOR_READER_ITER (self), FALSE);

#ifdef SHUMATE_HAS_VECTOR_RENDERER
  ShumateVectorGeometryIter iter = {0};
  int prev_x = 0, prev_y = 0;
  int winding_number = 0;

  g_return_val_if_fail (self->feature != NULL, FALSE);

  if (self->feature->type != VECTOR_TILE__TILE__GEOM_TYPE__POLYGON)
    return FALSE;

  iter.feature = self->feature;

  /* See <https://web.archive.org/web/20130126163405/http://geomalgorithms.com/a03-_inclusion.html>.
     I chose the winding algorithm because it has fewer edge cases. */

  while (shumate_vector_geometry_iter (&iter))
    {
      switch (iter.op)
        {
          case SHUMATE_VECTOR_GEOMETRY_OP_MOVE_TO:
            break;
          case SHUMATE_VECTOR_GEOMETRY_OP_LINE_TO:
          case SHUMATE_VECTOR_GEOMETRY_OP_CLOSE_PATH:
            if (prev_y <= y && iter.y > y)
              {
                if ((iter.x - prev_x) * (y - prev_y) > (iter.y - prev_y) * (x - prev_x))
                  winding_number ++;
              }
            else if (prev_y > y && iter.y <= y)
              {
                if ((iter.x - prev_x) * (y - prev_y) < (iter.y - prev_y) * (x - prev_x))
                  winding_number --;
              }
            break;
        }

      prev_x = iter.x;
      prev_y = iter.y;
    }

  return winding_number != 0;
#else
  return FALSE;
#endif
}

/*< private >
 * shumate_vector_reader_iter_new:
 * @reader: A [class@VectorReader]
 *
 * Creates a new [class@VectorReaderIter] for @reader.
 *
 * The public API is [method@VectorReader.iterate].
 *
 * Returns: (transfer full): A new [class@VectorReaderIter]
 */
ShumateVectorReaderIter *
shumate_vector_reader_iter_new (ShumateVectorReader *reader)
{
  g_return_val_if_fail (SHUMATE_IS_VECTOR_READER (reader), NULL);

  return g_object_new (SHUMATE_TYPE_VECTOR_READER_ITER,
                       "reader", reader,
                       NULL);
}

#ifdef SHUMATE_HAS_VECTOR_RENDERER

/*< private >
 * shumate_vector_reader_iter_get_layer_struct:
 * @self: A [class@VectorReaderIter]
 *
 * Gets the raw protobuf struct for the current layer.
 *
 * Returns: (transfer none): a [struct@VectorTile__Tile__Layer]
 */
VectorTile__Tile__Layer *
shumate_vector_reader_iter_get_layer_struct (ShumateVectorReaderIter *self)
{
  g_return_val_if_fail (SHUMATE_IS_VECTOR_READER_ITER (self), NULL);
  return self->layer;
}

/*< private >
 * shumate_vector_reader_iter_get_feature_struct:
 *
 * Gets the raw protobuf struct for the current feature.
 *
 * Returns: (transfer none): a [struct@VectorTile__Tile__Feature]
 */
VectorTile__Tile__Feature *
shumate_vector_reader_iter_get_feature_struct (ShumateVectorReaderIter *self)
{
  g_return_val_if_fail (SHUMATE_IS_VECTOR_READER_ITER (self), NULL);
  return self->feature;
}

#endif