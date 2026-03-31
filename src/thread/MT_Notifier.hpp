/**
 * Copyright 2023 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - why/REQ:
//   . main thread timedwait() for worker/other threads' mt_notify()
//     * no lost notif
//   * timeout to prevent sleep forever
//   * immune to system clock changes
//
// - MT safe: YES
// - class safe: yes
// ***********************************************************************************************
#pragma once

#include <mutex>
#include <pthread.h>
#include <time.h>

namespace rlib
{
// ***********************************************************************************************
class MT_Notifier
{
public:
    MT_Notifier() noexcept { initCond_(); }
    void initCond_() noexcept;
    ~MT_Notifier() noexcept { pthread_cond_destroy(&mt_cond_); }
    MT_Notifier(const MT_Notifier&) = delete;

    void mt_notify() noexcept;

    // - no mt_ prefix since main-thread use ONLY
    // - if more threads call it, not guarantee to wakeup all threads but only 1
    void timedwait(const size_t aSec = 0, const size_t aRestNsec = 100'000'000) noexcept;

private:
    static constexpr clockid_t CLK_ = CLOCK_MONOTONIC;  // immune to system clock changes!!!

    std::mutex mt_mutex_;
    pthread_cond_t mt_cond_;  // pthread.. for CLOCK_MONOTONIC before glibc 2.30
    bool mt_posted_ = false;  // prevent lost notifications (eg no thread waiting)

    // -------------------------------------------------------------------------------------------
#ifdef IN_GTEST
public:
    void reset() noexcept
    {
        pthread_cond_destroy(&mt_cond_);
        initCond_();
        mt_posted_ = false;
    }
#endif
};

}  // namespace
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2023-09-20  CSZ       1)create
// 2023-09-21  CSZ       - timer based on ThreadBack
// 2023-10-26  CSZ       - timer based on sem_timedwait()
// 2024-07-04  CSZ       - sem_getvalue() is not MT safe, replaced
// 2026-03-31  CSZ       2)immune to system clock changes
// ***********************************************************************************************
// Q&A:
// - why pthread_cond + CLOCK_MONOTONIC instead of sem_timedwait?
//   . sem_timedwait uses CLOCK_REALTIME; system clock changes can cause hang or busy-spin
//   . pthread_condattr_setclock(CLOCK_MONOTONIC) is immune to clock adjustments
//   . predicate (mt_posted_) prevents lost notifications — same guarantee as semaphore
// - why not sem_clockwait(CLOCK_MONOTONIC)?
//   . requires glibc >= 2.34; not available on older platforms (eg RHEL8/glibc 2.28)
// - why not std::condition_variable?
//   . can't set CLOCK_MONOTONIC on glibc < 2.30 (no pthread_cond_clockwait)
// - why std::mutex with raw pthread_cond_t?
//   . std::mutex gives RAII; native_handle() provides pthread_mutex_t* for pthread_cond_timedwait
