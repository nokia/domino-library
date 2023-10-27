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
void MT_Semaphore::mt_timedwait(const size_t aSec, const size_t aRestNsec)
{
    timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
        return;
    const auto ns = ts.tv_nsec + aRestNsec;
    ts.tv_sec += (aSec + ns / 1000'000'000);
    ts.tv_nsec = ns % 1000'000'000;

    for (;;)
    {
        const auto ret = sem_timedwait(&mt_sem_, &ts);
        if (ret == 0)  // notified
            return;
        else if (errno == ETIMEDOUT)
            return;

        else if (errno == EINVAL)  // avoid die in mt_timedwait()
            return;
        continue;  // restart
    }  // for
}

}  // namespace
