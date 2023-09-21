/**
 * Copyright 2023 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - why:
//   * let main thread waits other thread's input (eg ThreadBack, MtInQueue)
//   * let other thread wakeup main thread
//   . base on semaphore
// - MT safety: YES (since all sem_*() are mt safety)
// - core:
//   . sem_
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
    MT_Semaphore()  { sem_init(&sem_, 0, 0); }  // 2nd: intra-process; 3rd: init value
    ~MT_Semaphore() { sem_destroy(&sem_); }

    void mt_wait() { sem_wait(&sem_); }
    void mt_notify()
    {
        int count = 0;
        if (sem_getvalue(&sem_, &count) == 0)  // succ
            if (count > 10)  // >0 to avoid count overflow; >10 to avoid sem_post() is wasted by eg try_lock fail
                return;
        sem_post(&sem_);
    }

    // -------------------------------------------------------------------------------------------
private:
    sem_t sem_;
};

}  // namespace
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2023-09-20  CSZ       1)create
// ***********************************************************************************************
