/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "Util.h"

#ifdef __MACH__
#include <sys/time.h>
//clock_gettime is not implemented on OSX
int clock_gettime(int /*clk_id*/, struct timespec* t) {
    struct timeval now;
#if defined(_WIN32) & defined(__MINGW32__)
    int rv = mingw_gettimeofday(&now, NULL);
#else
    int rv = gettimeofday(&now, NULL);
#endif
    if (rv) return rv;
    t->tv_sec  = now.tv_sec;
    t->tv_nsec = now.tv_usec * 1000;
    return 0;
}
#define CLOCK_REALTIME 0
#define CLOCK_MONOTONIC 0
#else
#include <time.h>
#endif

namespace apache { namespace thrift { namespace concurrency {

int64_t Util::currentTimeTicks(int64_t ticksPerSec) {
  int64_t result;

#if defined(HAVE_CLOCK_GETTIME)
  struct timespec now;
  clock_gettime(CLOCK_REALTIME, &now);
  toTicks(result, now, ticksPerSec);
#elif defined(HAVE_GETTIMEOFDAY)
  struct timeval now;
#if defined(_WIN32) & defined(__MINGW32__)
  int ret = mingw_gettimeofday(&now, NULL);
#else
  int ret = gettimeofday(&now, NULL);
#endif
  assert(ret == 0);
  toTicks(result, now, ticksPerSec);
#else
#error "No high-precision clock is available."
#endif // defined(HAVE_CLOCK_GETTIME)

  return result;
}

}}} // apache::thrift::concurrency
