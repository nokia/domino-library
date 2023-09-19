/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include <future>
#include <gtest/gtest.h>
#include <queue>
#include <thread>
#include <unistd.h>
#include <unordered_map>

#include "MsgSelf.hpp"
#include "UniLog.hpp"

#define MT_IN_Q_TEST
#include "MtInQueue.hpp"
#undef MT_IN_Q_TEST

using namespace testing;

namespace RLib {
// ***********************************************************************************************
struct CvMainThreadTest : public Test, public UniLog
{
    CvMainThreadTest() : UniLog(UnitTest::GetInstance()->current_test_info()->name()) {}
    void SetUp() override
    {
        ASSERT_EQ(0, mtQ_.mt_size()) << "REQ: empty at beginning"  << endl;
    }
    void TearDown() override
    {
        ASSERT_EQ(0, mtQ_.mt_size()) << "REQ: empty at end"  << endl;

    }
    ~CvMainThreadTest() { GTEST_LOG_FAIL }

    // -------------------------------------------------------------------------------------------
    MtInQueue mtQ_;

    // -------------------------------------------------------------------------------------------
    shared_ptr<MsgSelf> msgSelf_ = make_shared<MsgSelf>(
        [this](const PongMainFN& aPongMainFN){ pongMainFN_ = aPongMainFN; }, uniLogName());
    PongMainFN pongMainFN_;
};

#define WITH_MSG_SELF
// ***********************************************************************************************
TEST_F(CvMainThreadTest, GOLD_with_MsgSelf)
{
    queue<size_t> order;

    // setup msg handler table
    unordered_map<size_t, function<void(shared_ptr<void>)>> msgHdlrs;
    msgHdlrs[typeid(string).hash_code()] = [this, &order](shared_ptr<void> aMsg)
    {
        this->msgSelf_->newMsg(
            [aMsg, &order]()
            {
                order.push(0u);
                EXPECT_EQ("a", *static_pointer_cast<string>(aMsg))  << "REQ: correct msg" << endl;
            },
            EMsgPri_LOW
        );
    };
    msgHdlrs[typeid(int).hash_code()] = [this, &order](shared_ptr<void> aMsg)
    {
        this->msgSelf_->newMsg(
            [aMsg, &order]()
            {
                order.push(1u);
                EXPECT_EQ(2, *static_pointer_cast<int>(aMsg))  << "REQ: correct msg" << endl;
            },
            EMsgPri_NORM);
    };
    msgHdlrs[typeid(float).hash_code()] = [this, &order](shared_ptr<void> aMsg)
    {
        this->msgSelf_->newMsg(
            [aMsg, &order]()
            {
                order.push(2u);
                EXPECT_NEAR(3.0, *static_pointer_cast<float>(aMsg), 0.1)  << "REQ: correct msg" << endl;
            },
            EMsgPri_HIGH);
    };

    // push
    mtQ_.mt_push(make_shared<string>("a"));
    mtQ_.mt_push(make_shared<int>(2));
    mtQ_.mt_push(make_shared<float>(3.0));

    // pop
    for (size_t i = 0; i < 3; i++)
    {
        auto elePair = mtQ_.pop();
        if (elePair.first == nullptr) break;
        msgHdlrs[elePair.second](elePair.first);
    }
    pongMainFN_();  // call MsgSelf
    EXPECT_EQ(queue<size_t>({2,1,0}), order) << "REQ: priority FIFO" << endl;
}

}  // namespace
