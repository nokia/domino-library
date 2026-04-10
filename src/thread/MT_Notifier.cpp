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
    sem_post(&mt_sem_);  // EOVERFLOW harmless: sem stays MAX, wait still works
}

// ***********************************************************************************************
void MT_Notifier::timedwait(const size_t aSec, const size_t aRestNsec) noexcept
{
    static const auto s_mainTH = std::this_thread::get_id();
    assert(s_mainTH == std::this_thread::get_id() && "(Notifier) timedwait() must be called from main thread");

    timespec ts{0, 0};
    clock_gettime(CLOCK_MONOTONIC, &ts);  // clock-immune

    const auto ns = ts.tv_nsec + aRestNsec;
    ts.tv_sec += (aSec + ns / 1'000'000'000);
    ts.tv_nsec = ns % 1'000'000'000;

    for (;;)
    {
        const auto ret = sem_clockwait(&mt_sem_, CLOCK_MONOTONIC, &ts);  // clock-immune
        if (errno == ETIMEDOUT || ret == 0)  // timeout or notified(include EOVERFLOW on sem_post side)
        {
            while (sem_trywait(&mt_sem_) == 0);  // reset sem counter to avoid immediate wakeup next time
            return;
        }
        // continue for EINTR
    }
}

}  // namespace
