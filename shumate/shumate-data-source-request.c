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

#include "shumate-data-source-request.h"
#include "shumate-utils-private.h"
#include "shumate-profiling-private.h"


/**
 * ShumateDataSourceRequest:
 *
 * Represents a request to a [class@DataSource] for a tile.
 *
 * Data sources can return a tile multiple times. For example, a
 * [class@TileDownloader] may return cached data first, then later return data
 * from a network service when it arrives. This allows the map to be rendered
 * as quickly as possible without waiting for the network unnecessarily.
 *
 * Conventional async/finish method pairs don't support multiple returns.
 * Instead, [method@DataSource.start_request] is available, which returns a
 * [class@DataSourceRequest] whose properties, [property@DataSourceRequest:data]
 * and [property@DataSourceRequest:error], update as data becomes available.
 * The [signal@GObject.Object::notify] signal can be used to watch for these
 * changes. When the request is done and no more data will be returned,
 * [property@DataSourceRequest:completed] is set to %TRUE.
 *
 * [class@DataSource] implementations can use a subclass of
 * [class@DataSourceRequest], but the base class should be sufficient in most
 * cases.
 *
 * Since: 1.1
 */


typedef struct {
  GObject parent;

  ShumateGridPosition pos;
  GBytes *bytes;
  GError *error;

  gboolean completed : 1;
} ShumateDataSourceRequestPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (ShumateDataSourceRequest, shumate_data_source_request, G_TYPE_OBJECT)

enum {
  PROP_0,
  PROP_X,
  PROP_Y,
  PROP_ZOOM_LEVEL,
  PROP_DATA,
  PROP_ERROR,
  PROP_COMPLETED,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];

static void
shumate_data_source_request_finalize (GObject *object)
{
  ShumateDataSourceRequest *self = (ShumateDataSourceRequest *)object;
  ShumateDataSourceRequestPrivate *priv = shumate_data_source_request_get_instance_private (self);

  g_clear_pointer (&priv->bytes, g_bytes_unref);
  g_clear_error (&priv->error);

  G_OBJECT_CLASS (shumate_data_source_request_parent_class)->finalize (object);
}

