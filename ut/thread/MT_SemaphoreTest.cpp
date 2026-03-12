/**
 * Copyright 2023 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// MT_SemaphoreTest: UT for SEM-2/3/5 modifications
// - basic notify/wait/timeout/dedup already covered via ThreadBackTest, MtInQueueTest, MsgSelfTest
// ***********************************************************************************************
#include <gtest/gtest.h>

#include "UniLog.hpp"

#define IN_GTEST
#include "MT_Semaphore.hpp"
#undef IN_GTEST

using namespace std;
using namespace testing;

namespace rlib
{
// ***********************************************************************************************
struct MT_SemaphoreTest : public Test, public UniLog
{
    MT_SemaphoreTest()
        : UniLog(UnitTest::GetInstance()->current_test_info()->name())
    {}
    ~MT_SemaphoreTest()
    {
        GTEST_LOG_FAIL
    }

    MT_Semaphore sem_;
};

// ***********************************************************************************************
TEST_F(MT_SemaphoreTest, SEM5_large_nsec_no_crash)
{
    sem_.mt_notify();  // ensure immediate return
    sem_.timedwait(0, 2'000'000'000);  // REQ: aRestNsec >= 1s -> HID warning, no crash
}

TEST_F(MT_SemaphoreTest, SEM5_max_nsec_no_crash)
{
    sem_.mt_notify();
    sem_.timedwait(0, size_t(-1));  // REQ: extreme ns -> HID warning, no crash (same as ThreadBackTest::timeout)
}

// ***********************************************************************************************
TEST_F(MT_SemaphoreTest, SEM2_timedwait_on_main_thread_ok)
{
    sem_.mt_notify();
    sem_.timedwait(0, 10'000'000);  // REQ: main thread -> assert passes; abort if not
}

}  // namespace
