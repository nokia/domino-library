/**
 * Copyright 2023 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <cerrno>
#include <time.h>

#include "MT_Semaphore.hpp"

using namespace std;

namespace RLib
{
// ***********************************************************************************************
void MT_Semaphore::mt_notify()
{
    int count = 0;
    if (sem_getvalue(&mt_sem_, &count) == 0)  // succ
        if (count > 0)  // >0 to avoid count overflow; mt_timerFut_ ensure no lost
            return;
    sem_post(&mt_sem_);
}

// ***********************************************************************************************
void MT_Semaphore::mt_wait(const size_t aSec, const size_t aMsec)
{
    timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
        return;
    ts.tv_sec += aSec;
    ts.tv_nsec += aMsec * 1000'000;

    while (sem_timedwait(&mt_sem_, &ts) == -1 && errno == EINTR)
        continue;  // restart if interrupted by handler
}

}  // namespace
