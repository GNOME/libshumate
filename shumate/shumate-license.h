/*
 * Copyright (C) 2011-2013 Jiri Techet <techet@gmail.com>
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

#ifndef SHUMATE_LICENSE_H
#define SHUMATE_LICENSE_H

#include <shumate/shumate-defines.h>

#include <glib-object.h>
#include <clutter/clutter.h>

G_BEGIN_DECLS

#define SHUMATE_TYPE_LICENSE shumate_license_get_type ()

#define SHUMATE_LICENSE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SHUMATE_TYPE_LICENSE, ShumateLicense))

#define SHUMATE_LICENSE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SHUMATE_TYPE_LICENSE, ShumateLicenseClass))

#define SHUMATE_IS_LICENSE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SHUMATE_TYPE_LICENSE))

#define SHUMATE_IS_LICENSE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SHUMATE_TYPE_LICENSE))

#define SHUMATE_LICENSE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), SHUMATE_TYPE_LICENSE, ShumateLicenseClass))

typedef struct _ShumateLicensePrivate ShumateLicensePrivate;

typedef struct _ShumateLicense ShumateLicense;
typedef struct _ShumateLicenseClass ShumateLicenseClass;


/**
 * ShumateLicense:
 *
 * The #ShumateLicense structure contains only private data
 * and should be accessed using the provided API
 */
struct _ShumateLicense
{
  ClutterActor parent;

  ShumateLicensePrivate *priv;
};

struct _ShumateLicenseClass
{
  ClutterActorClass parent_class;
};

GType shumate_license_get_type (void);

ClutterActor *shumate_license_new (void);

void shumate_license_set_extra_text (ShumateLicense *license,
    const gchar *text);
const gchar *shumate_license_get_extra_text (ShumateLicense *license);

void shumate_license_set_alignment (ShumateLicense *license,
    PangoAlignment alignment);
PangoAlignment shumate_license_get_alignment (ShumateLicense *license);

void shumate_license_connect_view (ShumateLicense *license,
    ShumateView *view);
void shumate_license_disconnect_view (ShumateLicense *license);

G_END_DECLS

#endif
