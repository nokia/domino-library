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
        {
            thread th([this]() noexcept
            {   // thread main()
                for (;;)
                {
                    packaged_task<SafePtr<void>()> task;
                    {
                        // - lock to prevent new task/notif until my qCv_ sleep/wait-notif (ensure not loss notif)
                        // - lock to prevent other thread steal task
                        unique_lock<mutex> lock(this->qMutex_);
                        this->qCv_.wait(lock,
                            [this]() noexcept { return this->mt_stopAllTH_ || !this->taskQ_.empty(); });

                        if (this->mt_stopAllTH_)
                            return;
                        // qCv_.wait(): lock then check predicate, so no need check taskQ_.empty() here

                        task = move(this->taskQ_.front());
                        this->taskQ_.pop_front();
                    }

                    // - thread can continue when task() throw
                    // - other excepts (eg bad_alloc) are rare & hard-recover
                    try { task(); }
                    catch(...) {}  // packaged_task already saved exception in its future

                    // no lock so can only use MT_safe part in "this"
                    this->mt_nDoneFut_.fetch_add(1, std::memory_order_relaxed);  // fastest +1
                    mt_pingMainTH();  // notify mainTH 1 task done
                }
            });  // thread main()
            if (th.joinable()) {
                thPool_.emplace_back(move(th));
            }
            else {  // ut can't cover this branch
                // - rare
                // - throw is safer than just log(=hide)
                throw runtime_error("(ThPoolBack) failed to construct some thread!!!");
            }
        }  // for-loop to create threads
    } catch(...) {  // ut can't cover this branch; rare but safer
        clean_();
        throw;  // break constructor
    }
}

// ***********************************************************************************************
ThPoolBack::~ThPoolBack() noexcept
{
    clean_();
    HID("!!! discard nTask=" << taskQ_.size());
}

// ***********************************************************************************************
void ThPoolBack::clean_() noexcept
{
    mt_stopAllTH_ = true;
    qCv_.notify_all();

    for (auto&& th : thPool_)
        if (th.joinable())  // safer: if sth wrong during thread lifecycle
            th.join();  // safer: avoid terminate when destruct th
}

// ***********************************************************************************************
bool ThPoolBack::newTaskOK(const MT_TaskEntryFN& mt_aEntryFN, const TaskBackFN& aBackFN, UniLog& oneLog) noexcept
{
    // validate
    if (! ThreadBack::newTaskOK(mt_aEntryFN, aBackFN, oneLog))
        return false;

    packaged_task<SafePtr<void>()> task(mt_aEntryFN);  // packaged_task can get_future()="task result"
    fut_backFN_S_.emplace_back(task.get_future(), aBackFN);  // save future<> & aBackFN()
    {
        unique_lock<mutex> lock(qMutex_);
        taskQ_.emplace_back(move(task));
    }
    qCv_.notify_one();  // notify thread pool to run a new task

    return true;
}

}  // namespace
