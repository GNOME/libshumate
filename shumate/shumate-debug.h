/*
 * Copyright (C) 2007 Collabora Ltd.
 * Copyright (C) 2007 Nokia Corporation
 * Copyright (C) 2008-2009 Pierre-Luc Beaudoin <pierre-luc@pierlux.com>
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

#ifndef __SHUMATE_DEBUG_H__
#define __SHUMATE_DEBUG_H__

#include "config.h"

#include <glib.h>

G_BEGIN_DECLS

/* Please keep this enum in sync with #keys in shumate-debug.c */
typedef enum
{
  SHUMATE_DEBUG_LOADING = 1 << 1,
  SHUMATE_DEBUG_ENGINE = 1 << 2,
  SHUMATE_DEBUG_VIEW = 1 << 3,
  SHUMATE_DEBUG_NETWORK = 1 << 4,
  SHUMATE_DEBUG_CACHE = 1 << 5,
  SHUMATE_DEBUG_SELECTION = 1 << 6,
  SHUMATE_DEBUG_MEMPHIS = 1 << 7,
  SHUMATE_DEBUG_OTHER = 1 << 8,
} ShumateDebugFlags;

gboolean shumate_debug_flag_is_set (ShumateDebugFlags flag);
void shumate_debug (ShumateDebugFlags flag,
    const gchar *format,
    ...) G_GNUC_PRINTF (2, 3);
void shumate_debug_set_flags (const gchar *flags_string);
G_END_DECLS

#endif /* __SHUMATE_DEBUG_H__ */

/* ------------------------------------ */

/* Below this point is outside the __DEBUG_H__ guard - so it can take effect
 * more than once. So you can do:
 *
 * #define DEBUG_FLAG SHUMATE_DEBUG_ONE_THING
 * #include "debug.h"
 * ...
 * DEBUG ("if we're debugging one thing");
 * ...
 * #undef DEBUG_FLAG
 * #define DEBUG_FLAG SHUMATE_DEBUG_OTHER_THING
 * #include "debug.h"
 * ...
 * DEBUG ("if we're debugging the other thing");
 * ...
 */

#ifdef DEBUG_FLAG
#ifdef ENABLE_DEBUG

#undef DEBUG
#define DEBUG(format, ...) \
  shumate_debug (DEBUG_FLAG, "%s: " format, G_STRFUNC, ## __VA_ARGS__)

#undef DEBUGGING
#define DEBUGGING shumate_debug_flag_is_set (DEBUG_FLAG)

#else /* !defined (ENABLE_DEBUG) */

#undef DEBUG
#define DEBUG(format, ...) do {} while (0)

#undef DEBUGGING
#define DEBUGGING 0

#endif /* !defined (ENABLE_DEBUG) */
#endif /* defined (DEBUG_FLAG) */
