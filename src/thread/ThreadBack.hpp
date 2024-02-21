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
//
// - REQ:
//   * run MT_ThreadEntryFN() in new thread by ThreadBack::newThread()
//   * run ThreadBackFN() in main thread after MT_ThreadEntryFN() done by ThreadBack::hdlFinishedThreads()
//   * MT_ThreadEntryFN()'s result as ThreadBackFN()'s para so succ(true) or fail(false)
//   . eg newThread() must be called in main thread (or at least 1 same thread), no defense code to protect
//   . MT_ThreadEntryFN & ThreadBackFN shall NOT throw exception
//     . they can try-catch all exception & leave RLib simple/focus
//     . exception is bug to be fixed than pretected
//   . cross platform:
//     . std::async
//     . std::future (to msg from other thread back to main thread)
//   . most info in main thread, eg allThreads_, easy handle ThreadBackFN() by diff ways
//   * align with MsgSelf since all "msg" shall queue in MsgSelf
//     . aovid complex MsgSelf: ThreadBack provides 1 func to fill aBack into MsgSelf
//     . avoid complex ThreadBack (viaMsgSelf() in new hpp)
//     . avoid block main thread
//   . limit thread#?
//     . eg linsee's /proc/sys/kernel/threads-max=154w, cloud=35w
//     . /proc/sys/kernel/threads-max is for each process
//     . so no necessary to limit in ThreadBack
//
// - support multi-thread
//   . MT_/mt_ prefix: yes
//   . others: NO (only use in main thread - most dom lib code shall in main thread - simple & easy)
//
// - mem safe: yes with limit
//   . no duty to any unsafe behavior of MT_ThreadEntryFN or ThreadBackFN
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
    static void newThread(const MT_ThreadEntryFN&, const ThreadBackFN&, UniLog& = *UniLog::defaultUniLog());
    static size_t hdlFinishedThreads(UniLog& = *UniLog::defaultUniLog());

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
// 2023-10-25  CSZ       - can mt_pingMainTH() at the end of a thread
// 2023-08-17  CSZ       - handle async() fail
// 2023-08-18  CSZ       - rm mt_nFinishedThread_
// 2023-09-14  CSZ       2)align with MsgSelf
// 2023-10-25  CSZ       - with semaphore's wait-notify
// ***********************************************************************************************
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
//       . can reduce about half iterations with 15 more LOC
//       . atomic is lightweight & fast than mutex
//       * since little benefit, decide rm it
