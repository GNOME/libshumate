/*
 * Copyright 2026 Corentin Noël <corentin.noel@collabora.com>
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

#ifndef SHUMATE_MARKER_ITEM_FACTORY_PRIVATE_H
#define SHUMATE_MARKER_ITEM_FACTORY_PRIVATE_H

#include <glib.h>

#include "shumate-marker-item-factory.h"

G_BEGIN_DECLS

struct _ShumateMarkerItemFactory
{
  GObject parent_instance;
};

struct _ShumateMarkerItemFactoryClass
{
  GObjectClass parent_class;

  /* setup @list_item so it can be bound */
  void (* setup)    (ShumateMarkerItemFactory *self,
                     GObject                  *item,
                     gboolean                  bind,
                     GFunc                     func,
                     gpointer                  data);
  /* undo the effects of ShumateMarkerItemFactoryClass::setup() */
  void (* teardown) (ShumateMarkerItemFactory *self,
                     GObject                  *item,
                     gboolean                  unbind,
                     GFunc                     func,
                     gpointer                  data);

  /* Update properties on @list_item to the given @item, which is in @position and @selected state.
   * One or more of those properties might be unchanged. */
  void (* update)   (ShumateMarkerItemFactory *self,
                     GObject                  *item,
                     gboolean                  unbind,
                     gboolean                  bind,
                     GFunc                     func,
                     gpointer                  data);
};

void shumate_marker_item_factory_setup    (ShumateMarkerItemFactory *self,
                                           GObject                  *item,
                                           gboolean                  bind,
                                           GFunc                     func,
                                           gpointer                  data);
void shumate_marker_item_factory_teardown (ShumateMarkerItemFactory *self,
                                           GObject                  *item,
                                           gboolean                  unbind,
                                           GFunc                     func,
                                           gpointer                  data);

void shumate_marker_item_factory_update   (ShumateMarkerItemFactory *self,
                                           GObject                  *item,
                                           gboolean                  unbind,
                                           gboolean                  bind,
                                           GFunc                     func,
                                           gpointer                  data);


G_END_DECLS

#endif
