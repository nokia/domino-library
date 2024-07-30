/**
 * Copyright 2024 Nokia. All rights reserved.
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <functional>
#include <future>
#include <iostream>
#include <memory>

#include "ThreadBack.hpp"

using namespace std;

namespace RLib
{
// ***********************************************************************************************
size_t ThreadBack::hdlFinishedTasks(UniLog& oneLog)
{
    size_t nHandled = 0;
    for (auto&& fut_backFN = fut_backFN_S_.begin(); fut_backFN != fut_backFN_S_.end();)  // may limit# if too many
    {
        // - async() failure will throw exception -> terminate since compiling forbid exception
        // - valid async()'s future never invalid
        // - valid packaged_task's get_future() never invalid
        auto&& fut = fut_backFN->first;
        //HID("(ThreadBack) nHandled=" << nHandled << ", valid=" << fut.valid() << ", backFn=" << &(fut_backFN->second));
        if (fut.wait_for(0s) == future_status::ready)
        {
            fut_backFN->second(fut.get());  // callback
            fut_backFN = fut_backFN_S_.erase(fut_backFN);
            ++nHandled;
        }
        else
            ++fut_backFN;
    }  // 1 loop, simple & safe
    return nHandled;
}

// ***********************************************************************************************
bool ThreadBack::newTaskOK(const MT_TaskEntryFN& mt_aEntryFN, const TaskBackFN& aBackFN, UniLog& oneLog)
{
    if (! aBackFN)
    {
        ERR("(ThreadBack) aBackFN=null doesn't make sense!!! Why not async() directly?");
        return false;
    }
    if (! mt_aEntryFN)
    {
        ERR("(ThreadBack) NOT support mt_aEntryFN=null!!! Necessary?");
        return false;
    }
    return true;
}

}  // namespace
