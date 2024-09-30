/**
 * Copyright 2024 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - Why this class:
//   . support both async() & thread pool
//   . common here, special in AsyncBack or ThPoolBack
// ***********************************************************************************************
#pragma once

#include <atomic>
#include <functional>
#include <future>
#include <list>
#include <utility>

#include "MT_PingMainTH.hpp"
#include "SafePtr.hpp"
#include "UniLog.hpp"

#define THREAD_BACK  (rlib::ObjAnywhere::getObj<rlib::ThreadBack>())

namespace rlib
{
// ***********************************************************************************************
using MT_TaskEntryFN  = std::function<SafePtr<void>()>;  // ret-nullptr means failure
using TaskBackFN      = std::function<void(SafePtr<void>)>;  // MT_TaskEntryFN's ret as para
using StoreThreadBack = std::list<std::pair<std::future<SafePtr<void>>, TaskBackFN> >;  // deque is worse when rm mid

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
    size_t hdlDoneFut(UniLog& = UniLog::defaultUniLog_);

    auto nFut() { return fut_backFN_S_.size(); }

    static bool mt_inMyMainTH()
    {
        static const auto s_myMainTH = std::this_thread::get_id();
        return s_myMainTH == std::this_thread::get_id();
    }

protected:
    // -------------------------------------------------------------------------------------------
    StoreThreadBack      fut_backFN_S_;     // must save future till thread end
    std::atomic<size_t>  mt_nDoneFut_ = 0;  // improve main thread to search done thread(s)

    // -------------------------------------------------------------------------------------------
#ifdef IN_GTEST
public:
    auto& mt_nDoneFut() { return mt_nDoneFut_; }
#endif
};

}  // namespace
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2024-07-09  CSZ       1)create
// 2024-08-05  CSZ       - mt_nDoneFut_ to improve iteration of fut_backFN_S_
//                       - MT_TaskEntryFN ret SafePtr<void> instead of bool
// ***********************************************************************************************
// - why SafePtr
//   . thread can ret any type data include bool
//   . simpler than using MtQ to ship rsp back to main thread
//   . safe convert from SafePtr<void> to corresponding data