static void
shumate_data_source_request_get_property (GObject    *object,
                                          guint       prop_id,
                                          GValue     *value,
                                          GParamSpec *pspec)
{
  ShumateDataSourceRequest *self = SHUMATE_DATA_SOURCE_REQUEST (object);
  ShumateDataSourceRequestPrivate *priv = shumate_data_source_request_get_instance_private (self);

  switch (prop_id)
    {
    case PROP_X:
      g_value_set_int (value, priv->pos.x);
      break;

    case PROP_Y:
      g_value_set_int (value, priv->pos.y);
      break;

    case PROP_ZOOM_LEVEL:
      g_value_set_int (value, priv->pos.zoom);
      break;

    case PROP_DATA:
      g_value_set_boxed (value, priv->bytes);
      break;

    case PROP_ERROR:
      g_value_set_boxed (value, priv->error);
      break;

    case PROP_COMPLETED:
      g_value_set_boolean (value, priv->completed);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
shumate_data_source_request_set_property (GObject      *object,
                                          guint         prop_id,
                                          const GValue *value,
                                          GParamSpec   *pspec)
{
  ShumateDataSourceRequest *self = SHUMATE_DATA_SOURCE_REQUEST (object);
  ShumateDataSourceRequestPrivate *priv = shumate_data_source_request_get_instance_private (self);

  switch (prop_id)
    {
    case PROP_X:
      priv->pos.x = g_value_get_int (value);
      break;

    case PROP_Y:
      priv->pos.y = g_value_get_int (value);
      break;

    case PROP_ZOOM_LEVEL:
      priv->pos.zoom = g_value_get_int (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
shumate_data_source_request_class_init (ShumateDataSourceRequestClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = shumate_data_source_request_finalize;
  object_class->get_property = shumate_data_source_request_get_property;
  object_class->set_property = shumate_data_source_request_set_property;

  /**
   * ShumateDataSourceRequest:x:
   *
   * The X coordinate of the requested tile.
   *
   * Since: 1.1
   */
  properties[PROP_X] =
    g_param_spec_int ("x", "x", "x",
                      G_MININT, G_MAXINT, 0,
                      G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * ShumateDataSourceRequest:y:
   *
   * The Y coordinate of the requested tile.
   *
   * Since: 1.1
   */
  properties[PROP_Y] =
    g_param_spec_int ("y", "y", "y",
                      G_MININT, G_MAXINT, 0,
                      G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * ShumateDataSourceRequest:zoom-level:
   *
   * The zoom level of the requested tile.
   *
   * Since: 1.1
   */
  properties[PROP_ZOOM_LEVEL] =
    g_param_spec_int ("zoom-level", "zoom-level", "zoom-level",
                      G_MININT, G_MAXINT, 0,
                      G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * ShumateDataSourceRequest:data:
   *
   * The most recent data for the tile, if available. If an error is emitted,
   * this will be set to %NULL.
   *
   * Since: 1.1
   */
  properties[PROP_DATA] =
    g_param_spec_boxed ("data", "data", "data",
                        G_TYPE_BYTES,
                        G_PARAM_STATIC_STRINGS | G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * ShumateDataSourceRequest:error:
   *
   * The error that occurred during the request, if any.
   *
   * Since: 1.1
   */
  properties[PROP_ERROR] =
    g_param_spec_boxed ("error", "error", "error",
                        G_TYPE_ERROR,
                        G_PARAM_STATIC_STRINGS | G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * ShumateDataSourceRequest:completed:
   *
   * %TRUE if the request has been completed, otherwise %FALSE. A completed
   * request will not receive further updates to either
   * [property@DataSourceRequest:data] or [property@DataSourceRequest:error].
   *
   * Since: 1.1
   */
  properties[PROP_COMPLETED] =
    g_param_spec_boolean ("completed", "completed", "completed",
                          FALSE,
                          G_PARAM_STATIC_STRINGS | G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
shumate_data_source_request_init (ShumateDataSourceRequest *self)
{
}

/**
 * shumate_data_source_request_get_x:
 * @self: a [class@DataSourceRequest]
 *
 * Gets the X coordinate of the requested tile.
 *
 * Returns: the X coordinate
 *
 * Since: 1.1
 */
int
shumate_data_source_request_get_x (ShumateDataSourceRequest *self)
{
  ShumateDataSourceRequestPrivate *priv = shumate_data_source_request_get_instance_private (self);
  g_return_val_if_fail (SHUMATE_IS_DATA_SOURCE_REQUEST (self), 0);
  return priv->pos.x;
}

/**
 * shumate_data_source_request_get_y:
 * @self: a [class@DataSourceRequest]
 *
 * Gets the Y coordinate of the requested tile.
 *
 * Returns: the Y coordinate
 *
 * Since: 1.1
 */
int
shumate_data_source_request_get_y (ShumateDataSourceRequest *self)
{
  ShumateDataSourceRequestPrivate *priv = shumate_data_source_request_get_instance_private (self);
  g_return_val_if_fail (SHUMATE_IS_DATA_SOURCE_REQUEST (self), 0);
  return priv->pos.y;
}

/**
 * shumate_data_source_request_get_zoom_level:
 * @self: a [class@DataSourceRequest]
 *
 * Gets the zoom level of the requested tile.
 *
 * Returns: the zoom level
 *
 * Since: 1.1
 */
int
shumate_data_source_request_get_zoom_level (ShumateDataSourceRequest *self)
{
  ShumateDataSourceRequestPrivate *priv = shumate_data_source_request_get_instance_private (self);
  g_return_val_if_fail (SHUMATE_IS_DATA_SOURCE_REQUEST (self), 0);
  return priv->pos.zoom;
}

/**
 * shumate_data_source_request_get_data:
 * @self: a [class@DataSourceRequest]
 *
 * Gets the latest data from the request.
 *
 * Returns: (transfer none) (nullable): The latest data, if any.
 *
 * Since: 1.1
 */
GBytes *
shumate_data_source_request_get_data (ShumateDataSourceRequest *self)
{
  ShumateDataSourceRequestPrivate *priv = shumate_data_source_request_get_instance_private (self);
  g_return_val_if_fail (SHUMATE_IS_DATA_SOURCE_REQUEST (self), NULL);
  return priv->bytes;
}

/**
 * shumate_data_source_request_get_error:
 * @self: a [class@DataSourceRequest]
 *
 * Gets the latest error from the request.
 *
 * Returns: (transfer none) (nullable): The latest error, if any.
 *
 * Since: 1.1
 */
GError *
shumate_data_source_request_get_error (ShumateDataSourceRequest *self)
{
  ShumateDataSourceRequestPrivate *priv = shumate_data_source_request_get_instance_private (self);
  g_return_val_if_fail (SHUMATE_IS_DATA_SOURCE_REQUEST (self), NULL);
  return priv->error;
}

/**
 * shumate_data_source_request_is_completed:
 * @self: a [class@DataSourceRequest]
 *
 * Gets whether the request has been completed. Completed requests will not
 * receive new data or errors.
 *
 * Returns: %TRUE if the request is completed, otherwise %FALSE
 *
 * Since: 1.1
 */
gboolean
shumate_data_source_request_is_completed (ShumateDataSourceRequest *self)
{
  ShumateDataSourceRequestPrivate *priv = shumate_data_source_request_get_instance_private (self);
  g_return_val_if_fail (SHUMATE_IS_DATA_SOURCE_REQUEST (self), FALSE);
  return priv->completed;
}

/**
 * shumate_data_source_request_new:
 * @x: X coordinate of the requested tile
 * @y: Y coordinate of the requested tile
 * @zoom_level: Zoom level of the requested tile
 *
 * Creates a new [class@DataSourceRequest].
 *
 * Only implementations of [vfunc@DataSource.start_request] should need to
 * construct a new request object.
 *
 * Returns: (transfer full): a new [class@DataSourceRequest]
 *
 * Since: 1.1
 */
ShumateDataSourceRequest *
shumate_data_source_request_new (int x,
                                 int y,
                                 int zoom_level)
{
  return g_object_new (SHUMATE_TYPE_DATA_SOURCE_REQUEST,
                       "x", x,
                       "y", y,
                       "zoom-level", zoom_level,
                       NULL);
}

/**
 * shumate_data_source_request_emit_data:
 * @self: a [class@DataSourceRequest]
 * @data: the data to emit
 * @complete: %TRUE to also complete the request, %FALSE otherwise
 *
 * Emits tile data as a response to the request. This sets the
 * [property@DataSourceRequest:data] property.
 *
 * If @complete is %TRUE, then [property@DataSourceRequest:completed] is set to
 * %TRUE as well.
 *
 * Since: 1.1
 */
void
shumate_data_source_request_emit_data (ShumateDataSourceRequest *self,
                                       GBytes                   *data,
                                       gboolean                  complete)
{
  ShumateDataSourceRequestPrivate *priv = shumate_data_source_request_get_instance_private (self);
  g_autofree char *profiling_desc = NULL;

  g_return_if_fail (SHUMATE_IS_DATA_SOURCE_REQUEST (self));
  g_return_if_fail (data != NULL);
  g_return_if_fail (!priv->completed);

  if (priv->bytes != NULL && g_bytes_equal (data, priv->bytes))
    return;

  g_clear_pointer (&priv->bytes, g_bytes_unref);
  priv->bytes = g_bytes_ref (data);

  if (complete)
    priv->completed = TRUE;

  profiling_desc = g_strdup_printf ("(%d, %d) @ %d", priv->pos.x, priv->pos.y, priv->pos.zoom);
  SHUMATE_PROFILE_START_NAMED (emit_data);
  g_object_notify_by_pspec ((GObject *)self, properties[PROP_DATA]);
  SHUMATE_PROFILE_END_NAMED (emit_data, profiling_desc);

  if (complete)
    g_object_notify_by_pspec ((GObject *)self, properties[PROP_COMPLETED]);
}

/**
 * shumate_data_source_request_emit_error:
 * @self: a [class@DataSourceRequest]
 * @error: an error
 *
 * Emits a fatal error in response to the request. This completes the request,
 * so no more data or errors can be emitted after this. Non-fatal errors should
 * not be reported.
 *
 * If [property@DataSourceRequest:data] was previously set, it will be cleared.
 *
 * Since: 1.1
 */
void
shumate_data_source_request_emit_error (ShumateDataSourceRequest *self,
                                        const GError             *error)
{
  ShumateDataSourceRequestPrivate *priv = shumate_data_source_request_get_instance_private (self);

  g_return_if_fail (SHUMATE_IS_DATA_SOURCE_REQUEST (self));
  g_return_if_fail (error != NULL);
  g_return_if_fail (!priv->completed);

  g_clear_error (&priv->error);
  priv->error = g_error_copy (error);

  priv->completed = TRUE;

  if (priv->bytes)
    {
      g_clear_pointer (&priv->bytes, g_bytes_unref);
      g_object_notify_by_pspec ((GObject *)self, properties[PROP_DATA]);
    }

  g_object_notify_by_pspec ((GObject *)self, properties[PROP_ERROR]);
  g_object_notify_by_pspec ((GObject *)self, properties[PROP_COMPLETED]);
}

/**
 * shumate_data_source_request_complete:
 * @self: a [class@DataSourceRequest]
 *
 * Marks the request as complete. No more data or errors may be emitted.
 *
 * This can only be called if data has been emitted. If there is no data,
 * use [method@DataSourceRequest.emit_error] instead, which will automatically
 * complete the request.
 *
 * Since: 1.1
 */
void
shumate_data_source_request_complete (ShumateDataSourceRequest *self)
{
  ShumateDataSourceRequestPrivate *priv = shumate_data_source_request_get_instance_private (self);

  g_return_if_fail (SHUMATE_IS_DATA_SOURCE_REQUEST (self));
  g_return_if_fail (!priv->completed);
  g_return_if_fail (priv->bytes != NULL || priv->error != NULL);

  priv->completed = TRUE;
  g_object_notify_by_pspec ((GObject *)self, properties[PROP_COMPLETED]);
}
