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
void MT_Semaphore::mt_notify() noexcept
{
    // - can't sem_getvalue() as NOT mt-safe
    // - mt_notified_ is to avoid sem counter overflow; & not rouse main-thread repeatedly
    if (!mt_notified_.test_and_set())  // memory_order_seq_cst to ensure other thread(s) see the flag
        sem_post(&mt_sem_);  // impossible failed since MT_Semaphore's constructor
}

// ***********************************************************************************************
void MT_Semaphore::timedwait(const size_t aSec, const size_t aRestNsec) noexcept
{
    timespec ts{0, 0};
    clock_gettime(CLOCK_REALTIME, &ts);

    const auto ns = ts.tv_nsec + aRestNsec;  // usr's duty for reasonable aRestNsec; here's duty for no crash
    ts.tv_sec += (aSec + ns / 1000'000'000);
    ts.tv_nsec = ns % 1000'000'000;

    for (;;)
    {
        const auto ret = sem_timedwait(&mt_sem_, &ts);
        if (errno == ETIMEDOUT || ret == 0)  // timeout or notified -> wakeup to handle sth
        {
            mt_notified_.clear();  // memory_order_seq_cst to ensure other thread(s) see the flag
            return;
        }

        // impossible since MT_Semaphore's constructor
        // else if (errno == EINVAL)  // avoid dead loop
        //    return;

        continue;  // restart for EINTR
    }  // for
}

}  // namespace
