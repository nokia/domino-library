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
        EXPECT_EQ(0, ThreadBack::nThread()) << "REQ: clear env";
    }

    ~ThreadBackTest()
    {
        EXPECT_EQ(0, ThreadBack::nThread()) << "REQ: handle all";
        GTEST_LOG_FAIL
    }

    shared_ptr<MsgSelf> msgSelf_ = make_shared<MsgSelf>(uniLogName());
};

#define THREAD_AND_BACK
// ***********************************************************************************************
//                                   [main thread]
//                                         |
//                                         | std::async()    [new thread]
//               ThreadBack::newThreadOK() |--------------------->| MT_ThreadEntryFN()
//   [future, ThreadBackFN]->fut_backFN_S_ |                      |
//                                         |                      |
//                                         |                      |
//                                         |<.....................| future<>
//        ThreadBack::hdlFinishedThreads() |
//                                         |
TEST_F(ThreadBackTest, GOLD_entryFn_inNewThread_thenBackFn_inMainThread_withTimedWait)
{
    atomic<thread::id> mt_threadID(this_thread::get_id());
    INF("main thread id=" << mt_threadID);
    EXPECT_TRUE(ThreadBack::newThreadOK(
        // MT_ThreadEntryFN
        [&mt_threadID, this]() -> bool
        {
            EXPECT_NE(this_thread::get_id(), mt_threadID) << "REQ: in new thread";
            mt_threadID = this_thread::get_id();
            HID("MT_ThreadEntryFN(): thread id=" << mt_threadID);  // REQ: MT safe
            return true;
        },
        // ThreadBackFN
        [&mt_threadID, this](bool)
        {
            EXPECT_NE(this_thread::get_id(), mt_threadID) << "REQ: in diff thread";
            mt_threadID = this_thread::get_id();
            INF("ThreadBackFN thread id=" << mt_threadID);
        }
    ));

    while (true)
    {
        if (ThreadBack::hdlFinishedThreads() == 0)
        {
            INF("new thread not end yet, wait... mt_threadID=" << mt_threadID)
            timedwait();  // REQ: timedwait() is more efficient than keep hdlFinishedThreads()
            continue;
        }
        EXPECT_EQ(mt_threadID, this_thread::get_id()) << "REQ: run ThreadBackFN() in main thread afterwards";
        return;
    }
}
TEST_F(ThreadBackTest, GOLD_entryFnResult_toBackFn_withoutTimedWait)
{
    const size_t maxThread = 2;  // req: test entryFn result true(succ) / false(fail)
    for (size_t idxThread = 0; idxThread < maxThread; ++idxThread)
    {
        SCOPED_TRACE(idxThread);
        ThreadBack::newThreadOK(
            // MT_ThreadEntryFN
            [idxThread]() -> bool
            {
                return idxThread % 2 != 0;  // ret true / false
            },
            // ThreadBackFN
            [idxThread](bool aRet)
            {
                EXPECT_EQ(idxThread % 2 != 0, aRet) << "REQ: check true & false";
            }
        );
    }

    // req: call all ThreadBackFN
    for (size_t nHandled = 0; nHandled < maxThread; nHandled += ThreadBack::hdlFinishedThreads())
    {
        INF("nHandled=" << nHandled);
    }
}
TEST_F(ThreadBackTest, canHandle_someThreadDone_whileOtherRunning)
{
    atomic<bool> canEnd(false);
    ThreadBack::newThreadOK(
        // MT_ThreadEntryFN
        [&canEnd]() -> bool
        {
            while (not canEnd)
                this_thread::yield();  // not end until instruction
            return true;
        },
        // ThreadBackFN
        [](bool) {}
    );

    ThreadBack::newThreadOK(
        // MT_ThreadEntryFN
        []() -> bool
        {
            return false;  // quick end
        },
        // ThreadBackFN
        [](bool) {}
    );

    while (ThreadBack::hdlFinishedThreads() == 0)
    {
        INF("both threads not end yet, wait...");
        this_thread::yield();
    }

    canEnd = true;  // 1st thread keep running while 2nd is done
    while (ThreadBack::hdlFinishedThreads() == 0)  // no timedwait() so keep occupy cpu
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
    ThreadBack::newThreadOK(
        [] { return true; },  // entryFn
        [](bool) {}  // backFn
    );
    timedwait(0, 500'000'000);  // long timer to ensure thread done beforehand
    auto dur = duration_cast<std::chrono::milliseconds>(high_resolution_clock::now() - start);
    EXPECT_LT(dur.count(), 500) << "REQ: entryFn end shall notify g_sem instead of timeout";

    while (ThreadBack::hdlFinishedThreads() == 0);  // clear all threads
}

#define ABNORMAL
// ***********************************************************************************************
TEST_F(ThreadBackTest, asyncFail_noException_toBackFnWithFalse)
{
    ThreadBack::invalidNewThread(
        [](bool aRet)
        {
            EXPECT_FALSE(aRet) << "REQ: async failed -> ret=false always";
        }
    );
    ThreadBack::hdlFinishedThreads();
}
TEST_F(ThreadBackTest, emptyThreadList_ok)
{
    size_t nHandled = ThreadBack::hdlFinishedThreads();
    EXPECT_EQ(0u, nHandled);
}
TEST_F(ThreadBackTest, invalid_msgSelf_entryFN_backFN)
{
    EXPECT_FALSE(ThreadBack::newThreadOK(
        [] { return true; },  // entryFn
        viaMsgSelf([](bool) {}, nullptr)  // invalid since msgSelf==nullptr
    ));
    EXPECT_FALSE(ThreadBack::newThreadOK(
        [] { return true; },  // entryFn
        viaMsgSelf(nullptr, msgSelf_)  // invalid since backFn==nullptr
    ));
    EXPECT_FALSE(ThreadBack::newThreadOK(
        MT_ThreadEntryFN(nullptr),  // invalid since entryFn==nullptr
        [](bool) {}  // backFn
    ));
    EXPECT_EQ(0, ThreadBack::nThread());
}

#define MAIN_THREAD
// ***********************************************************************************************
TEST_F(ThreadBackTest, can_debug_usr_code)
{
    EXPECT_TRUE(ThreadBack::inMyMainThread()) << "REQ: OK in main thread";

    ThreadBack::newThreadOK(
        // entryFn
        []
        {
            EXPECT_FALSE(ThreadBack::inMyMainThread()) << "REQ: NOK in other thread";
            return true;
        },
        // backFn
        [](bool) {}
    );
    while (ThreadBack::hdlFinishedThreads() == 0);  // clear all threads
}

}  // namespace
