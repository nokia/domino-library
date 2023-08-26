/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include <future>
#include <gtest/gtest.h>
#include <unistd.h>

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

    void threadMain(int aStartNum, int aSteps)
    {
        for (int i = 0; i < aSteps; i++)
        {
            mtQ_.mt_push(make_shared<int>(aStartNum + i));
        }
    }

    // -------------------------------------------------------------------------------------------
    MtInQueue mtQ_;
};

#define FIFO
// ***********************************************************************************************
TEST_F(MtInQueueTest, GOLD_fifo_multiThreadSafe)
{
    const int nMsg = 10000;
    auto push_thread = async(launch::async, bind(&MtInQueueTest::threadMain, this, 0, nMsg));

    int nHdl = 0;
    INF("GOLD_fifo_multiThreadSafe: before loop")
    while (nHdl < nMsg)
    {
        auto msg = static_pointer_cast<int>(mtQ_.mt_pop());
        if (msg) ASSERT_EQ(nHdl++, *msg) << "REQ: fifo";
        else this_thread::yield();  // simulate real world
    }
    INF("REQ: loop cost 2610us now, previously no cache & lock cost 4422us")
}
TEST_F(MtInQueueTest, GOLD_nonBlock_pop)
{
    ASSERT_FALSE(mtQ_.mt_pop()) << "REQ: can pop empty" << endl;

    mtQ_.mt_push(make_shared<string>("1st"));
    mtQ_.mt_push(make_shared<string>("2nd"));
    mtQ_.backdoor().lock();
    ASSERT_FALSE(mtQ_.mt_pop()) << "REQ: not blocked" << endl;

    mtQ_.backdoor().unlock();
    ASSERT_EQ("1st", *static_pointer_cast<string>(mtQ_.mt_pop())) << "REQ: can pop";

    mtQ_.backdoor().lock();
    ASSERT_EQ("2nd", *static_pointer_cast<string>(mtQ_.mt_pop())) << "REQ: can pop from cache" << endl;
    mtQ_.backdoor().unlock();
}
TEST_F(MtInQueueTest, size)
{
    mtQ_.mt_push(nullptr);
    ASSERT_EQ(1u, mtQ_.mt_size())  << "REQ: inc size"  << endl;

    mtQ_.mt_push(nullptr);
    ASSERT_EQ(2u, mtQ_.mt_size())  << "REQ: inc size"  << endl;

    mtQ_.mt_pop();
    ASSERT_EQ(1u, mtQ_.mt_size())  << "REQ: dec size"  << endl;

    mtQ_.mt_pop();
    ASSERT_EQ(0u, mtQ_.mt_size())  << "REQ: dec size"  << endl;
}
TEST_F(MtInQueueTest, noSet_getNull)
{
    EXPECT_FALSE(mtQ_.mt_pop())  << "REQ: get null" << endl;
}
TEST_F(MtInQueueTest, DISABLED_fifo_multiThreadSafe)
{
    const int steps = 10000;
    int startNum_1 = 0;
    int startNum_2 = 0 + steps;
    auto thread_1 = async(launch::async, bind(&MtInQueueTest::threadMain, this, startNum_1, steps));
    auto thread_2 = async(launch::async, bind(&MtInQueueTest::threadMain, this, startNum_2, steps));

    int whichThread = 0;
    int nToThread_1 = 0;
    int nToThread_2 = 0;
    int nEmptyQueue = 0;
    for (int i = 0; i < steps * 2;)
    {
        auto value = static_pointer_cast<int>(mtQ_.mt_pop());
        if (not value)
        {
            ++nEmptyQueue;
            if (nEmptyQueue%2 == 0) usleep(1u);    // give more chance to other threads, & speedup the testcase
            continue;
        }
        ++i;

        if (*value < steps)
        {
            ASSERT_EQ(startNum_1++, *value);       // req: fifo under multi-thread
            if (whichThread != 1) nToThread_1++;   // thread switch
            whichThread = 1;
        }
        else
        {
            ASSERT_EQ(startNum_2++, *value);       // req: fifo under multi-thread
            if (whichThread != 2) nToThread_2++;   // thread switch
            whichThread = 2;
        }

        if (nToThread_1 > 10 && nToThread_2 > 10)  // switch enough, can stop earlier
        {
            break;
        }
    }
    INF("switch to thread_1=" << nToThread_1 << ", switch to thread_2=" << nToThread_2
        << ", nPop_1=" << startNum_1 << ", nPop_2=" << startNum_2 - steps << ", nEmptyQ=" << nEmptyQueue)


    thread_1.wait();
    thread_2.wait();
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
    {
        MtInQueue mtQ;
        mtQ.mt_push(make_shared<TestObj>(isDestructed));
        EXPECT_FALSE(isDestructed);
    }
    EXPECT_TRUE(isDestructed);       // req: destruct correctly
}
}
