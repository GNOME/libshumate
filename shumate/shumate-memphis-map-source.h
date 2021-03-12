/*
 * Copyright 2021 Collabora Ltd. (https://www.collabora.com)
 * Copyright 2021 Corentin Noël <corentin.noel@collabora.com>
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

#ifndef _SHUMATE_MEMPHIS_MAP_SOURCE_H_
#define _SHUMATE_MEMPHIS_MAP_SOURCE_H_

#include <shumate/shumate-map-source.h>

G_BEGIN_DECLS

#define SHUMATE_TYPE_MEMPHIS_MAP_SOURCE shumate_memphis_map_source_get_type()
G_DECLARE_FINAL_TYPE (ShumateMemphisMapSource, shumate_memphis_map_source, SHUMATE, MEMPHIS_MAP_SOURCE, ShumateMapSource)

ShumateMemphisMapSource *shumate_memphis_map_source_new (void);

G_END_DECLS

#endif
