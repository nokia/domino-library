/**
 * Copyright 2023 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - basic notify/wait/timeout/dedup already covered via ThreadBackTest, MtInQueueTest, MsgSelfTest
// ***********************************************************************************************
#include <chrono>
#include <gtest/gtest.h>
#include <thread>
#include <time.h>

#include "UniLog.hpp"

#define IN_GTEST
#include "MT_Notifier.hpp"
#undef IN_GTEST

using namespace std;
using namespace testing;

namespace rlib
{
// ***********************************************************************************************
struct MT_NotifierTest : public Test, public UniLog
{
    MT_NotifierTest()
        : UniLog(UnitTest::GetInstance()->current_test_info()->name())
    {}
    ~MT_NotifierTest()
    {
        GTEST_LOG_FAIL
    }

    MT_Notifier notif_;
};

// ***********************************************************************************************
TEST_F(MT_NotifierTest, large_nsec_no_crash)
{
    notif_.mt_notify();  // ensure immediate return
    notif_.timedwait(0, 2'000'000'000);  // REQ: aRestNsec >= 1s -> no crash
}

TEST_F(MT_NotifierTest, max_nsec_no_crash)
{
    notif_.mt_notify();
    notif_.timedwait(0, size_t(-1));  // REQ: extreme ns -> no crash (same as ThreadBackTest::timeout)
}

// ***********************************************************************************************
TEST_F(MT_NotifierTest, timedwait_on_main_thread_ok)
{
    notif_.mt_notify();
    notif_.timedwait(0, 10'000'000);  // REQ: main thread -> assert passes; abort if not
}

// ***********************************************************************************************
TEST_F(MT_NotifierTest, timedwait_immune_to_clock_backward_jump)
{
    timespec probe;
    clock_gettime(CLOCK_REALTIME, &probe);
    if (clock_settime(CLOCK_REALTIME, &probe) != 0)
        GTEST_SKIP() << "can't set CLOCK_REALTIME backward";

    thread th([]{
        this_thread::sleep_for(chrono::milliseconds(50));
        timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec -= 3;  // backward 3s
        clock_settime(CLOCK_REALTIME, &ts);
    });

    timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    notif_.timedwait(0, 300'000'000);  // 300ms
    clock_gettime(CLOCK_MONOTONIC, &t1);
    auto msDur = (t1.tv_sec - t0.tv_sec) * 1000 + (t1.tv_nsec - t0.tv_nsec) / 1'000'000;
    // CLOCK_MONOTONIC：msDur=~300ms; if CLOCK_REALTIME：msDur=~3300ms
    EXPECT_GE(msDur, 200);
    EXPECT_LE(msDur, 1000);

    // restore
    th.join();

    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 3;
    clock_settime(CLOCK_REALTIME, &ts);
}

TEST_F(MT_NotifierTest, timedwait_immune_to_clock_forward_jump)
{
    timespec probe;
    clock_gettime(CLOCK_REALTIME, &probe);
    if (clock_settime(CLOCK_REALTIME, &probe) != 0)
        GTEST_SKIP() << "can't set CLOCK_REALTIME forward";

    thread th([]{
        this_thread::sleep_for(chrono::milliseconds(50));
        timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 3;  // forward 3s
        clock_settime(CLOCK_REALTIME, &ts);
    });

    timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    notif_.timedwait(0, 300'000'000);  // 300ms
    clock_gettime(CLOCK_MONOTONIC, &t1);
    // CLOCK_MONOTONIC：msDur=~300ms; if CLOCK_REALTIME：msDur=~50ms
    auto msDur = (t1.tv_sec - t0.tv_sec) * 1000 + (t1.tv_nsec - t0.tv_nsec) / 1'000'000;
    EXPECT_GE(msDur, 200);
    EXPECT_LE(msDur, 1000);

    // restore
    th.join();

    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec -= 3;
    clock_settime(CLOCK_REALTIME, &ts);
}

// ***********************************************************************************************
// Race: sem_clockwait returns → worker notify (flag still set, no sem_post!) → clear → timedwait blocks
TEST_F(MT_NotifierTest, noLostNotify_underHighFreqNotify)
{
    notif_.reset();
    atomic<bool> stop{false};

    thread worker([&]{
        while (!stop.load(memory_order_relaxed))
            notif_.mt_notify();  // surge notify
    });

    int nLost = 0;
    for (int i = 0; i < 100; ++i)
    {
        auto t0 = chrono::steady_clock::now();
        notif_.timedwait(0, 50'000'000);  // 50ms timeout
        auto ms = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - t0).count();
        if (ms >= 45)  // should wake immediately, not timeout
            ++nLost;
    }

    stop = true;
    worker.join();
    EXPECT_EQ(0, nLost) << "REQ: no notify lost under surge notify";
}

}  // namespace
