/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include "MtQueue.hpp"

#include <future>
#include <unistd.h>
#include <gtest/gtest.h>

using namespace testing;

namespace RLib {
// ***********************************************************************************************
struct MtQueueTest : public Test
{
    void threadMain(int aStartNum, int aSteps)
    {
        for (int i = 0; i < aSteps; i++)
        {
            mtQueue_.push(std::make_shared<int>(aStartNum + i));
            if (i == 0 || i == aSteps/2) usleep(1u);  // give chance to other threads, at least 2
        }
    }

    MtQueue mtQueue_;
};

#define FIFO
// ***********************************************************************************************
TEST_F(MtQueueTest, noSet_getNull)
{
    EXPECT_FALSE(mtQueue_.pop());   // req: get null
}
TEST_F(MtQueueTest, GOLD_fifo_multiThreadSafe)
{
    const int steps = 10000;
    int startNum_1 = 0;
    int startNum_2 = 0 + steps;
    auto thread_1 = async(std::launch::async, std::bind(&MtQueueTest::threadMain, this, startNum_1, steps));
    auto thread_2 = async(std::launch::async, std::bind(&MtQueueTest::threadMain, this, startNum_2, steps));

    int whichThread = 0;
    int nToThread_1 = 0;
    int nToThread_2 = 0;
    int nEmptyQueue = 0;
    for (int i = 0; i < steps * 2;)
    {
        auto value = std::static_pointer_cast<int>(mtQueue_.pop());
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
    std::cerr << "switch to thread_1=" << nToThread_1 << ", switch to thread_2=" << nToThread_2
        << ", nPop_1=" << startNum_1 << ", nPop_2=" << startNum_2 - steps << ", nEmptyQ=" << nEmptyQueue << std::endl;
}

#define FETCH
// ***********************************************************************************************
TEST_F(MtQueueTest, GOLD_fetchSpecified_multiThreadSafe)
{
    const int steps = 10;
    int startNum_1 = 0;
    int startNum_2 = 0 + steps;
    auto thread_1 = async(std::launch::async, std::bind(&MtQueueTest::threadMain, this, startNum_1, steps));
    auto thread_2 = async(std::launch::async, std::bind(&MtQueueTest::threadMain, this, startNum_2, steps));

    int nEmptyQueue = 0;
    for (int i = 0; i < 2;)
    {
        auto value = std::static_pointer_cast<int>(mtQueue_.fetch(
            [](std::shared_ptr<void> aEle) { return *std::static_pointer_cast<int>(aEle) % steps == steps -1; }));
        if (not value)
        {
            ++nEmptyQueue;
            if (nEmptyQueue%2 == 0) usleep(1u);    // give more chance to other threads, & speedup the testcase
            continue;
        }
        ++i;

        if (*value < steps)
        {
            EXPECT_EQ(steps -1, *value);       // req: fetch under multi-thread
            EXPECT_FALSE(mtQueue_.fetch(
                [](std::shared_ptr<void> aEle) { return *std::static_pointer_cast<int>(aEle) == steps -1; }));      // req: rm
        }
        else
        {
            EXPECT_EQ(steps * 2 - 1, *value);  // req: fetch under multi-thread
            EXPECT_FALSE(mtQueue_.fetch(
                [](std::shared_ptr<void> aEle) { return *std::static_pointer_cast<int>(aEle) == steps * 2 -1; }));  // req: rm
        }
    }
    EXPECT_EQ(size_t(steps * 2 - 2), mtQueue_.size());
    std::cerr << "nEmptyQ=" << nEmptyQueue << std::endl;
}
TEST_F(MtQueueTest, GOLD_fetch_null)
{
    EXPECT_FALSE(mtQueue_.fetch([](std::shared_ptr<void>) { return true; }));
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
        MtQueue mtQ;
        mtQ.push(std::make_shared<TestObj>(isDestructed));
        EXPECT_FALSE(isDestructed);
    }
    EXPECT_TRUE(isDestructed);       // req: destruct correctly
}
}
