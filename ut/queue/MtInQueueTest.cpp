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
#include "MtInQueue.hpp"

using namespace testing;

namespace RLib {
// ***********************************************************************************************
struct MtQueueTest : public Test, public UniLog
{
    MtQueueTest() : UniLog(UnitTest::GetInstance()->current_test_info()->name()) {}
    ~MtQueueTest() { GTEST_LOG_FAIL }

    void threadMain(int aStartNum, int aSteps)
    {
        for (int i = 0; i < aSteps; i++)
        {
            mtQueue_.mt_push(make_shared<int>(aStartNum + i));
            if (i == 0 || i == aSteps/2) usleep(1u);  // give chance to other threads, at least 2
        }
    }

    MtInQueue mtQueue_;
};

#define FIFO
// ***********************************************************************************************
TEST_F(MtQueueTest, noSet_getNull)
{
    EXPECT_FALSE(mtQueue_.mt_pop()) << "REQ: get null" << endl;
    EXPECT_EQ(0, mtQueue_.mt_size()) << "REQ: empty at beginning"  << endl;
}
TEST_F(MtQueueTest, GOLD_fifo_multiThreadSafe)
{
    const int steps = 10000;
    int startNum_1 = 0;
    int startNum_2 = 0 + steps;
    auto thread_1 = async(launch::async, bind(&MtQueueTest::threadMain, this, startNum_1, steps));
    auto thread_2 = async(launch::async, bind(&MtQueueTest::threadMain, this, startNum_2, steps));

    int whichThread = 0;
    int nToThread_1 = 0;
    int nToThread_2 = 0;
    int nEmptyQueue = 0;
    for (int i = 0; i < steps * 2;)
    {
        auto value = static_pointer_cast<int>(mtQueue_.mt_pop());
        if (not value)
        {
            ++nEmptyQueue;
            if (nEmptyQueue%2 == 0) usleep(1u);    // give more chance to other threads, & speedup the testcase
            continue;
        }
        ++i;

        if (*value < steps)
        {
            EXPECT_EQ(startNum_1++, *value);       // req: fifo under multi-thread
            if (whichThread != 1) nToThread_1++;   // thread switch
            whichThread = 1;
        }
        else
        {
            EXPECT_EQ(startNum_2++, *value);       // req: fifo under multi-thread
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
TEST_F(MtQueueTest, GOLD_destructCorrectly)
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
