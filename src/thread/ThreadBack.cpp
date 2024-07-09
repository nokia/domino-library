/**
 * Copyright 2022 Nokia. All rights reserved.
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <functional>
#include <future>
#include <iostream>
#include <memory>

#include "MT_PingMainTH.hpp"
#include "ThreadBack.hpp"

namespace RLib
{
// ***********************************************************************************************
size_t ThreadBack::hdlFinishedThreads(UniLog& oneLog)
{
    size_t nHandled = 0;
    for (auto&& fut_backFN = fut_backFN_S_.begin(); fut_backFN != fut_backFN_S_.end();)  // may limit# if too many
    {
        // safer to check valid here than in newThreadOK(), eg bug after newThreadOK()
        auto&& fut = fut_backFN->first;
        const bool over = ! fut.valid() || fut.wait_for(0s) == future_status::ready;
        HID("(ThreadBack) nHandled=" << nHandled << ", valid=" << fut.valid() << ", backFn=" << &(fut_backFN->second));
        if (over)
        {
            fut_backFN->second(fut.valid() && fut.get());  // callback
            fut_backFN = fut_backFN_S_.erase(fut_backFN);
            ++nHandled;
        }
        else
            ++fut_backFN;
    }  // 1 loop, simple & safe
    return nHandled;
}

// ***********************************************************************************************
bool ThreadBack::newThreadOK(const MT_ThreadEntryFN& mt_aEntryFn, const ThreadBackFN& aBackFn, UniLog& oneLog)
{
    // validate
    if (! aBackFn)
    {
        ERR("(ThreadBack) aBackFn=null doesn't make sense!!! Why not async() directly?");
        return false;
    }
    if (! mt_aEntryFn)
    {
        ERR("(ThreadBack) NOT support mt_aEntryFn=null!!! Necessary?");
        return false;
    }

    // create new thread
    fut_backFN_S_.emplace_back(  // save future<> & aBackFn()
        async(
            launch::async,
            [mt_aEntryFn]()  // must cp than ref, otherwise dead loop
            {
                const bool ret = mt_aEntryFn();
                mt_pingMainTH();
                return ret;
            }
        ),
        aBackFn
    );
    HID("(ThreadBack) valid=" << fut_backFN_S_.back().first.valid() << ", backFn=" << &(fut_backFN_S_.back().second));
    return true;
}

// ***********************************************************************************************
StoreThreadBack ThreadBack::fut_backFN_S_;

}  // namespace
