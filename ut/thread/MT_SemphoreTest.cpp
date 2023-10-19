/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include <future>
#include <gtest/gtest.h>
#include <set>
#include <thread>
#include <unistd.h>
#include <unordered_map>

#include "MsgSelf.hpp"
#include "MT_Semaphore.hpp"
#include "MtInQueue.hpp"
#include "ThreadBackViaMsgSelf.hpp"
#include "UniLog.hpp"

using namespace testing;

namespace RLib {
// ***********************************************************************************************
struct MT_SemaphoreTest : public Test, public UniLog
{
    MT_SemaphoreTest()
        : UniLog(UnitTest::GetInstance()->current_test_info()->name())
        , mtQ_([this]{ mt_waker_.mt_notify(); })
    {}
    void SetUp() override
    {
        ASSERT_EQ(0, mtQ_.mt_size()) << "REQ: empty at beginning"  << endl;
    }
    void TearDown() override
    {
        EXPECT_EQ(0, mtQ_.mt_size()) << "REQ: empty at end"  << endl;
    }
    ~MT_SemaphoreTest()
    {
        mtQ_.mt_clear();  // not impact other case
        GTEST_LOG_FAIL
    }

    // -------------------------------------------------------------------------------------------
    MT_Semaphore mt_waker_;
    MtInQueue mtQ_;

    shared_ptr<MsgSelf> msgSelf_ = make_shared<MsgSelf>(
        [this](const PongMainFN& aPongMainFN){ pongMainFN_ = aPongMainFN; }, uniLogName());
    PongMainFN pongMainFN_;
};

#define INTEGRATION
// ***********************************************************************************************
TEST_F(MT_SemaphoreTest, GOLD_integrate_MsgSelf_ThreadBack_MtInQueue)  // simulate real world
{
    set<string> cb_info;

    // setup msg handler table for mtQ_
    unordered_map<size_t, function<void(shared_ptr<void>)>> msgHdlrs;
    msgHdlrs[typeid(string).hash_code()] = [this, &cb_info](shared_ptr<void> aMsg)
    {
        msgSelf_->newMsg(  // REQ: via MsgSelf
            [aMsg, &cb_info]
            {
                EXPECT_EQ("a", *static_pointer_cast<string>(aMsg));
                cb_info.emplace("REQ: a's Q hdlr via MsgSelf");
            }
        );
    };
    msgHdlrs[typeid(int).hash_code()] = [this, &cb_info](shared_ptr<void> aMsg)
    {
        msgSelf_->newMsg(
            [aMsg, &cb_info]
            {
                EXPECT_EQ(2, *static_pointer_cast<int>(aMsg));
                cb_info.emplace("REQ: 2's Q hdlr via MsgSelf");
            }
        );
    };

    // push
    ThreadBack::newThread(
        // entryFn
        [this]
        {
            mtQ_.mt_push(make_shared<string>("a"));
            mt_waker_.mt_notify();  // REQ: can notify (or rely on sem's timeout)
            return true;
        },
        // backFn
        viaMsgSelf(  // REQ: via MsgSelf
            [this, &cb_info](bool aRet)
            {
                EXPECT_TRUE(aRet) << "entryFn succ" << endl;
                cb_info.emplace("REQ: a's backFn via MsgSelf");
            },
            msgSelf_
        )
    );
    ThreadBack::newThread(
        // entryFn
        [this]
        {
            mtQ_.mt_push(make_shared<int>(2));
            //mt_waker_.mt_notify();  // REQ: rely on sem's timeout
            return true;
        },
        // backFn
        viaMsgSelf(
            [this, &cb_info](bool aRet)
            {
                EXPECT_TRUE(aRet) << "entryFn succ" << endl;
                cb_info.emplace("REQ: 2's backFn via MsgSelf");
            },
            msgSelf_
        )
    );

    // simulate main()
    const set<string> expect = {"REQ: a's Q hdlr via MsgSelf", "REQ: a's backFn via MsgSelf",
        "REQ: 2's Q hdlr via MsgSelf", "REQ: 2's backFn via MsgSelf"};
    for (;;)
    {
        // handle all done Thread
        ThreadBack::hdlFinishedThreads();

        // handle all existing in mtQ_
        for (;;)
        {
            auto elePair = mtQ_.pop();
            if (elePair.first == nullptr) break;  // handle next eg MsgSelf queue
            msgHdlrs[elePair.second](elePair.first);
        }

        pongMainFN_();  // handle all existing in MsgSelf

        if (expect == cb_info)
            return;
        mt_waker_.mt_wait();
    }
}

// ***********************************************************************************************
TEST_F(MT_SemaphoreTest, GOLD_timer_wakeup)
{
    ThreadBack::newThread(
        []{ return true; }, // entryFn; no wakeup
        [](bool){} // backFn
    );
    mt_waker_.mt_wait();
    EXPECT_EQ(1u, ThreadBack::hdlFinishedThreads()) << "REQ: MT_Semaphore's timer shall wakeup its mt_wait()";
}

}  // namespace
