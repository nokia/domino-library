/**
 * Copyright 2023 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "MT_Semaphore.hpp"

using namespace std;

namespace RLib
{
// ***********************************************************************************************
MT_Semaphore::~MT_Semaphore()
{
    mt_stopTimer_.store(true);
    timerFut_.get();
    sem_destroy(&sem_);  // must sfter timer thread stopped
}

// ***********************************************************************************************
void MT_Semaphore::mt_notify()
{
    int count = 0;
    if (sem_getvalue(&sem_, &count) == 0)  // succ
        if (count > 0)  // >0 to avoid count overflow; >10 to avoid sem_post() is wasted by eg try_lock fail
            return;
    sem_post(&sem_);
}

}  // namespace
