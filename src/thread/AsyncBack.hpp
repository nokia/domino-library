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
//                    [main thread]
//                         |
//                         | std::async()    [new thread]
//  AsyncBack::newTaskOK() |--------------------->| MT_TaskEntryFN()  // time-consuming task eg block to rec msg
//                         |                      |
//            TaskBackFN() |<.....................| (over)
//                         |
//                         |
//
// - core: TaskBackFN
//   . after MT_TaskEntryFN() end in "other thread", TaskBackFN() is auto-run in MAIN THREAD - key diff async()
//
// - VALUE/why:
//   * keep all logic (most code) in main thread (single thread: simple logic, no lock/deadlock, etc)
//     . while time-consuming tasks in other threads (min MT coding)
//   . cross platform:
//     . std::async
//     . std::future (to msg from other thread back to main thread)
//
// - REQ:
//   * run MT_TaskEntryFN() in new thread by async() (common in ThreadBack, async here)
//   * when MT_TaskEntryFN() finished, auto rouse main thread to run TaskBackFN()
//   . MT_TaskEntryFN() ret T(succ)/F(fail)
//     . this ret is as input para of TaskBackFN(); if eg async() fail, this para=F also
//   * AsyncBack is for normal/most scenario, may NOK for huge threads, high throughput, etc
//   * ONLY call AsyncBack in main-thread
//
// - Use-safe: yes with condition:
//   . not support TOO many threads (used-up thread resource; impossible in most/normal cases)
//   . lower performance than eg thread pool (but simpler impl than thread pool)
//   . destructor will FOREVER wait all thread finished
// - MT safe: NO
//   * all AsyncBack func (include ThreadBack) must run in 1 thread (best in main thread)
//     . AsyncBack can call inMyMainTH() to ensure this
//     . but can all rlib func call inMyMainTH()? little benefit so giveup
//       * common sense/principle: rlib (include AsyncBack) not call inMyMainTH()
//       . inMyMainTH() for user debug - any main-thread func shall ret T if call inMyMainTH()
// - Exception-safe: NO
//   * assume no exception from any hdlr provided to rlib
//   . no duty to any unsafe behavior of MT_TaskEntryFN & TaskBackFN (eg throw exception)
//     . MT_TaskEntryFN & TaskBackFN shall NOT throw exception
//     . they can try-catch all exception & leave rlib simple/focus
// - support multi-thread
//   . MT_/mt_ prefix: yes
//   . others: NO (only use in main thread - most dom lib code shall in main thread - simple & nsesilbe)
// ***********************************************************************************************
#pragma once

#include "ThreadBack.hpp"

namespace rlib
{
// ***********************************************************************************************
class AsyncBack : public ThreadBack
{
public:
    // destruct-future will block till thread done (ut verified) - so default ~AsyncBack() is safe

    bool newTaskOK(const MT_TaskEntryFN&, const TaskBackFN&, UniLog& = UniLog::defaultUniLog_) override;
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
// 2024-07-10  CSZ       - mv common to base=ThreadBack
// ***********************************************************************************************
// - Q&A:
//   . MT_/mt_ prefix
//     . MT = multi-thread: mark it to run in diff thread
//     . most func/variable are in main-thread, so w/o this prefix
//   . must save future<>
//     . otherwise the thread looks like serialized with main thread
//   . limit thread#?
//     . eg linsee's /proc/sys/kernel/threads-max=154w, cloud=35w
//       . /proc/sys/kernel/threads-max is for each process
//       . so no necessary to limit
//     . most code in main-thread -> very little (eg 1%) in ThreadBack, shouldn't be many threads
//   . cancel thread?
//     . async not support cancel
//     . usr-self can cancel by eg flag
//   . thread pool?
//     . then eg future.wait_for() can't be used, may high-risk to self-impl
//     * thread pool costs more time than async (eg 0.3s:0.1s of 100 threads on belinb03)
//   * hdlFinishedTasks()'s wait_for() each thread, may spend too long?
//     . ThreadBack is to use for time-consuming tasks
//     * should be not too many threads exist simultaneously
//     * so should not spend too long (wait till new req appears)
//     . mt_nFinishedThread_:
//       . can reduce about half iterations with 15 more LOC
//       . atomic is lightweight & fast than mutex
//       * since little benefit, decide rm it
