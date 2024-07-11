/**
 * Copyright 2024 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - Why:
//   . alt AsyncBack by limit thread# (limited resource) but may wait more time for free thread
//   . avoid create/destroy thread, 10~100 faster than AsyncBack
//   . code more complex than AsyncBack
// ***********************************************************************************************
#pragma once

#include <atomic>
#include <condition_variable>
#include <future>
#include <mutex>
#include <deque>
#include <vector>

#include "ThreadBack.hpp"

using namespace std;

namespace RLib
{
// ***********************************************************************************************
class ThPoolBack : public ThreadBack
{
public:
    explicit ThPoolBack(size_t aMaxThread = 10);
    ~ThPoolBack();

    bool newTaskOK(const MT_TaskEntryFN&, const TaskBackFN&, UniLog& = UniLog::defaultUniLog_) override;

    void fakeNotifyAll() { condition_.notify_all(); }  // UT only

private:
    // -------------------------------------------------------------------------------------------
    vector<thread>        thPool_;
    deque<packaged_task<bool()>> taskQ_;

    mutex              mutex_;
    condition_variable condition_;
    atomic<bool>       stopAllTH_ = false;
};

}  // namespace
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2024-07-09  CSZ       1)create
// ***********************************************************************************************
