/**
 * Copyright 2023 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <time.h>
#include <cassert>
#include <cerrno>
#include <thread>

#include "MT_Notifier.hpp"
#include "UniLog.hpp"

using namespace std;

namespace rlib
{
// ***********************************************************************************************
void MT_Notifier::mt_notify() noexcept
{
    {
        lock_guard lock(mt_mutex_);
        mt_posted_ = true;
    }
    pthread_cond_signal(&mt_cond_);
}

// ***********************************************************************************************
void MT_Notifier::initCond_() noexcept
{
    pthread_condattr_t cattr;
    pthread_condattr_init(&cattr);
    pthread_condattr_setclock(&cattr, CLK_);
    pthread_cond_init(&mt_cond_, &cattr);
    pthread_condattr_destroy(&cattr);
}

// ***********************************************************************************************
void MT_Notifier::timedwait(const size_t aSec, const size_t aRestNsec) noexcept
{
    static const auto s_mainTH = std::this_thread::get_id();
    assert(s_mainTH == std::this_thread::get_id() && "(Notifier) timedwait() must be called from main thread");

    timespec ts{0, 0};
    clock_gettime(CLK_, &ts);

    const auto ns = ts.tv_nsec + aRestNsec;
    ts.tv_sec += (aSec + ns / 1000'000'000);
    ts.tv_nsec = ns % 1000'000'000;

    unique_lock lock(mt_mutex_);
    while (!mt_posted_)
    {
        if (pthread_cond_timedwait(&mt_cond_, mt_mutex_.native_handle(), &ts) == ETIMEDOUT)  // timeout
            break;
    }
    mt_posted_ = false;
}

}  // namespace
