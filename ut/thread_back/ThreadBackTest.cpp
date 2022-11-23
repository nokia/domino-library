/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <thread>
#include <unistd.h>

#include "MsgSelf.hpp"
#include "ThreadBack.hpp"
#include "UniLog.hpp"

using namespace testing;

namespace RLib
{
// ***********************************************************************************************
struct ThreadBackTest : public Test, public UniLog
{
    ThreadBackTest() : UniLog(UnitTest::GetInstance()->current_test_info()->name())
    {
        EXPECT_EQ(0, ThreadBack::nThread());  // req: clean env
    }

    ~ThreadBackTest()
    {
        EXPECT_EQ(0, ThreadBack::nThread());  // req: handle all
        ThreadBack::reset();
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
//                            |
TEST_F(ThreadBackTest, GOLD_backFn_in_mainThread)
{
    atomic<thread::id> threadID(this_thread::get_id());
    DBG("main thread id=" << threadID);
    ThreadBack::newThread(
        // MT_ThreadEntryFN
        [&threadID]() -> bool
        {
            threadID = this_thread::get_id();
            cout << "MT_ThreadEntryFN(): thread id=" << threadID << endl;
            return true;
        },
        // ThreadBackFN
        [&threadID, this](bool)
        {
            threadID = this_thread::get_id();
            DBG("ThreadBackFN thread id=" << threadID);
        }
    );

    while (this_thread::get_id() == threadID)            // req: MT_ThreadEntryFN() in new thread
    {
        DBG("new thread not start yet, wait... threadID=" << threadID);
        this_thread::yield();
    }

    while (ThreadBack::hdlFinishedThreads() == 0)
    {
        DBG("new thread not end yet, wait... threadID=" << threadID)
        this_thread::yield();
    }
    EXPECT_EQ(threadID, this_thread::get_id());          // req: ThreadBackFN() in main thread
}
TEST_F(ThreadBackTest, entryFnRet_toBackFn)
{
    const size_t maxThread = 2000;  // req: multi & performance(2000 may timeout wait_for(0s))
    for (size_t idxThread = 0; idxThread < maxThread; ++idxThread)
    {
        ThreadBack::newThread(
            // MT_ThreadEntryFN
            [idxThread]() -> bool
            {
                return idxThread % 5 != 0;
            },
            // ThreadBackFN
            [idxThread](bool aRet)
            {
                EXPECT_EQ(idxThread % 5 != 0, aRet);  // req
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
            while (not canEnd) this_thread::yield();  // not end until instruction
            return true;
        },
        // ThreadBackFN
        [](bool)
        {
        }
    );

    ThreadBack::newThread(
        // MT_ThreadEntryFN
        []() -> bool
        {
            return false;  // quick end
        },
        // ThreadBackFN
        [](bool)
        {
        }
    );

    while (ThreadBack::hdlFinishedThreads() == 0)
    {
        DBG("both threads not end yet, wait...");
        this_thread::yield();
    }

    canEnd = true;  // 1st thread keep running till now
    while (ThreadBack::hdlFinishedThreads() == 0)
    {
        DBG("2nd thread done, wait 1st done...")
        this_thread::yield();
    }
}

#define CAN_WITH_MSGSELF
// ***********************************************************************************************
TEST_F(ThreadBackTest, canWithMsgSelf)
{
    FromMainFN handleAllMsg;
    MsgSelf msgSelf([&handleAllMsg](const FromMainFN& aFromMainFN){ handleAllMsg = aFromMainFN; }, uniLogName());
    queue<size_t> order;
    for (size_t idxThread = EMsgPri_MIN; idxThread < EMsgPri_MAX; idxThread++)
    {
        ThreadBack::newThread(
            // MT_ThreadEntryFN
            []() -> bool
            {
                return false;
            },
            // ThreadBackFN
            [idxThread, &order, &msgSelf](bool)
            {
                msgSelf.newMsg([&order, idxThread](){ order.push(idxThread); }, (EMsgPriority)idxThread);  // req
            }
        );
    }

    for (size_t nHandled = 0; nHandled < EMsgPri_MAX; nHandled += ThreadBack::hdlFinishedThreads())  // all BackFN()
    {
        DBG("nHandled=" << nHandled);
    }
    handleAllMsg();
    EXPECT_EQ(queue<size_t>({2,1,0}), order);  // req: priority FIFO
}

}  // namespace
