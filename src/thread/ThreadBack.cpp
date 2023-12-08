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
    for (auto&& it = allThreads_.begin(); it != allThreads_.end();)
    {
        // safer to check valid here than in newThread(), eg bug after newThread()
        auto&& threadFut = it->first;
        const bool threadOver = ! threadFut.valid() || (threadFut.wait_for(0s) == future_status::ready);
        HID("(ThreadBack) nHandled=" << nHandled << ", valid=" << threadFut.valid() << ", backFn=" << &(it->second));
        if (threadOver)
        {
            it->second(threadFut.valid() && threadFut.get());  // callback
            it = allThreads_.erase(it);
            ++nHandled;
        }
        else
            ++it;
    }  // 1 loop, simple & safe
    return nHandled;
}

// ***********************************************************************************************
void ThreadBack::newThread(const MT_ThreadEntryFN& mt_aEntryFn, const ThreadBackFN& aBackFn, UniLog& oneLog)
{
    allThreads_.emplace_back(  // save future<> & aBackFn()
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
    HID("valid=" << allThreads_.back().first.valid() << ", backFn=" << &(allThreads_.back().second));
}

// ***********************************************************************************************
StoreThreadBack ThreadBack::allThreads_;

}  // namespace
