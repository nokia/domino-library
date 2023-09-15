/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include <future>
#include <gtest/gtest.h>
#include <queue>
#include <unistd.h>

#include "UniLog.hpp"
#include "UtInitObjAnywhere.hpp"

#define MT_IN_Q_TEST
#include "MtInQueue.hpp"
#undef MT_IN_Q_TEST

using namespace testing;

namespace RLib {
// ***********************************************************************************************
struct MtInQueueTest : public UtInitObjAnywhere
{
    MtInQueueTest() { ObjAnywhere::get<MaxNofreeDom>(*this)->setMsgSelf(msgSelf_); }
    void SetUp() override
    {
        ASSERT_EQ(0, mtQ_.mt_size()) << "REQ: empty at beginning"  << endl;
    }
    void TearDown() override
    {
        ASSERT_EQ(0, mtQ_.mt_size()) << "REQ: empty at end"  << endl;

    }

    void threadMain(int aStartNum, int aSteps)
    {
        for (int i = 0; i < aSteps; i++)
        {
            mtQ_.mt_push(make_shared<int>(aStartNum + i));
        }
    }

    // -------------------------------------------------------------------------------------------
    MtInQueue mtQ_;

    // -------------------------------------------------------------------------------------------
    shared_ptr<MsgSelf> msgSelf_ = make_shared<MsgSelf>(
        [this](const PongMainFN& aPongMainFN){ pongMainFN_ = aPongMainFN; }, uniLogName());
    PongMainFN pongMainFN_;
    shared_ptr<MaxNofreeDom> dom_ = ObjAnywhere::get<MaxNofreeDom>(*this);
};

#define FIFO
// ***********************************************************************************************
TEST_F(MtInQueueTest, GOLD_fifo_multiThreadSafe)
{
    const int nMsg = 10000;
    auto push_thread = async(launch::async, [=](){ this->threadMain(0, nMsg); });

    int nHdl = 0;
    INF("GOLD_fifo_multiThreadSafe: before loop")
    while (nHdl < nMsg)
    {
        auto msg = mtQ_.pop<int>();
        if (msg) ASSERT_EQ(nHdl++, *msg) << "REQ: fifo";
        else this_thread::yield();  // simulate real world
    }
    INF("REQ: loop cost 2372us now, previously no cache & lock cost 4422us")
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
TEST_F(MtInQueueTest, size)
{
    mtQ_.mt_push<void>(nullptr);
    ASSERT_EQ(1u, mtQ_.mt_size())  << "REQ: inc size"  << endl;

    mtQ_.mt_push<void>(nullptr);
    ASSERT_EQ(2u, mtQ_.mt_size())  << "REQ: inc size"  << endl;

    mtQ_.pop();
    ASSERT_EQ(1u, mtQ_.mt_size())  << "REQ: dec size"  << endl;

    mtQ_.pop();
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
TEST_F(MtInQueueTest, GOLD_with_domino)
{
    mtQ_.mt_push(make_shared<string>("a"));
    mtQ_.mt_push(make_shared<int>(2));
    mtQ_.mt_push(make_shared<float>(3.0));

    queue<size_t> order;
    for (size_t pri = EMsgPri_MIN; pri < EMsgPri_MAX; pri++)
    {
        auto elePair = mtQ_.pop();
        if (elePair.first == nullptr) break;

        const auto ev_str = to_string(elePair.second);
        dom_->replaceShared(ev_str, elePair.first);
        dom_->setPriority(ev_str, EMsgPriority(pri));
        dom_->setHdlr(ev_str, [&order, pri](){ order.push(pri); });
        dom_->setState({{ev_str, true}});  // to MsgSelf
    }
    pongMainFN_();  // call MsgSelf
    EXPECT_EQ(queue<size_t>({2,1,0}), order) << "REQ: priority FIFO" << endl;
}

}  // namespace
