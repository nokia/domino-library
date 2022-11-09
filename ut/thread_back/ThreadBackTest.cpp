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

#include "MainRouser.hpp"
#include "ThreadBack.hpp"
#include "UniLog.hpp"
#include "UtInitObjAnywhere.hpp"

using namespace testing;

namespace RLib
{
// ***********************************************************************************************
struct UtMainRouser : public MainRouser
{
    void toMainThread(const FnInMainThread& aFunc) override
    {
        runAllBackFn_ = aFunc;
        idle.clear();
    }

    // -------------------------------------------------------------------------------------------
    atomic_flag idle = ATOMIC_VAR_INIT(true);
    FnInMainThread runAllBackFn_;
};

// ***********************************************************************************************
struct ThreadBackTest : public Test, public UniLog
{
    ThreadBackTest()
        : UniLog(string("ThreadBack.") + UnitTest::GetInstance()->current_test_info()->name())
        , utInit_(uniLogName())
    {
        ObjAnywhere::set<MainRouser>(utMainRouser_, *this);

        EXPECT_TRUE(ThreadBack::empty());  // req: clean env
    }

    ~ThreadBackTest()
    {
        EXPECT_TRUE(ThreadBack::empty());  // req: handle all
        ThreadBack::reset();
        GTEST_LOG_FAIL
    }

    // -------------------------------------------------------------------------------------------
    UtInitObjAnywhere        utInit_;
    shared_ptr<UtMainRouser> utMainRouser_ = make_shared<UtMainRouser>();
};

// ***********************************************************************************************
//                      [main thread]
//                            |
//                            | std::async()    [new thread]
//    ThreadBack::newThread() |--------------------->| ThreadEntryFN()
//                            |                      |
//                            |                      | finishedThreads_.push(ThreadBackFN, ret)
//                            |                      |
//                            |<.....................| UtMainRouser.toMainThread()
//                            |                       (idle = false)
//              (idle = true) |
// ThreadBack::runAllBackFn() |
//                            |
TEST_F(ThreadBackTest, GOLD_backFn_in_mainThread)
{
    const size_t MAX_THREAD = 100;
    const auto idMainThread = this_thread::get_id();
    HID("main thread=" << idMainThread << ": start creating nThread=" << MAX_THREAD);
    queue<shared_future<void> > fut;
    size_t nFinishedThread = 0;
    for (size_t idxThread = 0; idxThread < MAX_THREAD; idxThread++)
    {
        fut.push(ThreadBack::newThread(
            // ThreadEntryFN
            [idxThread, idMainThread]() -> bool
            {
                EXPECT_NE(idMainThread, this_thread::get_id());  // req: new thread
                return (idxThread % 2 == 0);
            },
            // ThreadBackFN
            [idxThread, idMainThread, &nFinishedThread, this](bool aRet)
            {
                EXPECT_EQ(idMainThread, this_thread::get_id());  // req: main thread !!!
                EXPECT_EQ(idxThread % 2 == 0, aRet);             // req: ThreadBackFN(ThreadEntryFN());
                ++nFinishedThread;
                HID("main thread=" << idMainThread << ": 1 back() done, idxThread=" << idxThread
                    << ", ret=" << aRet << ", nFinishedThread=" << nFinishedThread);
            },
            *this
        ));
        EXPECT_FALSE(ThreadBack::empty());                      // req: has thread(s)
    }

    do
    {
        utMainRouser_->idle.test_and_set()                      // req: new thread end at MainRouser.toMainThread()
            ? this_thread::yield()                              // real world may block on wait()
            : utMainRouser_->runAllBackFn_();                   // req: ThreadBackFN & aRet available
    } while (nFinishedThread < MAX_THREAD);                     // req: loop till all thread done !!!
}

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
            // ThreadEntryFN
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
            *this
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

}  // namespace