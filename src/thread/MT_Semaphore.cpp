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
    sem_getvalue(&mt_sem_, &count);  // impossible failed since MT_Semaphore's constructore
    if (count > 0)  // >0 to avoid count overflow; timeout is deadline
        return;
    sem_post(&mt_sem_);  // impossible failed since MT_Semaphore's constructor
}

// ***********************************************************************************************
void MT_Semaphore::timedwait(const size_t aSec, const size_t aRestNsec)
{
    timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 0;
    clock_gettime(CLOCK_REALTIME, &ts);  // impossible failed since MT_Semaphore's constructor

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

        // impossible since MT_Semaphore's constructor
        // else if (errno == EINVAL)  // avoid dead loop
        //    return;

        continue;  // restart
    }  // for
}

}  // namespace
