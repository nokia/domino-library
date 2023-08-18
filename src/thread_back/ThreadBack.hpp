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
// - REQ:
//   * run MT_ThreadEntryFN() in new thread by ThreadBack::newThread()
//   * run ThreadBackFN() in main thread after MT_ThreadEntryFN() done by ThreadBack::hdlFinishedThreads()
//   * MT_ThreadEntryFN()'s result as ThreadBackFN()'s para so succ(true) or fail(false)
//   . cross platform:
//     . std::async
//     . std::future (to msg from other thread back to main thread)
//   . most info in main thread, eg allThreads_, easy handle ThreadBackFN() by diff ways
//   . not integrated with MsgSelf
//     . aovid complicate MsgSelf which include back to main(), priority FIFO, withdraw msg
//     . avoid block main thread
//     . avoid complicate ThreadBack (focus on 1 think: ThreadBackFN in main thread)
//     . can work with MsgSelf
//
// - support exception: NO!!! since
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
//   . thread pool to avoid cost of creating/destroying thread?
//     . then eg future.wait_for() can't be used, may high-risk to self-impl
//     . ThreadBack is used for time-cost task, thread create/destroy should be too small to care
//   * hdlFinishedThreads()'s wait_for() each thread, may spend too long?
//     . ThreadBack is to use for time-consuming tasks
//     * should be not too many threads exist simultaneously
//     * so should not spend too long (wait till new req appears)
//     . mt_nFinishedThread_:
//       . can reduce about half iterations with ~10 more LOC
//       . atomic is lightweight & fast than mutex
//       * since little benefit, decide rm it
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
// !!! ThreadBack NOT support MT_ThreadEntryFN & ThreadBackFN throw exception!!!
using MT_ThreadEntryFN  = function<bool()>;      // succ ret true, otherwise false
using ThreadBackFN      = function<void(bool)>;  // entry ret as para
using StoreThreadBack   = list<pair<future<bool>, ThreadBackFN> >;

// ***********************************************************************************************
class ThreadBack
{
public:
    // MT_ThreadEntryFN & ThreadBackFN shall NOT throw, otherwise ThreadBack abnormal!!!
    static void newThread(const MT_ThreadEntryFN&, const ThreadBackFN&, UniLog& = UniLog::defaultUniLog());
    static size_t hdlFinishedThreads(UniLog& = UniLog::defaultUniLog());

    static size_t nThread() { return allThreads_.size(); }

private:
    // -------------------------------------------------------------------------------------------
    static StoreThreadBack allThreads_;


    // -------------------------------------------------------------------------------------------
#ifdef THREAD_BACK_TEST  // UT only
public:
    static void invalidNewThread(const ThreadBackFN& aBack)
    {
        allThreads_.emplace_back(future<bool>(), aBack);  // invalid future
    }
#endif
};

}  // namespace
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2022-10-23  CSZ       1)create
// 2022-11-15  CSZ       - simpler & MT safe
// 2023-07-12  CSZ       - copilot compare
// 2023-08-17  CSZ       - handle async() fail
// ***********************************************************************************************
