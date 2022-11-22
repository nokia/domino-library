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
    for (auto nFinished = mt_nFinishedThread_.load(); nFinished > 0; nFinished = mt_nFinishedThread_.load())
    {
        for (auto&& it = allThreads_.begin(); it != allThreads_.end();)
        {
            future_status status;
            if ((status = it->first.wait_for(0s)) == future_status::ready)
            {
                it->second(it->first.get());
                it = allThreads_.erase(it);
                ++nHandled;
                if (--mt_nFinishedThread_ == 0) break;
            }
            else ++it;
            HID("nFinished=" << nFinished << ", nHandled=" << nHandled << ", status=" << static_cast<int>(status));
        }
    }
    return nHandled;
}

// ***********************************************************************************************
void ThreadBack::newThread(const MT_ThreadEntryFN& mt_aEntry, const ThreadBackFN& aBack, UniLog& oneLog)
{
    allThreads_.emplace_back(
        async(launch::async, [mt_aEntry]() -> bool
        {
            bool ret = false;
            try { ret = mt_aEntry(); }
            catch (...)  // shall not hang future<>.wait_for()
            {
                cout << "!!!Fail: mt_aEntry throw exception" << endl;  // can't use UniLog that's not MT safe
            }
            ++ThreadBack::mt_nFinishedThread_;  // last but still before future_status::ready, careful !!!
            return ret;
        }),
        aBack
    );
}

// ***********************************************************************************************
void ThreadBack::reset()
{
    allThreads_.clear();
    mt_nFinishedThread_ = 0;
}

// ***********************************************************************************************
StoreThreadBack ThreadBack::allThreads_;
atomic<size_t>  ThreadBack::mt_nFinishedThread_(0);

}  // namespace
