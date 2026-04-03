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

#include <atomic>
#include <semaphore.h>

namespace rlib
{
// ***********************************************************************************************
class MT_Notifier
{
public:
    MT_Notifier() noexcept { sem_init(&mt_sem_, 0, 0); }
    ~MT_Notifier() noexcept { sem_destroy(&mt_sem_); }
    MT_Notifier(const MT_Notifier&) = delete;

    void mt_notify() noexcept;

    // - no mt_ prefix since main-thread use ONLY
    // - if more threads call it, not guarantee to wakeup all threads but only 1
    void timedwait(const size_t aSec = 0, const size_t aRestNsec = 100'000'000) noexcept;

private:
    sem_t mt_sem_;
    std::atomic_flag mt_notified_ = ATOMIC_FLAG_INIT;  // prevent sem overflow; multi-notify = one post

    // -------------------------------------------------------------------------------------------
#ifdef IN_GTEST
public:
    void reset() noexcept
    {
        sem_destroy(&mt_sem_);
        sem_init(&mt_sem_, 0, 0);
        mt_notified_.clear();
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
// 2026-03-31  CSZ       2)immune to system clock changes (pthread_cond + CLOCK_MONOTONIC)
// 2026-04-03  CSZ       3)back semaphore (clock-immune on glibc2.30+, else not support)
// ***********************************************************************************************
// Q&A:
// - why semaphore instead of pthread_cond?
//   . clock-immune on most glibc, not support old glibc
//   . semaphore is faster than CV
//   . simpler API
//   . no spurious wakeup, no need for predicate
