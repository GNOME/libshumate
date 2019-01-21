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

#include "shumate/shumate-features.h"
#include "shumate/shumate-defines.h"
#include "shumate/shumate-enum-types.h"
#include "shumate/shumate-version.h"

#include "shumate/shumate-layer.h"
#include "shumate/shumate-marker-layer.h"
#include "shumate/shumate-path-layer.h"
#include "shumate/shumate-point.h"
#include "shumate/shumate-custom-marker.h"
#include "shumate/shumate-location.h"
#include "shumate/shumate-coordinate.h"
#include "shumate/shumate-marker.h"
#include "shumate/shumate-label.h"
#include "shumate/shumate-view.h"
#include "shumate/shumate-bounding-box.h"
#include "shumate/shumate-scale.h"

#include "shumate/shumate-map-source.h"
#include "shumate/shumate-tile-source.h"
#include "shumate/shumate-tile-cache.h"
#include "shumate/shumate-renderer.h"

#include "shumate/shumate-map-source-factory.h"

#include "shumate/shumate-map-source-chain.h"

#include "shumate/shumate-network-tile-source.h"
#include "shumate/shumate-network-bbox-tile-source.h"
#include "shumate/shumate-file-tile-source.h"
#include "shumate/shumate-null-tile-source.h"

#include "shumate/shumate-memory-cache.h"
#include "shumate/shumate-file-cache.h"

#include "shumate/shumate-image-renderer.h"
#include "shumate/shumate-error-tile-renderer.h"

#undef __SHUMATE_SHUMATE_H_INSIDE__

#endif
