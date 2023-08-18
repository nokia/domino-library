/**
 * Copyright 2022 Nokia. All rights reserved.
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <future>
#include <iostream>
#include <memory>

#include "ThreadBack.hpp"

namespace RLib
{
// ***********************************************************************************************
size_t ThreadBack::hdlFinishedThreads(UniLog& oneLog)
{
    size_t nHandled = 0;
    for (auto&& it = allThreads_.begin(); it != allThreads_.end();)
    {
        // safer to check valid here than newThread(), eg bug after newThread()
        const bool threadOver = (not it->first.valid()) || (it->first.wait_for(0s) == future_status::ready);
        HID("(ThreadBack) nHandled=" << nHandled << ", valid=" << it->first.valid());
        if (threadOver)
        {
            it->second(it->first.valid() && it->first.get());  // callback
            it = allThreads_.erase(it);
            ++nHandled;
            if (--mt_nFinishedThread_ == 0)  // faster return
                return nHandled;
        }
        else
            ++it;
    }
    return nHandled;  // though mt_nFinishedThread_ may > 0, to avoid dead-loop, & limit-duty
}

// ***********************************************************************************************
void ThreadBack::newThread(const MT_ThreadEntryFN& mt_aEntry, const ThreadBackFN& aBack, UniLog& oneLog)
{
    allThreads_.emplace_back(  // save future<> & aBack()
        async(launch::async, [mt_aEntry]() -> bool  // new thread to run this lambda
        {
            bool ret = mt_aEntry();  // mt_aEntry() shall NOT throw exception!!! (can use lambda to catch)
            ++mt_nFinishedThread_;   // still before future_status::ready, careful !!!
            return ret;
        }),
        aBack
    );
}

// ***********************************************************************************************
StoreThreadBack ThreadBack::allThreads_;
atomic<size_t>  ThreadBack::mt_nFinishedThread_(0);

}  // namespace
