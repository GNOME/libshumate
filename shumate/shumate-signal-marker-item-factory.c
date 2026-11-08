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

#include "shumate-marker-item-factory-private.h"
#include "shumate-signal-marker-item-factory.h"

struct _ShumateSignalMarkerItemFactoryClass
{
  ShumateMarkerItemFactoryClass parent_instance;

  void (* setup)    (ShumateSignalMarkerItemFactory *self,
                     GObject                        *marker_item);
  void (* teardown) (ShumateSignalMarkerItemFactory *self,
                     GObject                        *marker_item);
  void (* bind)     (ShumateSignalMarkerItemFactory *self,
                     GObject                        *marker_item);
  void (* unbind)   (ShumateSignalMarkerItemFactory *self,
                     GObject                        *marker_item);
};

struct _ShumateSignalMarkerItemFactory
{
  ShumateMarkerItemFactory parent_instance;
};

G_DEFINE_FINAL_TYPE (ShumateSignalMarkerItemFactory, shumate_signal_marker_item_factory, SHUMATE_TYPE_MARKER_ITEM_FACTORY)

enum {
  SETUP,
  BIND,
  UNBIND,
  TEARDOWN,

  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static void
shumate_signal_marker_item_factory_setup (ShumateMarkerItemFactory *factory,
                                    GObject            *item,
                                    gboolean            bind,
                                    GFunc               func,
                                    gpointer            data)
{
  g_signal_emit (factory, signals[SETUP], 0, item);

  SHUMATE_MARKER_ITEM_FACTORY_CLASS (shumate_signal_marker_item_factory_parent_class)->setup (factory, item, bind, func, data);

  if (bind)
    g_signal_emit (factory, signals[BIND], 0, item);
}

static void
shumate_signal_marker_item_factory_update (ShumateMarkerItemFactory *factory,
                                     GObject            *item,
                                     gboolean            unbind,
                                     gboolean            bind,
                                     GFunc               func,
                                     gpointer            data)
{
  if (unbind)
    g_signal_emit (factory, signals[UNBIND], 0, item);

  SHUMATE_MARKER_ITEM_FACTORY_CLASS (shumate_signal_marker_item_factory_parent_class)->update (factory, item, unbind, bind, func, data);

  if (bind)
    g_signal_emit (factory, signals[BIND], 0, item);
}

static void
shumate_signal_marker_item_factory_teardown (ShumateMarkerItemFactory *factory,
                                       GObject            *item,
                                       gboolean            unbind,
                                       GFunc               func,
                                       gpointer            data)
{
  if (unbind)
    g_signal_emit (factory, signals[UNBIND], 0, item);

  SHUMATE_MARKER_ITEM_FACTORY_CLASS (shumate_signal_marker_item_factory_parent_class)->teardown (factory, item, unbind, func, data);

  g_signal_emit (factory, signals[TEARDOWN], 0, item);
}


static void
shumate_signal_marker_item_factory_class_init (ShumateSignalMarkerItemFactoryClass *klass)
{
  ShumateMarkerItemFactoryClass *factory_class = SHUMATE_MARKER_ITEM_FACTORY_CLASS (klass);

  factory_class->setup = shumate_signal_marker_item_factory_setup;
  factory_class->teardown = shumate_signal_marker_item_factory_teardown;
  factory_class->update = shumate_signal_marker_item_factory_update;

  /**
   * ShumateSignalMarkerItemFactory::setup:
   * @self: the list item factory
   * @object: the `GObject` to set up
   *
   * Emitted when a newly created listitem needs to be prepared for use.
   *
   * It is the first signal emitted for every listitem.
   *
   * The handler for this signal must call [method@Gtk.ListItem.set_child]
   * to populate the listitem with widgets.
   *
   * The [signal@Shumate.SignalMarkerItemFactory::teardown] signal is the opposite
   * of this signal and can be used to undo everything done in this signal.
   */
  signals[SETUP] =
    g_signal_new ("setup",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ShumateSignalMarkerItemFactoryClass, setup),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1,
                  G_TYPE_OBJECT);
  g_signal_set_va_marshaller (signals[SETUP],
                              G_TYPE_FROM_CLASS (klass),
                              g_cclosure_marshal_VOID__OBJECTv);

  /**
   * ShumateSignalMarkerItemFactory::bind:
   * @self: the list item factory
   * @object: The `GObject` to bind
   *
   * Emitted when an object has been bound to an item.
   *
   * The handler for this signal must set
   * to populate the listitem with widgets.
   *
   * After this signal was emitted, the object might be shown in
   * a [class@Gtk.ListView] or other widget.
   *
   * The [signal@Shumate.SignalMarkerItemFactory::unbind] signal is the
   * opposite of this signal and can be used to undo everything done
   * in this signal.
   */
  signals[BIND] =
    g_signal_new ("bind",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ShumateSignalMarkerItemFactoryClass, bind),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1,
                  G_TYPE_OBJECT);
  g_signal_set_va_marshaller (signals[BIND],
                              G_TYPE_FROM_CLASS (klass),
                              g_cclosure_marshal_VOID__OBJECTv);

  /**
   * ShumateSignalMarkerItemFactory::unbind:
   * @self: the list item factory
   * @object: The `GObject` to unbind
   *
   * Emitted when an object has been unbound from its item.
   *
   * This happens for example when a listitem was removed from use
   * in a list widget and its [property@Gtk.ListItem:item] is about
   * to be unset.
   *
   * This signal is the opposite of the [signal@Shumate.SignalMarkerItemFactory::bind]
   * signal and should be used to undo everything done in that signal.
   */
  signals[UNBIND] =
    g_signal_new ("unbind",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ShumateSignalMarkerItemFactoryClass, unbind),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1,
                  G_TYPE_OBJECT);
  g_signal_set_va_marshaller (signals[UNBIND],
                              G_TYPE_FROM_CLASS (klass),
                              g_cclosure_marshal_VOID__OBJECTv);

  /**
   * ShumateSignalMarkerItemFactory::teardown:
   * @self: The `ShumateSignalMarkerItemFactory`
   * @object: The `GObject` to tear down
   *
   * Emitted when an object is about to be destroyed.
   *
   * It is the last signal ever emitted for this @object.
   *
   * This signal is the opposite of the [signal@Shumate.SignalMarkerItemFactory::setup]
   * signal and should be used to undo everything done in that signal.
   */
  signals[TEARDOWN] =
    g_signal_new ("teardown",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ShumateSignalMarkerItemFactoryClass, teardown),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1,
                  G_TYPE_OBJECT);
  g_signal_set_va_marshaller (signals[TEARDOWN],
                              G_TYPE_FROM_CLASS (klass),
                              g_cclosure_marshal_VOID__OBJECTv);

}

static void
shumate_signal_marker_item_factory_init (ShumateSignalMarkerItemFactory *self)
{
}



ShumateMarkerItemFactory *
shumate_signal_marker_item_factory_new (void)
{
  return g_object_new (SHUMATE_TYPE_SIGNAL_MARKER_ITEM_FACTORY, NULL);
}
