/**
 * Copyright 2023 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - why/REQ:
//   * let main thread mt_wait() other thread's input (eg from ThreadBack, MtInQueue)
//   * let other thread mt_notify() to wakeup main thread from mt_wait()
//   . base on semaphore that is simple enough
//     . std::latch/etc need c++20, too high vs semaphore (POSIX)
//     * sem_wait() doesn't miss afterwards sem_post()
//   * support timeout to prevent missing sem_post() that may hang main thread
// - MT safety: YES (since all sem_*() are mt safety)
// - core:
//   . sem_
// ***********************************************************************************************
#pragma once

#include <atomic>
#include <chrono>
#include <semaphore.h>
#include <thread>

using namespace std;
using namespace std::chrono;

namespace RLib
{
// ***********************************************************************************************
class MT_Semaphore
{
public:
    template<typename Rep = int, typename Period = ratio<1, 1000>>
    MT_Semaphore(const duration<Rep, Period>& aTimeout = 10ms)
        : mt_stopTimer_(false)
        , timerFut_(async(launch::async, [aTimeout, this]
        {
            while (! mt_stopTimer_.load())
            {
                this_thread::sleep_for(aTimeout);
                mt_notify();
            }
        }))
    {
        sem_init(&sem_, 0, 0);  // 2nd para: intra-process; 3rd: init value
    }
    ~MT_Semaphore()
    {
        mt_stopTimer_.store(true);
        timerFut_.get();
        sem_destroy(&sem_);  // must sfter timer thread stopped
    }

    void mt_wait() { sem_wait(&sem_); }
    void mt_notify()
    {
        int count = 0;
        if (sem_getvalue(&sem_, &count) == 0)  // succ
            if (count > 0)  // >0 to avoid count overflow; >10 to avoid sem_post() is wasted by eg try_lock fail
                return;
        sem_post(&sem_);
    }

    // -------------------------------------------------------------------------------------------
private:
    sem_t sem_;

    atomic<bool> mt_stopTimer_;
    future<void> timerFut_;
};

}  // namespace
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2023-09-20  CSZ       1)create
// ***********************************************************************************************
