/**
 * Copyright 2024 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - Why:
//   . support both async() & thread pool
//   . common here, special in AsyncBack or ThPoolBack
// ***********************************************************************************************
#pragma once

#include <functional>
#include <future>
#include <list>

#include "MT_PingMainTH.hpp"
#include "UniLog.hpp"

using namespace std;

namespace RLib
{
// ***********************************************************************************************
using MT_TaskEntryFN  = function<bool()>;      // succ ret true, otherwise false
using TaskBackFN      = function<void(bool)>;  // entry ret as para
using StoreThreadBack = list<pair<future<bool>, TaskBackFN> >;  // deque rm middle is worse

// ***********************************************************************************************
class ThreadBack
{
public:
    virtual ~ThreadBack() = default;

    // @brief: Schedules an asynchronous task to be executed in a background thread.
    // @param MT_TaskEntryFN: The task to be executed asynchronously.
    // @param TaskBackFN    : The callback to be invoked in the main thread upon task completion.
    // @param UniLog        : The logger to use for this operation.
    virtual bool newTaskOK(const MT_TaskEntryFN&, const TaskBackFN&, UniLog& = UniLog::defaultUniLog_) = 0;

    // @brief: Processes completed threads and invokes their callbacks.
    // @param UniLog: The logger to use for this operation.
    // @return: The number of completed tasks processed.
    size_t hdlFinishedTasks(UniLog& = UniLog::defaultUniLog_);

    auto nThread() { return fut_backFN_S_.size(); }

    static bool inMyMainTH()
    {
        static const auto s_myMainTH = this_thread::get_id();
        return s_myMainTH == this_thread::get_id();
    }

protected:
    // -------------------------------------------------------------------------------------------
    StoreThreadBack fut_backFN_S_;
};

}  // namespace
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2024-07-09  CSZ       1)create
// ***********************************************************************************************
