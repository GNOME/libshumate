/*
 * Copyright (C) 2010-2013 Jiri Techet <techet@gmail.com>
 * Copyright (C) 2019 Marcus Lundblad <ml@update.uu.se>
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#if !defined (__SHUMATE_SHUMATE_H_INSIDE__) && !defined (SHUMATE_COMPILATION)
#error "Only <shumate/shumate.h> can be included directly."
#endif

#ifndef __SHUMATE_IMAGE_RENDERER_H__
#define __SHUMATE_IMAGE_RENDERER_H__

#include <shumate/shumate-tile.h>
#include <shumate/shumate-renderer.h>

G_BEGIN_DECLS

#define SHUMATE_TYPE_IMAGE_RENDERER shumate_image_renderer_get_type ()

#define SHUMATE_IMAGE_RENDERER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SHUMATE_TYPE_IMAGE_RENDERER, ShumateImageRenderer))

#define SHUMATE_IMAGE_RENDERER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SHUMATE_TYPE_IMAGE_RENDERER, ShumateImageRendererClass))

#define SHUMATE_IS_IMAGE_RENDERER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SHUMATE_TYPE_IMAGE_RENDERER))

#define SHUMATE_IS_IMAGE_RENDERER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SHUMATE_TYPE_IMAGE_RENDERER))

#define SHUMATE_IMAGE_RENDERER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), SHUMATE_TYPE_IMAGE_RENDERER, ShumateImageRendererClass))

typedef struct _ShumateImageRendererPrivate ShumateImageRendererPrivate;

typedef struct _ShumateImageRenderer ShumateImageRenderer;
typedef struct _ShumateImageRendererClass ShumateImageRendererClass;

/**
 * ShumateImageRenderer:
 *
 * The #ShumateImageRenderer structure contains only private data
 * and should be accessed using the provided API
 *
 * Since: 0.8
 */
struct _ShumateImageRenderer
{
  ShumateRenderer parent;

  ShumateImageRendererPrivate *priv;
};

struct _ShumateImageRendererClass
{
  ShumateRendererClass parent_class;
};

GType shumate_image_renderer_get_type (void);

ShumateImageRenderer *shumate_image_renderer_new (void);

G_END_DECLS

#endif /* __SHUMATE_IMAGE_RENDERER_H__ */
