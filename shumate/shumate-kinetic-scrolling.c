/*
 * Copyright (C) 2014 Lieven van der Heide
 * Copyright (C) 2021 Georges Basile Stavracas Neto <georges.stavracas@gmail.com>
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

#include "shumate-kinetic-scrolling-private.h"

#include <math.h>
#include <stdio.h>

/*
 * All our curves are second degree linear differential equations, and
 * so they can always be written as linear combinations of 2 base
 * solutions. c1 and c2 are the coefficients to these two base solutions,
 * and are computed from the initial position and velocity.
 *
 * In the case of simple deceleration, the differential equation is
 *
 *   y'' = -my'
 *
 * With m the resistance factor. For this we use the following 2
 * base solutions:
 *
 *   f1(x) = 1
 *   f2(x) = exp(-mx)
 *
 * In the case of overshoot, the differential equation is
 *
 *   y'' = -my' - ky
 *
 * With m the resistance, and k the spring stiffness constant. We let
 * k = m^2 / 4, so that the system is critically damped (ie, returns to its
 * equilibrium position as quickly as possible, without oscillating), and offset
 * the whole thing, such that the equilibrium position is at 0. This gives the
 * base solutions
 *
 *   f1(x) = exp(-mx / 2)
 *   f2(x) = t exp(-mx / 2)
*/

typedef enum {
  SHUMATE_KINETIC_SCROLLING_PHASE_DECELERATING,
  SHUMATE_KINETIC_SCROLLING_PHASE_FINISHED,
} ShumateKineticScrollingPhase;

struct _ShumateKineticScrolling
{
  ShumateKineticScrollingPhase phase;
  double lower;
  double upper;
  double overshoot_width;
  double decel_friction;
  double overshoot_friction;

  double c1;
  double c2;
  double equilibrium_position;

  double t_s;
  double position;
  double velocity;
};

static inline double
us_to_s (double t)
{
  return t / 1000000.0;
}

ShumateKineticScrolling *
shumate_kinetic_scrolling_new (double decel_friction,
                               double initial_velocity)
{
  ShumateKineticScrolling *data;

  data = g_new0 (ShumateKineticScrolling, 1);
  data->phase = SHUMATE_KINETIC_SCROLLING_PHASE_DECELERATING;
  data->decel_friction = decel_friction;
  data->c1 = initial_velocity / decel_friction;
  data->c2 = -data->c1;
  data->t_s = 0.0;
  data->position = 0.0;
  data->velocity = initial_velocity;

  return data;
}

void
shumate_kinetic_scrolling_free (ShumateKineticScrolling *kinetic)
{
  g_free (kinetic);
}

gboolean
shumate_kinetic_scrolling_tick (ShumateKineticScrolling *data,
                                double                   time_delta_us,
                                double                  *position)
{
  switch(data->phase)
    {
    case SHUMATE_KINETIC_SCROLLING_PHASE_DECELERATING:
      {
        double last_position = data->position;
        double last_time_ms = data->t_s;
        double exp_part;

        data->t_s += us_to_s (time_delta_us);

        exp_part = exp (-data->decel_friction * data->t_s);
        data->position = data->c1 + data->c2 * exp_part;
        data->velocity = -data->decel_friction * data->c2 * exp_part;

        if (fabs (data->velocity) < 1.0 ||
            (last_time_ms != 0.0 && fabs (data->position - last_position) < 1.0))
          {
            data->phase = SHUMATE_KINETIC_SCROLLING_PHASE_FINISHED;
            data->position = round (data->position);
            data->velocity = 0;
          }
        break;
      }

    case SHUMATE_KINETIC_SCROLLING_PHASE_FINISHED:
    default:
      break;
    }

  if (position)
    *position = data->position;

  return data->phase != SHUMATE_KINETIC_SCROLLING_PHASE_FINISHED;
}
