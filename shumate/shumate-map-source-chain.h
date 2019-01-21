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

#ifndef _SHUMATE_MAP_SOURCE_CHAIN_H_
#define _SHUMATE_MAP_SOURCE_CHAIN_H_

#include <glib-object.h>

#include "shumate-map-source.h"

G_BEGIN_DECLS

#define SHUMATE_TYPE_MAP_SOURCE_CHAIN shumate_map_source_chain_get_type ()

#define SHUMATE_MAP_SOURCE_CHAIN(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SHUMATE_TYPE_MAP_SOURCE_CHAIN, ShumateMapSourceChain))

#define SHUMATE_MAP_SOURCE_CHAIN_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SHUMATE_TYPE_MAP_SOURCE_CHAIN, ShumateMapSourceChainClass))

#define SHUMATE_IS_MAP_SOURCE_CHAIN(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SHUMATE_TYPE_MAP_SOURCE_CHAIN))

#define SHUMATE_IS_MAP_SOURCE_CHAIN_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SHUMATE_TYPE_MAP_SOURCE_CHAIN))

#define SHUMATE_MAP_SOURCE_CHAIN_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), SHUMATE_TYPE_MAP_SOURCE_CHAIN, ShumateMapSourceChainClass))

typedef struct _ShumateMapSourceChainPrivate ShumateMapSourceChainPrivate;

typedef struct _ShumateMapSourceChain ShumateMapSourceChain;
typedef struct _ShumateMapSourceChainClass ShumateMapSourceChainClass;

/**
 * ShumateMapSourceChain:
 *
 * The #ShumateMapSourceChain structure contains only private data
 * and should be accessed using the provided API
 *
 * Since: 0.6
 */
struct _ShumateMapSourceChain
{
  ShumateMapSource parent_instance;

  ShumateMapSourceChainPrivate *priv;
};

struct _ShumateMapSourceChainClass
{
  ShumateMapSourceClass parent_class;
};

GType shumate_map_source_chain_get_type (void);

ShumateMapSourceChain *shumate_map_source_chain_new (void);

void shumate_map_source_chain_push (ShumateMapSourceChain *source_chain,
    ShumateMapSource *map_source);
void shumate_map_source_chain_pop (ShumateMapSourceChain *source_chain);

G_END_DECLS

#endif /* _SHUMATE_MAP_SOURCE_CHAIN_H_ */
