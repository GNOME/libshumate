/*
 * Copyright (C) 2007 Collabora Ltd.
 * Copyright (C) 2007 Nokia Corporation
 * Copyright (C) 2008-2009 Pierre-Luc Beaudoin <pierre-luc@pierlux.com>
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

#include "config.h"

#include "shumate-debug.h"

#include <errno.h>
#include <fcntl.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef ENABLE_DEBUG

static ShumateDebugFlags flags = 0;

static GDebugKey keys[] = {
  { "Loading", SHUMATE_DEBUG_LOADING },
  { "Engine", SHUMATE_DEBUG_ENGINE },
  { "View", SHUMATE_DEBUG_VIEW },
  { "Network", SHUMATE_DEBUG_NETWORK },
  { "Cache", SHUMATE_DEBUG_CACHE },
  { "Selection", SHUMATE_DEBUG_SELECTION },
  { "Other", SHUMATE_DEBUG_OTHER },
  { 0, }
};

static void
debug_set_flags (ShumateDebugFlags new_flags)
{
  flags |= new_flags;
}


void
shumate_debug_set_flags (const gchar *flags_string)
{
  guint nkeys;

  for (nkeys = 0; keys[nkeys].value; nkeys++)
    ;

  if (flags_string)
    debug_set_flags (g_parse_debug_string (flags_string, keys, nkeys));
}


gboolean
shumate_debug_flag_is_set (ShumateDebugFlags flag)
{
  return (flag & flags) != 0;
}


void
shumate_debug (ShumateDebugFlags flag,
    const gchar *format,
    ...)
{
  if (flag & flags)
    {
      va_list args;
      va_start (args, format);
      g_logv (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, format, args);
      va_end (args);
    }
}


#else

gboolean
shumate_debug_flag_is_set (ShumateDebugFlags flag)
{
  return FALSE;
}


void
shumate_debug (ShumateDebugFlags flag, const gchar *format, ...)
{
}


void
shumate_debug_set_flags (const gchar *flags_string)
{
}


#endif /* ENABLE_DEBUG */
