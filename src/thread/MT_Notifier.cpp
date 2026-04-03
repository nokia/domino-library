/**
 * Copyright 2023 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <cassert>
#include <cerrno>
#include <thread>
#include <time.h>

#include "MT_Notifier.hpp"

namespace rlib
{
// ***********************************************************************************************
void MT_Notifier::mt_notify() noexcept
{
    // mt_notified_ prevents sem overflow & avoids repeatedly waking main thread
    if (!mt_notified_.test_and_set())  // only post once between timedwait calls
        sem_post(&mt_sem_);
}

// ***********************************************************************************************
// glibc 2.30+: sem_clockwait(CLOCK_MONOTONIC) — immune to clock changes
// glibc <2.30: sem_timedwait(CLOCK_REALTIME) — simpler, clock change is rare
#define GLIBC_2_30_PLUS (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 30))

void MT_Notifier::timedwait(const size_t aSec, const size_t aRestNsec) noexcept
{
    static const auto s_mainTH = std::this_thread::get_id();
    assert(s_mainTH == std::this_thread::get_id() && "(Notifier) timedwait() must be called from main thread");

    timespec ts{0, 0};
#if GLIBC_2_30_PLUS
    clock_gettime(CLOCK_MONOTONIC, &ts);  // clock-immune
#else
    clock_gettime(CLOCK_REALTIME, &ts);
#endif

    const auto ns = ts.tv_nsec + aRestNsec;
    ts.tv_sec += (aSec + ns / 1'000'000'000);
    ts.tv_nsec = ns % 1'000'000'000;

    for (;;)
    {
#if GLIBC_2_30_PLUS
        const auto ret = sem_clockwait(&mt_sem_, CLOCK_MONOTONIC, &ts);  // clock-immune
#else
        const auto ret = sem_timedwait(&mt_sem_, &ts);
#endif
        if (errno == ETIMEDOUT || ret == 0)  // timeout or notified
        {
            mt_notified_.clear();  // allow next notify to post
            return;
        }
        // continue for EINTR
    }
}

}  // namespace
