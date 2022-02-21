/*
 * Copyright (C) 2022 James Westman <james@jwestman.net>
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

#include <glib.h>

#include "shumate-user-agent.h"
#include "shumate-version.h"


static char *user_agent = NULL;


/**
 * shumate_get_user_agent:
 *
 * Gets the user agent libshumate will use for all requests.
 *
 * This API is not thread-safe and should only be called from the main thread.
 *
 * Returns: (transfer none): the user agent
 */
const char *
shumate_get_user_agent (void)
{
  if (user_agent == NULL)
    return "libshumate/" SHUMATE_VERSION;
  else
    return user_agent;
}


/**
 * shumate_set_user_agent:
 * @new_user_agent: (nullable): the new user agent, or %NULL to reset to the default
 *
 * Sets the user agent that libshumate uses for all requests.
 *
 * This API is not thread-safe and should only be called from the main thread.
 */
void
shumate_set_user_agent (const char *new_user_agent)
{
  g_clear_pointer (&user_agent, g_free);
  user_agent = g_strdup (new_user_agent);
}
