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
#include "MT_PingMainTH.hpp"
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
    {}
    void SetUp() override
    {
        ASSERT_EQ(0, mtQ_.mt_size()) << "REQ: empty at beginning"  << endl;
    }
    void TearDown() override
    {
        ASSERT_EQ(0, mtQ_.mt_size()) << "REQ: empty at end"  << endl;
    }
    ~MT_SemaphoreTest() { GTEST_LOG_FAIL }

    // -------------------------------------------------------------------------------------------
    MtInQueue mtQ_;

    shared_ptr<MsgSelf> msgSelf_ = make_shared<MsgSelf>(
        [this](const PongMainFN& aPongMainFN){ pongMainFN_ = aPongMainFN; }, uniLogName());
    PongMainFN pongMainFN_ = nullptr;
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
        INF("nMsg=" << msgSelf_->nMsg() << ", nQ=" << mtQ_.mt_size() << ", nTh=" << ThreadBack::nThread());
        ThreadBack::hdlFinishedThreads();

        // handle all existing in mtQ_
        for (;;)
        {
            INF("nMsg=" << msgSelf_->nMsg() << ", nQ=" << mtQ_.mt_size() << ", nTh=" << ThreadBack::nThread());
            auto elePair = mtQ_.pop();
            if (elePair.first == nullptr) break;  // handle next eg MsgSelf queue

            INF("nMsg=" << msgSelf_->nMsg() << ", nQ=" << mtQ_.mt_size() << ", nTh=" << ThreadBack::nThread());
            msgHdlrs[elePair.second](elePair.first);
        }

        INF("nMsg=" << msgSelf_->nMsg() << ", nQ=" << mtQ_.mt_size() << ", nTh=" << ThreadBack::nThread());
        if (pongMainFN_)    // possible nullptr
            pongMainFN_();  // handle all existing in MsgSelf

        INF("nMsg=" << msgSelf_->nMsg() << ", nQ=" << mtQ_.mt_size() << ", nTh=" << ThreadBack::nThread());
        if (expect == cb_info)
            return;

        INF("nMsg=" << msgSelf_->nMsg() << ", nQ=" << mtQ_.mt_size() << ", nTh=" << ThreadBack::nThread());
        g_sem.mt_timedwait();
    }
}

// ***********************************************************************************************
TEST_F(MT_SemaphoreTest, immediate_timeout)
{
    ThreadBack::newThread(
        []{ return true; }, // entryFn; no wakeup
        [](bool aRet){ EXPECT_TRUE(aRet); } // backFn
    );
    while (ThreadBack::hdlFinishedThreads() == 0)
        g_sem.mt_timedwait(0, 0);  // REQ: can timeout immediately
}

}  // namespace
