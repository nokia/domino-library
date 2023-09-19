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
struct MtInQueueTest : public Test, public UniLog
{
    MtInQueueTest() : UniLog(UnitTest::GetInstance()->current_test_info()->name()) {}
    void SetUp() override
    {
        ASSERT_EQ(0, mtQ_.mt_size()) << "REQ: empty at beginning"  << endl;
    }
    void TearDown() override
    {
        ASSERT_EQ(0, mtQ_.mt_size()) << "REQ: empty at end"  << endl;

    }
    ~MtInQueueTest() { GTEST_LOG_FAIL }

    // -------------------------------------------------------------------------------------------
    MtInQueue mtQ_;

    // -------------------------------------------------------------------------------------------
    shared_ptr<MsgSelf> msgSelf_ = make_shared<MsgSelf>(
        [this](const PongMainFN& aPongMainFN){ pongMainFN_ = aPongMainFN; }, uniLogName());
    PongMainFN pongMainFN_;
};

#define FIFO
// ***********************************************************************************************
TEST_F(MtInQueueTest, GOLD_fifo_multiThreadSafe)
{
    const int nMsg = 10000;
    auto push_thread = async(launch::async, [&nMsg, this]()
    {
        for (int i = 0; i < nMsg; i++)
        {
            this->mtQ_.mt_push(make_shared<int>(i));
            this_thread::sleep_for(1us);  // simulate real world (low-frequent msg)
        }
    });

    int nHdl = 0;
    while (nHdl < nMsg)
    {
        auto msg = mtQ_.pop<int>();
        if (msg) ASSERT_EQ(nHdl++, *msg) << "REQ: fifo";
        else mtQ_.wait();  // REQ: less CPU than repeat pop() or this_thread::yield()
    }
    INF("REQ(sleep 1us/push): e2e user=0.371s->0.148s, sys=0.402s->0.197s")
}
TEST_F(MtInQueueTest, GOLD_surgePush_performance)
{
    const int nMsg = 10000;
    auto push_thread = async(launch::async, [&nMsg, this]()
    {
        for (int i = 0; i < nMsg; i++)
        {
            this->mtQ_.mt_push(make_shared<int>(i));  // surge push
        }
    });

    int nHdl = 0;
    INF("before loop")
    while (nHdl < nMsg)
    {
        auto msg = mtQ_.pop<int>();
        if (msg) ASSERT_EQ(nHdl++, *msg) << "REQ: fifo";
        else this_thread::yield();  // REQ: test cache_ performance
    }
    INF("REQ: loop cost 2576us now, previously no cache & lock cost 4422us")
}
TEST_F(MtInQueueTest, GOLD_nonBlock_pop)
{
    ASSERT_FALSE(mtQ_.pop<void>()) << "REQ: can pop empty" << endl;

    mtQ_.mt_push(make_shared<string>("1st"));
    mtQ_.mt_push(make_shared<string>("2nd"));
    mtQ_.backdoor().lock();
    ASSERT_FALSE(mtQ_.pop<void>()) << "REQ: not blocked" << endl;

    mtQ_.backdoor().unlock();
    ASSERT_EQ("1st", *(mtQ_.pop<string>())) << "REQ: can pop";

    mtQ_.backdoor().lock();
    ASSERT_EQ("2nd", *(mtQ_.pop<string>())) << "REQ: can pop from cache" << endl;
    mtQ_.backdoor().unlock();
}
TEST_F(MtInQueueTest, size_and_wait)
{
    mtQ_.mt_push<int>(make_shared<int>(1));
    ASSERT_EQ(1u, mtQ_.mt_size())  << "REQ: inc size"  << endl;

    mtQ_.mt_push<int>(make_shared<int>(2));
    ASSERT_EQ(2u, mtQ_.mt_size())  << "REQ: inc size"  << endl;

    EXPECT_EQ(1, *(mtQ_.pop<int>())) << "REQ: fifo";
    ASSERT_EQ(1u, mtQ_.mt_size())  << "REQ: dec size"  << endl;

    mtQ_.mt_push<int>(make_shared<int>(3));
    mtQ_.wait();
    ASSERT_EQ(2u, mtQ_.mt_size())  << "REQ: wait() ret immediately since cache_ not empty"  << endl;

    EXPECT_EQ(2, *(mtQ_.pop<int>())) << "REQ: keep fifo after wait()";
    EXPECT_EQ(3, *(mtQ_.pop<int>())) << "REQ: keep fifo after wait()";
    ASSERT_EQ(0u, mtQ_.mt_size())  << "REQ: dec size"  << endl;
}

#define DESTRUCT
// ***********************************************************************************************
struct TestObj
{
    bool& isDestructed_;
    explicit TestObj(bool& aExtFlag) : isDestructed_(aExtFlag) { isDestructed_ = false; }
    ~TestObj() { isDestructed_ = true; }
};
TEST_F(MtInQueueTest, GOLD_destructCorrectly)
{
    bool isDestructed;
    mtQ_.mt_push(make_shared<TestObj>(isDestructed));
    ASSERT_FALSE(isDestructed);

    mtQ_.mt_clear();
    ASSERT_TRUE(isDestructed) << "REQ: destruct correctly" << endl;
}
TEST_F(MtInQueueTest, clear)
{
    mtQ_.mt_push<void>(nullptr);
    mtQ_.mt_push<void>(nullptr);
    mtQ_.pop();
    mtQ_.mt_push<void>(nullptr);
    ASSERT_EQ(2u, mtQ_.mt_clear()) << "REQ: clear all" << endl;
}

#define WITH_MSG_SELF
// ***********************************************************************************************
TEST_F(MtInQueueTest, GOLD_with_MsgSelf)
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
