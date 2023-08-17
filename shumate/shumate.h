/*
 * Copyright (C) 2008-2009 Pierre-Luc Beaudoin <pierre-luc@pierlux.com>
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

#ifndef SHUMATE_H
#define SHUMATE_H

#define __SHUMATE_SHUMATE_H_INSIDE__

#include <glib.h>

#include "shumate/shumate-enum-types.h"
#include "shumate/shumate-version.h"

#include "shumate/shumate-data-source.h"
#include "shumate/shumate-data-source-request.h"
#include "shumate/shumate-license.h"
#include "shumate/shumate-layer.h"
#include "shumate/shumate-map-layer.h"
#include "shumate/shumate-marker-layer.h"
#include "shumate/shumate-path-layer.h"
#include "shumate/shumate-point.h"
#include "shumate/shumate-location.h"
#include "shumate/shumate-coordinate.h"
#include "shumate/shumate-compass.h"
#include "shumate/shumate-marker.h"
#include "shumate/shumate-map.h"
#include "shumate/shumate-raster-renderer.h"
#include "shumate/shumate-viewport.h"
#include "shumate/shumate-scale.h"
#include "shumate/shumate-simple-map.h"
#include "shumate/shumate-map-source.h"
#include "shumate/shumate-map-source-registry.h"
#include "shumate/shumate-symbol-event.h"
#include "shumate/shumate-tile-downloader.h"
#include "shumate/shumate-user-agent.h"

#include "shumate/shumate-file-cache.h"

#include "shumate/shumate-vector-sprite.h"
#include "shumate/shumate-vector-sprite-sheet.h"
#include "shumate/shumate-vector-reader.h"
#include "shumate/shumate-vector-reader-iter.h"
#include "shumate/shumate-vector-renderer.h"

#undef __SHUMATE_SHUMATE_H_INSIDE__

#endif
