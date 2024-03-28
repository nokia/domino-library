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
    for (auto&& fut_backFN = fut_backFN_S_.begin(); fut_backFN != fut_backFN_S_.end();)
    {
        // safer to check valid here than in newThread(), eg bug after newThread()
        auto&& threadFut = fut_backFN->first;
        const bool threadOver = ! threadFut.valid() || (threadFut.wait_for(0s) == future_status::ready);
        HID("(ThreadBack) nHandled=" << nHandled << ", valid=" << threadFut.valid() << ", backFn=" << &(fut_backFN->second));
        if (threadOver)
        {
            fut_backFN->second(threadFut.valid() && threadFut.get());  // callback
            fut_backFN = fut_backFN_S_.erase(fut_backFN);
            ++nHandled;
        }
        else
            ++fut_backFN;
    }  // 1 loop, simple & safe
    return nHandled;
}

// ***********************************************************************************************
void ThreadBack::newThread(const MT_ThreadEntryFN& mt_aEntryFn, const ThreadBackFN& aBackFn, UniLog& oneLog)
{
    // validate
    if (! mt_aEntryFn)
    {
        ERR("(ThreadBack) mt_aEntryFn can't be empty!!!");
        return;
    }
    if (! aBackFn)
    {
        ERR("(ThreadBack) aBackFn can't be empty!!!");
        return;
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
}

// ***********************************************************************************************
StoreThreadBack ThreadBack::fut_backFN_S_;

}  // namespace
