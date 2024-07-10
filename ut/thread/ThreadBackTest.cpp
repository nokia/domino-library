/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include <atomic>
#include <chrono>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <queue>
#include <thread>
#include <unistd.h>

#include "MT_PingMainTH.hpp"
#include "UniLog.hpp"

#define RLIB_UT
#include "AsyncBack.hpp"
#include "ThreadBackViaMsgSelf.hpp"
#undef RLIB_UT

#include <chrono>
using namespace testing;

namespace RLib
{
// ***********************************************************************************************
struct ThreadBackTest : public Test, public UniLog
{
    ThreadBackTest() : UniLog(UnitTest::GetInstance()->current_test_info()->name())
    {
        EXPECT_EQ(0, asyncBack_.nThread()) << "REQ: clear env";
    }

    ~ThreadBackTest()
    {
        EXPECT_EQ(0, asyncBack_.nThread()) << "REQ: handle all";
        GTEST_LOG_FAIL
    }

    // -------------------------------------------------------------------------------------------
    AsyncBack asyncBack_;
    shared_ptr<MsgSelf> msgSelf_ = make_shared<MsgSelf>(uniLogName());
};

#define THREAD_AND_BACK
// ***********************************************************************************************
//                                  [main thread]
//                                       |
//                                       | std::async()    [new thread]
//               ThreadBack::newTaskOK() |--------------------->| MT_TaskEntryFN()
//   [future, TaskBackFN]->fut_backFN_S_ |                      |
//                                       |                      |
//                                       |                      |
//                                       |<.....................| future<>
//        ThreadBack::hdlFinishedTasks() |
//                                       |
TEST_F(ThreadBackTest, GOLD_entryFn_inNewThread_thenBackFn_inMainThread_withTimedWait)
{
    EXPECT_TRUE(ThreadBack::inMyMainTH()) << "REQ: OK in main thread";
    EXPECT_TRUE(asyncBack_.newTaskOK(
        // MT_TaskEntryFN
        [this]() -> bool
        {
            EXPECT_FALSE(ThreadBack::inMyMainTH()) << "REQ: in new thread";
            return true;
        },
        // TaskBackFN
        [this](bool)
        {
            EXPECT_TRUE(ThreadBack::inMyMainTH()) << "REQ: in main thread";
        }
    ));

    while (true)
    {
        if (asyncBack_.hdlFinishedTasks() == 0)
        {
            INF("new thread not end yet...");
            timedwait();  // REQ: timedwait() is more efficient than keep hdlFinishedTasks()
            continue;
        }
        return;
    }
}
TEST_F(ThreadBackTest, GOLD_entryFnResult_toBackFn_withoutTimedWait)
{
    const size_t maxThread = 2;  // req: test entryFn result true(succ) / false(fail)
    for (size_t idxThread = 0; idxThread < maxThread; ++idxThread)
    {
        SCOPED_TRACE(idxThread);
        EXPECT_TRUE(asyncBack_.newTaskOK(
            // MT_TaskEntryFN
            [idxThread]() -> bool
            {
                return idxThread % 2 != 0;  // ret true / false
            },
            // TaskBackFN
            [idxThread](bool aRet)
            {
                EXPECT_EQ(idxThread % 2 != 0, aRet) << "REQ: check true & false";
            }
        ));
    }

    // REQ: no timedwait() but keep asking
    for (size_t nHandled = 0; nHandled < maxThread; nHandled += asyncBack_.hdlFinishedTasks())
    {
        INF("nHandled=" << nHandled);
    }
}
TEST_F(ThreadBackTest, canHandle_someThreadDone_whileOtherRunning)
{
    atomic<bool> canEnd(false);
    asyncBack_.newTaskOK(
        // MT_TaskEntryFN
        [&canEnd]() -> bool
        {
            while (not canEnd)
                this_thread::yield();  // not end until instruction
            return true;
        },
        // TaskBackFN
        [](bool) {}
    );

    asyncBack_.newTaskOK(
        // MT_TaskEntryFN
        []() -> bool
        {
            return false;  // quick end
        },
        // TaskBackFN
        [](bool) {}
    );

    while (asyncBack_.hdlFinishedTasks() == 0)
    {
        INF("both threads not end yet, wait...");
        this_thread::yield();
    }

    canEnd = true;  // 1st thread keep running while 2nd is done
    while (asyncBack_.hdlFinishedTasks() == 0)  // no timedwait() so keep occupy cpu
    {
        INF("2nd thread done, wait 1st done...")
        this_thread::yield();
    }
}

#define WAIT_NOTIFY
// ***********************************************************************************************
TEST_F(ThreadBackTest, GOLD_entryFn_notify_insteadof_timeout)
{
    auto start = high_resolution_clock::now();
    asyncBack_.newTaskOK(
        [] { return true; },  // entryFn
        [](bool) {}  // backFn
    );
    timedwait(0, 500'000'000);  // long timer to ensure thread done beforehand
    auto dur = duration_cast<std::chrono::milliseconds>(high_resolution_clock::now() - start);
    EXPECT_LT(dur.count(), 500) << "REQ: entryFn end shall notify g_semToMainTH instead of timeout";

    while (asyncBack_.hdlFinishedTasks() == 0);  // clear all threads
}

#define ABNORMAL
// ***********************************************************************************************
TEST_F(ThreadBackTest, asyncFail_toBackFnWithFalse)
{
    asyncBack_.invalidNewThread(
        [](bool aRet)
        {
            EXPECT_FALSE(aRet) << "REQ: async failed -> ret=false always";
        }
    );
    asyncBack_.hdlFinishedTasks();
}
TEST_F(ThreadBackTest, emptyThreadList_ok)
{
    size_t nHandled = asyncBack_.hdlFinishedTasks();
    EXPECT_EQ(0u, nHandled);
}
TEST_F(ThreadBackTest, invalid_msgSelf_entryFN_backFN)
{
    EXPECT_FALSE(asyncBack_.newTaskOK(
        [] { return true; },  // entryFn
        viaMsgSelf([](bool) {}, nullptr)  // invalid since msgSelf==nullptr
    ));
    EXPECT_FALSE(asyncBack_.newTaskOK(
        [] { return true; },  // entryFn
        viaMsgSelf(nullptr, msgSelf_)  // invalid since backFn==nullptr
    ));
    EXPECT_FALSE(asyncBack_.newTaskOK(
        MT_TaskEntryFN(nullptr),  // invalid since entryFn==nullptr
        [](bool) {}  // backFn
    ));
    EXPECT_EQ(0, asyncBack_.nThread());
}

}  // namespace
