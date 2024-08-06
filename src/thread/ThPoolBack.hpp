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
//
// - Use-safe: yes with condition:
//   . not support TOO many tasks that exceeds fut_backFN_S_/nDoneTh_/.. (impossible in most/normal cases)
//   . destructor will FOREVER wait all thread finished
// - MT safe: NO (can be used in main thread only)
// - Exception-safe: NO
// ***********************************************************************************************
#pragma once

#include <atomic>
#include <condition_variable>
#include <future>
#include <mutex>
#include <deque>
#include <vector>

#include "ThreadBack.hpp"

namespace rlib
{
// ***********************************************************************************************
class ThPoolBack : public ThreadBack
{
public:
    explicit ThPoolBack(size_t aMaxThread = 10);
    ~ThPoolBack();

    bool newTaskOK(const MT_TaskEntryFN&, const TaskBackFN&, UniLog& = UniLog::defaultUniLog_) override;

private:
    // -------------------------------------------------------------------------------------------
    std::vector<std::thread>  thPool_;
    std::deque<std::packaged_task<SafePtr<void>()>>  taskQ_;

    std::mutex  mutex_;
    std::condition_variable cv_;
    std::atomic<bool>  stopAllTH_ = false;
};

}  // namespace
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2024-07-09  CSZ       1)create
// ***********************************************************************************************
