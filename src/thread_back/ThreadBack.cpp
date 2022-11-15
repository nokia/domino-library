/**
 * Copyright 2022 Nokia. All rights reserved.
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <future>
#include <iostream>
#include <memory>

#include "MainRouser.hpp"
#include "MsgSelf.hpp"
#include "ThreadBack.hpp"

namespace RLib
{
// ***********************************************************************************************
void ThreadBack::hdlFinishedThreads(UniLog& oneLog)
{
    while (allThreads_.size() > 0)
    {
        size_t nFinishedThread = mt_nFinishedThread_.exchange(0);
        if (not nFinishedThread) return;

        HID("nFinishedThread=" << nFinishedThread);
        for (auto&& it = allThreads_.begin(); it != allThreads_.end();)
        {
            if (it->first.wait_for(0s) == future_status::ready)
            {
                it->second(it->first.get());
                it = allThreads_.erase(it);
                if (--nFinishedThread == 0) break;
            }
            else ++it;
        }
    }
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
                ret = false;
            }
            ++ThreadBack::mt_nFinishedThread_;
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
