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
        future_status status;
        if ((status = it->first.wait_for(0s)) == future_status::ready)
        {
            it->second(it->first.get());
            it = allThreads_.erase(it);
            ++nHandled;
            if (--mt_nFinishedThread_ == 0) return nHandled;
        }
        else ++it;
        HID("(ThreadBack) nHandled=" << nHandled << ", status=" << static_cast<int>(status));
    }
    return nHandled;  // though mt_nFinishedThread_ may > 0, to avoid dead-loop, limit-duty
}

// ***********************************************************************************************
void ThreadBack::newThread(const MT_ThreadEntryFN& mt_aEntry, const ThreadBackFN& aBack, UniLog& oneLog)
{
    allThreads_.emplace_back(
        async(launch::async, [mt_aEntry]() -> bool
        {
            bool ret = mt_aEntry();  // can NOT throw within mt_aEntry()!!! (can use lambda to catch)
            ++ThreadBack::mt_nFinishedThread_;  // last but still before future_status::ready, careful !!!
            return ret;
        }),
        aBack
    );
}

// ***********************************************************************************************
StoreThreadBack ThreadBack::allThreads_;
atomic<size_t>  ThreadBack::mt_nFinishedThread_(0);

}  // namespace
