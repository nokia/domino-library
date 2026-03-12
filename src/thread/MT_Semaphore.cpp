/**
 * Copyright 2023 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <time.h>  // C header
#include <cassert>
#include <cerrno>
#include <thread>

#include "MT_Semaphore.hpp"
#include "UniLog.hpp"

using namespace std;

namespace rlib
{
// ***********************************************************************************************
void MT_Semaphore::mt_notify() noexcept
{
    // - can't sem_getvalue() as NOT mt-safe
    // - mt_notified_ is to avoid sem counter overflow; & not rouse main-thread repeatedly
    if (!mt_notified_.test_and_set(std::memory_order_acquire))  // acquire: ensure sem_post visibility
        sem_post(&mt_sem_);  // impossible failed since MT_Semaphore's constructor
}

// ***********************************************************************************************
void MT_Semaphore::timedwait(const size_t aSec, const size_t aRestNsec) noexcept
{
    // assert main thread (same pattern as ThreadBack::mt_inMyMainTH())
    static const auto s_mainTH = std::this_thread::get_id();
    assert(s_mainTH == std::this_thread::get_id() && "(Sem) timedwait() must be called from main thread");

    // clamp invalid aRestNsec to prevent unexpected timeout behavior
    auto nsec = aRestNsec;
    if (nsec >= 1'000'000'000)
    {
        HID("(Sem) aRestNsec(" << nsec << ") >= 1s, adding as extra seconds");
        // no clamp — keep original arithmetic that correctly handles overflow via division
    }

    timespec ts{0, 0};
    clock_gettime(CLOCK_REALTIME, &ts);

    const auto ns = ts.tv_nsec + nsec;  // usr's duty for reasonable aRestNsec; here's duty for no crash
    ts.tv_sec += (aSec + ns / 1000'000'000);
    ts.tv_nsec = ns % 1000'000'000;

    for (;;)
    {
        const auto ret = sem_timedwait(&mt_sem_, &ts);
        if (ret == 0 || errno == ETIMEDOUT)  // notified or timeout -> wakeup to handle sth
        {
            mt_notified_.clear(std::memory_order_release);  // release: publish flag clear to other threads
            return;
        }
        else if (errno != EINTR)  // EINVAL or other unexpected error
        {
            HID("(Sem) unexpected errno=" << errno);  // SEM-3: log before break
            break;
        }

        // impossible since MT_Semaphore's constructor
        // else if (errno == EINVAL)  // avoid dead loop
        //    return;
    }  // for
}

}  // namespace
