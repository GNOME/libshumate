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


#include "shumate-vector-symbol-private.h"
#include "shumate-vector-utils-private.h"
#include "shumate-vector-symbol-info-private.h"


struct _ShumateVectorSymbol
{
  GtkWidget parent_instance;

  ShumateVectorSymbolInfo *symbol_info;
};

G_DEFINE_TYPE (ShumateVectorSymbol, shumate_vector_symbol, GTK_TYPE_WIDGET)


enum {
  PROP_0,
  PROP_SYMBOL_INFO,
  N_PROPS,
};

static GParamSpec *obj_properties[N_PROPS] = { NULL, };


ShumateVectorSymbol *
shumate_vector_symbol_new (ShumateVectorSymbolInfo *symbol_info)
{
  return g_object_new (SHUMATE_TYPE_VECTOR_SYMBOL,
                       "symbol-info", symbol_info,
                       NULL);
}


static void
shumate_vector_symbol_constructed (GObject *object)
{
  ShumateVectorSymbol *self = (ShumateVectorSymbol *)object;
  g_autoptr(PangoAttrList) attrs = pango_attr_list_new ();
  PangoAttribute *attr;

  if (self->symbol_info->text_font != NULL)
    {
      g_autoptr(PangoFontDescription) desc = pango_font_description_from_string (self->symbol_info->text_font);
      attr = pango_attr_font_desc_new (desc);
      pango_attr_list_insert (attrs, attr);
    }

  attr = pango_attr_foreground_new (self->symbol_info->text_color.red * 65535,
                                    self->symbol_info->text_color.green * 65535,
                                    self->symbol_info->text_color.blue * 65535);
  pango_attr_list_insert (attrs, attr);

  attr = pango_attr_foreground_alpha_new (self->symbol_info->text_color.alpha * 65535);
  pango_attr_list_insert (attrs, attr);

  attr = pango_attr_size_new_absolute (self->symbol_info->text_size * PANGO_SCALE);
  pango_attr_list_insert (attrs, attr);

  GtkWidget *label = gtk_label_new (self->symbol_info->text);
  gtk_label_set_attributes (GTK_LABEL (label), attrs);
  gtk_widget_set_parent (label, GTK_WIDGET (self));

  G_OBJECT_CLASS (shumate_vector_symbol_parent_class)->constructed (object);
}


static void
shumate_vector_symbol_dispose (GObject *object)
{
  ShumateVectorSymbol *self = (ShumateVectorSymbol *)object;
  GtkWidget *child;

  while ((child = gtk_widget_get_first_child (GTK_WIDGET (object))))
    gtk_widget_unparent (child);

  g_clear_pointer (&self->symbol_info, shumate_vector_symbol_info_unref);

  G_OBJECT_CLASS (shumate_vector_symbol_parent_class)->dispose (object);
}


static void
shumate_vector_symbol_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  ShumateVectorSymbol *self = (ShumateVectorSymbol *)object;

  switch (property_id)
    {
    case PROP_SYMBOL_INFO:
      g_value_set_boxed (value, self->symbol_info);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}


static void
shumate_vector_symbol_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  ShumateVectorSymbol *self = SHUMATE_VECTOR_SYMBOL (object);

  switch (property_id)
    {
    case PROP_SYMBOL_INFO:
      g_assert (self->symbol_info == NULL);
      self->symbol_info = g_value_dup_boxed (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}


static void
shumate_vector_symbol_class_init (ShumateVectorSymbolClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = shumate_vector_symbol_constructed;
  object_class->dispose = shumate_vector_symbol_dispose;
  object_class->get_property = shumate_vector_symbol_get_property;
  object_class->set_property = shumate_vector_symbol_set_property;

  obj_properties[PROP_SYMBOL_INFO] =
    g_param_spec_boxed ("symbol-info",
                        "Symbol info",
                        "Symbol info",
                        SHUMATE_TYPE_VECTOR_SYMBOL_INFO,
                        G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, N_PROPS, obj_properties);

  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
}


static void
shumate_vector_symbol_init (ShumateVectorSymbol *self)
{
}


ShumateVectorSymbolInfo *
shumate_vector_symbol_get_symbol_info (ShumateVectorSymbol *self)
{
  g_return_val_if_fail (SHUMATE_IS_VECTOR_SYMBOL (self), NULL);

  return self->symbol_info;
}
