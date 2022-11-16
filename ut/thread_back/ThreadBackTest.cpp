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
    ThreadBackTest()
        : UniLog(string("ThreadBack.") + UnitTest::GetInstance()->current_test_info()->name())
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
    ThreadBack::newThread(
        // MT_ThreadEntryFN
        [&threadID]() -> bool
        {
            threadID = this_thread::get_id();
            return true;
        },
        // ThreadBackFN
        [&threadID](bool)
        {
            threadID = this_thread::get_id();
        }
    );

    while (this_thread::get_id() == threadID) this_thread::yield();  // req: MT_ThreadEntryFN() in new thread

    while (ThreadBack::hdlFinishedThreads() == 0) this_thread::yield();
    EXPECT_EQ(threadID, this_thread::get_id());  // req: ThreadBackFN() in main thread
}
TEST_F(ThreadBackTest, entryFnRet_toBackFn)
{
    const size_t maxThread = 100;  // req: multi / performance
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
    for (size_t nHandled = 0; nHandled < maxThread; nHandled += ThreadBack::hdlFinishedThreads());
}
TEST_F(ThreadBackTest, entryFnException_falseToBackFn)
{
    ThreadBack::newThread(
        // MT_ThreadEntryFN
        []() -> bool
        {
            throw 7;
        },
        // ThreadBackFN
        [](bool aRet)
        {
            EXPECT_FALSE(aRet);
        }
    );
    while (ThreadBack::hdlFinishedThreads() == 0) this_thread::yield();
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
            return true;  // quick end
        },
        // ThreadBackFN
        [](bool)
        {
        }
    );

    HID("nThread=" << ThreadBack::nThread());
    while (ThreadBack::hdlFinishedThreads() == 0) this_thread::yield();  // req: 2nd thread done while 1st running

    HID("nThread=" << ThreadBack::nThread());
    canEnd = true;  // 1st thread keep running till now
    while (ThreadBack::hdlFinishedThreads() == 0) this_thread::yield();
}
#if 0

// ***********************************************************************************************
TEST_F(ThreadBackTest, canWithMsgSelf)
{
    FromMainFN handleAllMsgFn;
    MsgSelf msgSelf([&handleAllMsgFn](const FromMainFN& aFromMainFN){ handleAllMsgFn = aFromMainFN; }, uniLogName());

    queue<shared_future<void> > fut;
    queue<size_t> order;
    size_t nFinishedThread = 0;
    for (size_t idxThread = EMsgPri_MIN; idxThread < EMsgPri_MAX; idxThread++)
    {
        fut.push(ThreadBack::newThread(
            // MT_ThreadEntryFN
            []() -> bool
            {
                return false;
            },
            // ThreadBackFN
            [idxThread, &nFinishedThread, this, &order, &msgSelf](bool aRet)
            {
                msgSelf.newMsg([&order, idxThread](){ order.push(idxThread); }, (EMsgPriority)idxThread);
                ++nFinishedThread;
            },
            bind(&ThreadBackTest::toMainThread, this);
        ));
    }

    do
    {
        utMainRouser_->idle.test_and_set()
            ? this_thread::yield()
            : utMainRouser_->runAllBackFn_();
    } while (nFinishedThread < EMsgPri_MAX);

    handleAllMsgFn();
    EXPECT_EQ(queue<size_t>({2,1,0}), order);
}
#endif

}  // namespace
