/**
 * Copyright 2022 Nokia. All rights reserved.
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <future>
#include <memory>

#include "MainRouser.hpp"
#include "MsgSelf.hpp"
#include "ThreadBack.hpp"

namespace RLib
{
// ***********************************************************************************************
void ThreadBack::runAllBackFn(UniLog& oneLog)
{
    unique_lock<mutex> guard(finishedLock_, try_to_lock);
    // not block main thread but retry later
    if (not guard.owns_lock())
    {
        HID("main thread=" << this_thread::get_id() << ": can't lock, try next time to avoid blocked")
        // whoelse locked must call MainRouser
        return;
    }

    // run all finished threads' ThreadBackFN()
    HID("main thread=" << this_thread::get_id() << ": nFinished=" << finishedThreads_.size()
        << ", nTotalThread_=" << nTotalThread_);
    while (not finishedThreads_.empty())
    {
        finishedThreads_.front()();
        finishedThreads_.pop();
        --nTotalThread_;
    }
}

// ***********************************************************************************************
shared_future<void> ThreadBack::newThread(ThreadEntryFN aEntry, ThreadBackFN aBack, UniLog& oneLog)
{
    ++nTotalThread_;

    return async(launch::async, [aEntry, aBack, oneLog]() mutable  // cp for safety in new thread
    {
        HID("new thread=" << this_thread::get_id() << ": start");
        const auto ret = aEntry();                                 // aEntry == nullptr: doesn't make sense

        ThreadBack::finishedLock_.lock();
        finishedThreads_.push(bind(aBack, ret));
        ThreadBack::finishedLock_.unlock();

        ObjAnywhere::get<MainRouser>(oneLog)->toMainThread(bind(&ThreadBack::runAllBackFn, oneLog));
        HID("new thread=" << this_thread::get_id() << ": end");
    });
}

// ***********************************************************************************************
void ThreadBack::reset()
{
    lock_guard<mutex> guard(finishedLock_);  // safer
    queue<FullBackFN>().swap(finishedThreads_);
    nTotalThread_ = 0;
}

// ***********************************************************************************************
queue<FullBackFN> ThreadBack::finishedThreads_;
mutex             ThreadBack::finishedLock_;
size_t            ThreadBack::nTotalThread_ = 0;

}  // namespace
