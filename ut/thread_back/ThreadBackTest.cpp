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
//                      [main thread]
//                            |
//                            | std::async()    [new thread]
//    ThreadBack::newThread() |--------------------->| MT_ThreadEntryFN()
//                            |                      |
//                            |                      | allThreads_.push(ThreadBackFN, ret)
//                            |                      |
//                            |<.....................| UtMainRouser.toMainThread()
//                            |                       (idle = false)
//              (idle = true) |
// ThreadBack::hdlFinishedThreads() |
//                            |
TEST_F(ThreadBackTest, GOLD_backFn_in_mainThread)
{
    const size_t MAX_THREAD = 100;
    const auto idMainThread = this_thread::get_id();
    HID("main thread=" << idMainThread << ": start creating nThread=" << MAX_THREAD);
    for (size_t idxThread = 0; idxThread < MAX_THREAD; idxThread++)
    {
        ThreadBack::newThread(
            // MT_ThreadEntryFN
            [idxThread, idMainThread]() -> bool
            {
                EXPECT_NE(idMainThread, this_thread::get_id());  // req: new thread
                return (idxThread % 2 == 0);
            },
            // ThreadBackFN
            [idxThread, idMainThread, this](bool aRet)
            {
                EXPECT_EQ(idMainThread, this_thread::get_id());  // req: main thread !!!
                EXPECT_EQ(idxThread % 2 == 0, aRet);             // req: ThreadBackFN(MT_ThreadEntryFN());
                HID("main thread=" << idMainThread << ": 1 back() done, idxThread=" << idxThread << ", ret=" << aRet);
            }
        );
        EXPECT_GT(ThreadBack::nThread(), 0);  // req: has thread(s)
    }

    while (ThreadBack::nThread() > 0)
    {
        ThreadBack::hdlFinishedThreads();
        this_thread::yield();
    }
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
