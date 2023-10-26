/**
 * Copyright 2023 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - why/REQ:
//   * let main thread mt_timedwait() other thread's input (eg from MsgSelf, ThreadBack, MtInQueue)
//   * let other thread mt_notify() to wakeup main thread from mt_timedwait()
//   . base on semaphore that is simple enough
//     . std::latch/etc need c++20, too high vs semaphore (POSIX)
//     * sem_wait() doesn't miss afterwards sem_post()
//   * support timeout to prevent missing sem_post() that may hang main thread
// - MT safety: YES (since all mt_sem_*() are mt safety)
// - core:
//   . mt_sem_
// ***********************************************************************************************
#pragma once

#include <semaphore.h>

using namespace std;

namespace RLib
{
// ***********************************************************************************************
class MT_Semaphore
{
public:
    MT_Semaphore()  { sem_init(&mt_sem_, 0, 0); }  // 2nd para: intra-process; 3rd: init value
    ~MT_Semaphore() { sem_destroy(&mt_sem_); }
    MT_Semaphore(const MT_Semaphore&) = delete;
    MT_Semaphore& operator=(const MT_Semaphore&) = delete;

    void mt_timedwait(const size_t aSec = 0, const size_t aMsec = 10);
    void mt_notify();

    // -------------------------------------------------------------------------------------------
private:
    sem_t mt_sem_;
};

}  // namespace
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2023-09-20  CSZ       1)create
// 2023-09-21  CSZ       - timer based on ThreadBack
// 2023-10-26  CSZ       - timer based on sem_timedwait()
// ***********************************************************************************************
