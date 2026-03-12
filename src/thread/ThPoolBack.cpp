/**
 * Copyright 2024 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <utility>
#include "ThPoolBack.hpp"

using namespace std;

namespace rlib
{
// ***********************************************************************************************
ThPoolBack::ThPoolBack(size_t aMaxThread)
{
    try {
        // validate
        if (aMaxThread == 0)
        {
            WRN("!!! Force to create 1 thread for min workable. Safe since > usr req.");
            aMaxThread = 1;
        }

        // create threads
        thPool_.reserve(aMaxThread);  // not construct any thread
        for (size_t i = 0; i < aMaxThread; ++i)
            thPool_.emplace_back(&ThPoolBack::mt_threadMain_, this);
    } catch(...) {  // ut can't cover this branch; rare but safer
        clean_();
        throw;  // break constructor
    }
}

// ***********************************************************************************************
void ThPoolBack::mt_threadMain_() noexcept
{
    for (;;)
    {
        packaged_task<SafePtr<void>()> task;
        {
            // - lock to prevent new task/notif until my mt_qCv_ sleep/wait-notif (ensure not loss notif)
            // - lock to prevent other thread steal task
            unique_lock<mutex> lock(mt_mutex_);
            mt_qCv_.wait(lock,
                [this]() noexcept { return mt_stopAllTH_ || !mt_taskQ_.empty(); });

            if (mt_stopAllTH_)
                return;
            // mt_qCv_.wait(): lock then check predicate, so no need check mt_taskQ_.empty() here

            task = move(mt_taskQ_.front());
            mt_taskQ_.pop_front();
        }

        // - thread can continue when task() throw
        // - other excepts (eg bad_alloc) are rare & hard-recover
        task();  // packaged_task saves exception in its future

        // no lock so can only use MT_safe part in "this"
        mt_nDoneFut_.fetch_add(1, std::memory_order_release);
        mt_pingMainTH();  // always ping, or may wait long under low load
    }
}

// ***********************************************************************************************
ThPoolBack::~ThPoolBack() noexcept
{
    clean_();
}

// ***********************************************************************************************
void ThPoolBack::clean_() noexcept
{
    mt_stopAllTH_ = true;
    mt_qCv_.notify_all();

    for (auto&& th : thPool_)
        if (th.joinable())
            th.join();  // safer: avoid terminate when destruct th

    // handle completed-but-unprocessed tasks before destruction
    (void)hdlDoneFut();
    if (!fut_backFN_S_.empty())
        HID("(ThPoolBack) discarding " << fut_backFN_S_.size() << " pending tasks (backFN not called)");
}

// ***********************************************************************************************
bool ThPoolBack::newTaskOK(MT_TaskEntryFN mt_aEntryFN, TaskBackFN aBackFN, UniLog& oneLog) noexcept
{
    // validate
    if (! ThreadBack::newTaskOK(mt_aEntryFN, aBackFN, oneLog))
        return false;

    packaged_task<SafePtr<void>()> task(std::move(mt_aEntryFN));  // packaged_task can get_future()="task result"
    fut_backFN_S_.emplace_back(task.get_future(), std::move(aBackFN));  // save future<> & aBackFN()
    {
        lock_guard<mutex> lock(mt_mutex_);
        mt_taskQ_.emplace_back(move(task));
    }
    mt_qCv_.notify_one();  // notify thread pool to run a new task

    return true;
}

}  // namespace
