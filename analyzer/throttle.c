/*

  Copyright (C) 2017 Gonzalo José Carracedo Carballal

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program.  If not, see
  <http://www.gnu.org/licenses/>

*/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>
#include <time.h>

#define SU_LOG_DOMAIN "throttle"

#include <sigutils/sigutils.h>
#include "throttle.h"
#include "realtime.h"

void
suscan_throttle_init(suscan_throttle_t *throttle, SUSCOUNT samp_rate)
{
  memset(throttle, 0, sizeof(suscan_throttle_t));
  throttle->samp_rate = samp_rate;
  throttle->t0 = suscan_gettime_raw();

  /*
   * In some circumstances, if both calls to suscan_gettime_raw happen
   * almost simultaneously, the difference in t0 is below the clock
   * resolution, entering in a full speed read that will hog the
   * CPU. This is definitely a bug, and this a workaround.
   */
  usleep(100000);
}

SUSCOUNT
suscan_throttle_get_portion(suscan_throttle_t *throttle, SUSCOUNT h)
{
  struct timespec sleep_time;
  int64_t delay;
  uint64_t now;

  if (!h)
    return 0;

  /*
   * At this point, we are (now-t0) after the last return and we processed
   * samp_count samples.  We can calculate when this return should occur,
   * and sleep as necessary.
   */

  now = suscan_gettime_raw();
  delay = ((throttle->samp_count + h) * 1000000000ull / throttle->samp_rate) - (now - throttle->t0);

  if (delay > 500) {
    sleep_time.tv_sec = delay / 1000000000ull;
    sleep_time.tv_nsec = delay % 1000000000ull;

    nanosleep(&sleep_time, NULL);
  }

  /* Check to avoid slow readers to overflow the available counter */
  if (throttle->samp_count > SUSCAN_THROTTLE_RESET_THRESHOLD) {
    throttle->samp_count = 0;
    throttle->t0 = now;
  }

  return h;
}

void
suscan_throttle_advance(suscan_throttle_t *throttle, SUSCOUNT got)
{
  throttle->samp_count += got;
}
