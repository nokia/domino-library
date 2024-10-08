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
    // validate
    if (aMaxThread == 0)
    {
        WRN("!!! Why 0 thread? Force to create 1 thread for min workable.");
        aMaxThread = 1;
    }

    // create threads
    thPool_.reserve(aMaxThread);  // not construct any thread
    for (size_t i = 0; i < aMaxThread; ++i)
    {
        thPool_.emplace_back([this]
        {   // thread main()
            for (;;)
            {
                packaged_task<SafePtr<void>()> task;
                {
                    unique_lock<mutex> lock(this->mutex_);  // lock so can use any in "this"
                    this->cv_.wait(lock, [this]{ return this->mt_stopAllTH_ || !this->taskQ_.empty(); });

                    if (this->mt_stopAllTH_)
                        return;
                    if (this->taskQ_.empty())  // ut can't cov since normal wait()'s pred
                        continue;
                    task = move(this->taskQ_.front());
                    this->taskQ_.pop_front();
                }
                task();

                // no lock so can only use MT_safe part in "this"
                this->mt_nDoneFut_.fetch_add(1, std::memory_order_relaxed);  // fastest +1
                mt_pingMainTH();  // notify mainTH 1 task done
            }
        });
    }  // for
}

// ***********************************************************************************************
ThPoolBack::~ThPoolBack()
{
    mt_stopAllTH_ = true;
    cv_.notify_all();

    for (auto&& th : thPool_)
        if (th.joinable())
            th.join();

    HID("!!! discard nTask=" << taskQ_.size());
}

// ***********************************************************************************************
bool ThPoolBack::newTaskOK(const MT_TaskEntryFN& mt_aEntryFN, const TaskBackFN& aBackFN, UniLog& oneLog)
{
    // validate
    if (! ThreadBack::newTaskOK(mt_aEntryFN, aBackFN, oneLog))
        return false;

    packaged_task<SafePtr<void>()> task(mt_aEntryFN);  // packaged_task can get_future()="task result"
    fut_backFN_S_.emplace_back(task.get_future(), aBackFN);  // save future<> & aBackFN()
    {
        unique_lock<mutex> lock(mutex_);
        taskQ_.emplace_back(move(task));
    }
    cv_.notify_one();  // notify thread pool to run a new task

    return true;
}

}  // namespace
