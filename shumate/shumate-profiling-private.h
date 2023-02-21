/*
 * Copyright (C) 2023 James Westman <james@jwestman.net>
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


#pragma once


#if SHUMATE_HAS_SYSPROF

#include <sysprof-capture.h>

#define SHUMATE_PROFILE_CURRENT_TIME SYSPROF_CAPTURE_CURRENT_TIME

typedef struct {
  gint64 begin;
  char *name;
  gboolean exited : 1;
  gboolean free_name : 1;
} ShumateProfileScope;

static inline void
shumate_profile_scope_end_with_desc (ShumateProfileScope *self,
                                     const char          *desc)
{
  if (self->exited)
    return;

  self->exited = TRUE;

  sysprof_collector_mark (self->begin,
                          SYSPROF_CAPTURE_CURRENT_TIME - self->begin,
                          "shumate",
                          self->name,
                          desc);

  if (self->free_name)
    g_free (self->name);
}

static inline void
shumate_profile_scope_end (ShumateProfileScope *self)
{
  shumate_profile_scope_end_with_desc (self, NULL);
}

G_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(ShumateProfileScope, shumate_profile_scope_end)

#define SHUMATE_PROFILE_COLLECT(begin, name, desc) \
  sysprof_collector_mark (begin, \
                          SYSPROF_CAPTURE_CURRENT_TIME - begin, \
                          "shumate", \
                          name, \
                          desc);

#define SHUMATE_PROFILE_START() \
    G_GNUC_UNUSED g_auto(ShumateProfileScope) __profile_scope = { \
      .begin = SYSPROF_CAPTURE_CURRENT_TIME, \
      .name = ((char *)__PRETTY_FUNCTION__), \
      .free_name = FALSE, \
      .exited = FALSE, \
    }

#define SHUMATE_PROFILE_START_NAMED(var_name) \
    G_GNUC_UNUSED g_auto(ShumateProfileScope) __profile_scope_##var_name = { \
      .begin = SYSPROF_CAPTURE_CURRENT_TIME, \
      .name = g_strdup_printf ("%s -- %s", __FUNCTION__, G_STRINGIFY (var_name)), \
      .free_name = TRUE, \
      .exited = FALSE, \
    }

#define SHUMATE_PROFILE_END(desc) \
    shumate_profile_scope_end_with_desc (&__profile_scope, desc)

#define SHUMATE_PROFILE_END_NAMED(var_name, desc) \
    shumate_profile_scope_end_with_desc (&__profile_scope_##var_name, desc)

#else

#define SHUMATE_PROFILE_CURRENT_TIME ((gint64) -1)

#define SHUMATE_PROFILE_COLLECT(begin, name, desc)
#define SHUMATE_PROFILE_START()
#define SHUMATE_PROFILE_START_NAMED(var_name)
#define SHUMATE_PROFILE_END(desc)
#define SHUMATE_PROFILE_END_NAMED(var_name, desc)

#endif

