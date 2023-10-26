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

#include "MT_Semaphore.hpp"
#include "UniLog.hpp"

#define MT_IN_Q_TEST
#include "MtInQueue.hpp"
#undef MT_IN_Q_TEST

using namespace testing;

namespace RLib
{
// ***********************************************************************************************
struct MtInQueueTest : public Test, public UniLog
{
    MtInQueueTest()
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
    ~MtInQueueTest() { GTEST_LOG_FAIL }

    // -------------------------------------------------------------------------------------------
    MtInQueue mtQ_;
    MT_Semaphore mt_waker_;
};

#define FIFO
// ***********************************************************************************************
TEST_F(MtInQueueTest, GOLD_simple_fifo_without_waker)
{
    MtInQueue mtQ;
    mtQ.mt_push<int>(make_shared<int>(1));
    mtQ.mt_push<int>(make_shared<int>(2));
    EXPECT_EQ(1, *(mtQ.pop<int>())) << "REQ: fifo";
    EXPECT_EQ(2, *(mtQ.pop<int>())) << "REQ: fifo";
}
TEST_F(MtInQueueTest, GOLD_sparsePush_fifo)
{
    const int nMsg = 10000;
    auto push_thread = async(launch::async, [&nMsg, this]()
    {
        for (int i = 0; i < nMsg; i++)
        {
            this->mtQ_.mt_push(make_shared<int>(i));
            this_thread::sleep_for(1us);  // simulate real world (sparse msg)
        }
    });

    int nHdl = 0;
    while (nHdl < nMsg)
    {
        auto msg = mtQ_.pop<int>();
        if (msg) ASSERT_EQ(nHdl++, *msg) << "REQ: fifo";
        else mt_waker_.mt_timedwait();  // REQ: less CPU than repeat pop() or this_thread::yield()
    }
    INF("REQ(sleep 1us/push): e2e user=0.354s->0.123s, sys=0.412s->0.159s")
}
TEST_F(MtInQueueTest, GOLD_surgePush_fifo)
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
TEST_F(MtInQueueTest, size_and_nowait)
{
    mtQ_.mt_push<int>(make_shared<int>(1));
    ASSERT_EQ(1u, mtQ_.mt_size())  << "REQ: inc size"  << endl;
    mt_waker_.mt_timedwait();
    ASSERT_EQ(1u, mtQ_.mt_size())  << "REQ: wait() ret immediately since mtQ_ not empty"  << endl;

    mtQ_.mt_push<int>(make_shared<int>(2));
    ASSERT_EQ(2u, mtQ_.mt_size())  << "REQ: inc size"  << endl;

    EXPECT_EQ(1, *(mtQ_.pop<int>())) << "REQ: fifo";
    ASSERT_EQ(1u, mtQ_.mt_size())  << "REQ: dec size"  << endl;
    mt_waker_.mt_timedwait();
    ASSERT_EQ(1u, mtQ_.mt_size())  << "REQ: wait() ret immediately since mtQ_ not empty"  << endl;

    mtQ_.mt_push<int>(make_shared<int>(3));
    mt_waker_.mt_timedwait();
    ASSERT_EQ(2u, mtQ_.mt_size())  << "REQ: wait() ret immediately since mtQ_ not empty"  << endl;

    EXPECT_EQ(2, *(mtQ_.pop<int>())) << "REQ: keep fifo after wait_for()";
    EXPECT_EQ(3, *(mtQ_.pop<int>())) << "REQ: keep fifo after wait_for()";
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

}  // namespace
