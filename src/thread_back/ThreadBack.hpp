/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - ISSUE/what:
//   . How to coordinate main thread(containing all logic) with other thread(time-consuming task)
// - Usage/how:
//                   [main thread]
//                         |
//                         | std::async()    [new thread]
// ThreadBack::newThread() |--------------------->| ThreadEntryFN()  // time-consuming task eg block to rec msg)
//                         |                      |
//          ThreadBackFN() |<.....................| (over)
//                         |
//                         |
// - core: ThreadBackFN
//   . after ThreadEntryFN() over in "other thread", ThreadBackFN() is run in MAIN THREAD - key diff std::async()
// - VALUE/why:
//   * keep all logic in main thread / single thread (simple, eg no lock/deadlock, no complex logic)
//     . while time-consuming tasks in other threads
//   . simple impl
//   . cross platform (base on MainWaker, diff impl eg UtMainWaker, SwmMainWaker)
//   . UtMainRouser is example to real world
// - Req:
//   * ThreadBackFN() in main thread
//   . support diff MainWaker
//     . in-time callback when new thread over (by MainWaker::threadOver())
//   . not integrated with MsgSelf
//     . aovid complicate MsgSelf which include back to main(), priority FIFO, withdraw msg
//     . avoid block main thread
//     . avoid complicate ThreadBack (focus on 1 think: ThreadBackFN in main thread)
//   . can work with MsgSelf
// - Note:
//   . why MainRouser to provide toMainThread()?
//     . another way is as para of ThreadBack::newThread() - complex to provide for each thread
//     . real world may implement MainRouser::toMain() then para to MsgSelf()
// ***********************************************************************************************
#pragma once

#include <functional>
#include <future>
#include <queue>
#include <mutex>

#include "MsgSelf.hpp"
#include "UniLog.hpp"

using namespace std;

namespace RLib
{
// ***********************************************************************************************
using ThreadEntryFN = function<bool()>;
using ThreadBackFN  = function<void(bool)>;
using FullBackFN    = function<void()>;

// ***********************************************************************************************
class ThreadBack
{
public:
    // !!!must return future<>, otherwise the thread looks like serialized with main thread
    static shared_future<void> newThread(ThreadEntryFN, ThreadBackFN, UNI_LOG);

    static void reset();  // ut clean for next case
    static bool empty() { return nTotalThread_ == 0; }

private:
     static void runAllBackFn(UniLog&);

   // -------------------------------------------------------------------------------------------
    static queue<FullBackFN> finishedThreads_;
    static mutex             finishedLock_;
    static size_t            nTotalThread_;
};

}  // namespace
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2022-10-23  CSZ       1)create
// ***********************************************************************************************
