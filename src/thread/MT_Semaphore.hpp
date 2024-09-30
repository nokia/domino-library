/**
 * Copyright 2023 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - why/REQ:
//   * let main thread timedwait() other's input (eg from MsgSelf, ThreadBack, MtInQueue)
//   * let other thread mt_notify() to wakeup main thread upon timedwait()
//   . base on semaphore that is simple enough
//     * condition_variable may lose notification if nobody is waiting, but semaphore never
//     . std::latch/etc need c++20, too high vs semaphore (POSIX)
//   * support timeout to prevent sleep forever
//
// - core:
//   . mt_sem_
//
// - MT safe: YES (since all mt_sem_*() are mt safety)
// - class safe: yes
// ***********************************************************************************************
#pragma once

#include <atomic>
#include <semaphore.h>

namespace rlib
{
// ***********************************************************************************************
class MT_Semaphore
{
public:
    MT_Semaphore()  noexcept { sem_init(&mt_sem_, 0, 0); }  // 2nd para: intra-process; 3rd: init value
    ~MT_Semaphore() noexcept { sem_destroy(&mt_sem_); }
    MT_Semaphore(const MT_Semaphore&) = delete;  // then compiler NOT auto-gen mv(), =() etc

    void mt_notify() noexcept;

    // - no mt_ prefix since main-thread use ONLY
    // - if more threads call it, not guarantee to wakeup all threads but only 1
    void timedwait(const size_t aSec = 0, const size_t aRestNsec = 100'000'000) noexcept;

private:
    sem_t mt_sem_;
    std::atomic_flag mt_notified_ = ATOMIC_FLAG_INIT;  // = false

    // -------------------------------------------------------------------------------------------
#ifdef IN_GTEST
public:
    void reset() noexcept  // not mt safe
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
// ***********************************************************************************************
// Q&A:
// - why not check sem_*() failure?
//   . ENOMEM: sem exceeds max# - impossible normally/mostly
//   . EINVAL: invalid para - impossible
//   . EPERM : no right - impossible normally/mostly
//   . ENOSYS: not support sem - impossible normally/mostly
//   . EBUSY : re-init sem - impossible
