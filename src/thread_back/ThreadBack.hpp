/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - ISSUE/what:
//   . How to coordinate main thread(containing all logic) with other threads(time-consuming tasks)
//
// - Usage/how:
//                   [main thread]
//                         |
//                         | std::async()    [new thread]
// ThreadBack::newThread() |--------------------->| MT_ThreadEntryFN()  // time-consuming task eg block to rec msg)
//                         |                      |
//          ThreadBackFN() |<.....................| (over)
//                         |
//                         |
//
// - core: ThreadBackFN
//   . after MT_ThreadEntryFN() over in "other thread", ThreadBackFN() is run in MAIN THREAD - key diff vs async()
//
// - VALUE/why:
//   * keep all logic in main thread / single thread (simple, eg no lock/deadlock, no complex logic)
//     . while time-consuming tasks in other threads
//   . simple & natural
//     * use std::future to msg from other thread back to main thread
//     * most info in main thread, eg allThreads_, easy handle ThreadBackFN() by diff ways
//   . cross platform
//
// - Req:
//   * ThreadBackFN() in main thread
//   . support diff MainWaker
//     . in-time callback when new thread over (by MainWaker::threadOver())
//   . not integrated with MsgSelf
//     . aovid complicate MsgSelf which include back to main(), priority FIFO, withdraw msg
//     . avoid block main thread
//     . avoid complicate ThreadBack (focus on 1 think: ThreadBackFN in main thread)
//   . can work with MsgSelf
//
// - support exception: NO, since
//   . dom lib branches inc 77% (Dec,2022), unnecessary complex
//
// - support multi-thread
//   . MT_/mt_ prefix: yes
//   . others: NO (only use in main thread - most dom lib code shall in main thread - simple & easy)
//
// - Q&A:
//   . MT_/mt_ prefix
//     . MT = multi-thread: mark it to run in diff thread
//     . most func/variable are in main-thread, so w/o this prefix
//   . must save future<>
//     . otherwise the thread looks like serialized with main thread
// ***********************************************************************************************
#pragma once

#include <functional>
#include <future>
#include <list>

#include "UniLog.hpp"

using namespace std;

namespace RLib
{
// ***********************************************************************************************
using MT_ThreadEntryFN  = function<bool()>;      // succ ret true, otherwise false
using ThreadBackFN      = function<void(bool)>;  // entry ret as para
using StoreThreadBack   = list<pair<future<bool>, ThreadBackFN> >;

// ***********************************************************************************************
class ThreadBack
{
public:
    static void newThread(const MT_ThreadEntryFN&, const ThreadBackFN&, UniLog& = UniLog::defaultUniLog());
    static size_t hdlFinishedThreads(UniLog& = UniLog::defaultUniLog());

    static size_t nFinishedThread() { return mt_nFinishedThread_; }
    static size_t nThread() { return allThreads_.size(); }

private:
    // -------------------------------------------------------------------------------------------
    static StoreThreadBack allThreads_;
    static atomic<size_t>  mt_nFinishedThread_;
};

}  // namespace
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2022-10-23  CSZ       1)create
// 2022-11-15  CSZ       - simpler & MT safe
// ***********************************************************************************************
