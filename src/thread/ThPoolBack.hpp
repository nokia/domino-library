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
//   . not support TOO many tasks that exceeds fut_backFN_S_/mt_nDoneFut_/.. (impossible in most/normal cases)
//   . destructor will FOREVER wait all thread finished
// - MT safe:
//   . MT_/mt_ prefix: yes
//   . others: no (must call in main thread)
// - Exception-safe: follow noexcept-declare
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
    // @brief Constructs a thread pool with exactly the specified number of threads.
    // @param aMaxThread: Exact number of threads to create (minimum 1).
    // @throws runtime_error: If any thread cannot be created, ensuring the pool matches the requested size.
    explicit ThPoolBack(size_t aMaxThread = 10) noexcept(false);
    ~ThPoolBack() noexcept;

    bool newTaskOK(const MT_TaskEntryFN&, const TaskBackFN&, UniLog& = UniLog::defaultUniLog_) noexcept override;

private:
    void clean_() noexcept;
    // -------------------------------------------------------------------------------------------
    std::vector<std::thread>  thPool_;

    std::deque<std::packaged_task<SafePtr<void>()>>  taskQ_;
    std::mutex  qMutex_;
    std::condition_variable  qCv_;

    std::atomic<bool>  mt_stopAllTH_ = false;
};

}  // namespace
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2024-07-09  CSZ       1)create
// 2025-03-21  CSZ       2)enhance to avoid hang; enable exception for safety
// ***********************************************************************************************
