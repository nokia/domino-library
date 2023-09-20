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
#include "MtInQueue.hpp"
#include "ThreadBackViaMsgSelf.hpp"
#include "UniLog.hpp"

using namespace testing;

namespace RLib {
// ***********************************************************************************************
struct CvMainThreadTest : public Test, public UniLog
{
    CvMainThreadTest() : UniLog(UnitTest::GetInstance()->current_test_info()->name()) {}
    void SetUp() override
    {
        ASSERT_EQ(0, mtQ_.mt_size()) << "REQ: empty at beginning"  << endl;
        ASSERT_EQ(0, mtQ2_.mt_size()) << "REQ: empty at beginning"  << endl;
    }
    void TearDown() override
    {
        ASSERT_EQ(0, mtQ_.mt_size()) << "REQ: empty at end"  << endl;
        ASSERT_EQ(0, mtQ2_.mt_size()) << "REQ: empty at end"  << endl;

    }
    ~CvMainThreadTest() { GTEST_LOG_FAIL }

    // -------------------------------------------------------------------------------------------
    MtInQueue mtQ_;
    MtInQueue mtQ2_;  // simulate diff resource

    // -------------------------------------------------------------------------------------------
    shared_ptr<MsgSelf> msgSelf_ = make_shared<MsgSelf>(
        [this](const PongMainFN& aPongMainFN){ pongMainFN_ = aPongMainFN; }, uniLogName());
    PongMainFN pongMainFN_;
};

#define WITH_MSG_SELF
// ***********************************************************************************************
TEST_F(CvMainThreadTest, GOLD_integrate_MsgSelf_ThreadBack_MtInQueue)  // simulate real world
{
    set<string> cb_info;

    // setup msg handler table for mtQ_
    unordered_map<size_t, function<void(shared_ptr<void>)>> msgHdlrs;
    msgHdlrs[typeid(string).hash_code()] = [this, &cb_info](shared_ptr<void> aMsg)
    {
        msgSelf_->newMsg(
            [aMsg, &cb_info]()
            {
                EXPECT_EQ("a", *static_pointer_cast<string>(aMsg));
                cb_info.emplace("REQ: a's Q hdlr via MsgSelf");
            }
        );
    };
    msgHdlrs[typeid(int).hash_code()] = [this, &cb_info](shared_ptr<void> aMsg)
    {
        msgSelf_->newMsg(
            [aMsg, &cb_info]()
            {
                EXPECT_EQ(2, *static_pointer_cast<int>(aMsg));
                cb_info.emplace("REQ: 2's Q hdlr via MsgSelf");
            }
        );
    };

    // push
    ThreadBack::newThread(
        // entryFn
        [this]() -> bool
        {
            mtQ_.mt_push(make_shared<string>("a"));
            return true;
        },
        // backFn
        viaMsgSelf(
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
        [this]() -> bool
        {
            mtQ2_.mt_push(make_shared<int>(2));
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
    while (expect != cb_info)
    {
        // handle all done Thread
        ThreadBack::hdlFinishedThreads();

        // handle all existing in mtQ_
        for (;;)
        {
            auto elePair = mtQ_.pop();
            if (elePair.first == nullptr) break;
            msgHdlrs[elePair.second](elePair.first);
        }
        // handle all existing in mtQ2_
        for (;;)
        {
            auto elePair = mtQ2_.pop();
            if (elePair.first == nullptr) break;
            msgHdlrs[elePair.second](elePair.first);
        }

        pongMainFN_();  // handle all existing in MsgSelf
    }
}

}  // namespace
