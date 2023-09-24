/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <queue>
#include <thread>
#include <unistd.h>

#include "MsgSelf.hpp"
#include "UniLog.hpp"

#define THREAD_BACK_TEST
#include "ThreadBackViaMsgSelf.hpp"
#undef THREAD_BACK_TEST

using namespace testing;

namespace RLib
{
// ***********************************************************************************************
struct ThreadBackTest : public Test, public UniLog
{
    ThreadBackTest() : UniLog(UnitTest::GetInstance()->current_test_info()->name())
    {
        EXPECT_EQ(0, ThreadBack::nThread()) << "REQ: clean env" << endl;
    }

    ~ThreadBackTest()
    {
        EXPECT_EQ(0, ThreadBack::nThread()) << "REQ: handle all" << endl;
        GTEST_LOG_FAIL
    }
};

#define REQ
// ***********************************************************************************************
//                               [main thread]
//                                     |
//                                     | std::async()    [new thread]
//             ThreadBack::newThread() |--------------------->| MT_ThreadEntryFN()
// [future, ThreadBackFN]->allThreads_ |                      |
//                                     |                      |
//                                     |                      |
//                                     |<.....................| future<>
//    ThreadBack::hdlFinishedThreads() |
//                                     |
TEST_F(ThreadBackTest, GOLD_entryFn_inNewThread_thenBackFn_inMainThread)
{
    atomic<thread::id> mt_threadID(this_thread::get_id());
    DBG("main thread id=" << mt_threadID);
    ThreadBack::newThread(
        // MT_ThreadEntryFN
        [&mt_threadID]() -> bool
        {
            mt_threadID = this_thread::get_id();
            cout << "MT_ThreadEntryFN(): thread id=" << mt_threadID << endl;
            return true;
        },
        // ThreadBackFN
        [&mt_threadID, this](bool)
        {
            mt_threadID = this_thread::get_id();
            DBG("ThreadBackFN thread id=" << mt_threadID);
        }
    );

    while (this_thread::get_id() == mt_threadID)  // req: run MT_ThreadEntryFN() in new thread
    {
        DBG("new thread not start yet, wait... mt_threadID=" << mt_threadID);
        this_thread::yield();
    }

    while (ThreadBack::hdlFinishedThreads() == 0)
    {
        DBG("new thread not end yet, wait... mt_threadID=" << mt_threadID)
        this_thread::yield();
    }
    EXPECT_EQ(mt_threadID, this_thread::get_id()) << "REQ: run ThreadBackFN() in main thread afterwards" << endl;
}
TEST_F(ThreadBackTest, GOLD_entryFnResult_toBackFn)
{
    const size_t maxThread = 2;  // req: test entryFn result true(succ) / false(fail)
    for (size_t idxThread = 0; idxThread < maxThread; ++idxThread)
    {
        ThreadBack::newThread(
            // MT_ThreadEntryFN
            [idxThread]() -> bool
            {
                return idxThread % 2 != 0;  // ret true / false
            },
            // ThreadBackFN
            [idxThread](bool aRet)
            {
                EXPECT_EQ(idxThread % 2 != 0, aRet) << "REQ: check true & false" << endl;
            }
        );
    }

    // req: call all ThreadBackFN
    for (size_t nHandled = 0; nHandled < maxThread; nHandled += ThreadBack::hdlFinishedThreads())
    {
        DBG("nHandled=" << nHandled);
    }
}
TEST_F(ThreadBackTest, canHandle_someThreadDone_whileOtherRunning)
{
    atomic<bool> canEnd(false);
    ThreadBack::newThread(
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

    ThreadBack::newThread(
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
        DBG("both threads not end yet, wait...");
        this_thread::yield();
    }

    canEnd = true;  // 1st thread keep running while 2nd is done
    while (ThreadBack::hdlFinishedThreads() == 0)
    {
        DBG("2nd thread done, wait 1st done...")
        this_thread::yield();
    }
}

#define ABNORMAL
// ***********************************************************************************************
TEST_F(ThreadBackTest, asyncFail_noException_toBackFnWithFalse)
{
    ThreadBack::invalidNewThread(
        [](bool aRet)
        {
            EXPECT_FALSE(aRet) << "REQ: async failed -> ret=false always" << endl;
        }
    );
    ThreadBack::hdlFinishedThreads();
}
TEST_F(ThreadBackTest, emptyThreadList_ok)
{
    size_t nHandled = ThreadBack::hdlFinishedThreads();
    EXPECT_EQ(0u, nHandled);
}

}  // namespace
