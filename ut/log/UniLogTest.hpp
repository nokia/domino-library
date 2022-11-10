/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include <gtest/gtest.h>

using namespace testing;

namespace RLib
{
// ***********************************************************************************************
struct UNI_LOG_TEST : public Test
{
    ~UNI_LOG_TEST()
    {
        EXPECT_EQ(nLog_, UniLog::nLog());      // req: clean
    }
    const size_t nLog_ = UniLog::nLog();

    const string logName_ = UnitTest::GetInstance()->current_test_info()->name();

    // -------------------------------------------------------------------------------------------
    struct ClassUsr : public UniLog
    {
        // req: can log
        ClassUsr(const UniLogName aUniLogName = ULN_DEFAULT) : UniLog(aUniLogName) { DBG("hello, this=" << this); }
        ~ClassUsr() { DBG("bye, this=" << this); }
    };

    static void funcUsr(UniLog& oneLog = UniLog::defaultUniLog())
    {
        DBG("hello");                          // req: can log, same API
    }

    struct ClassUseDefaultLog
    {
        ClassUseDefaultLog() { DBG("hello"); } // req: can log
        ~ClassUseDefaultLog() { DBG("bye"); }  // req: can log
    };

    static void funcUseDefaultLog()
    {
        DBG("hello");                          // req: can log, same API
    }
};

// ***********************************************************************************************
TEST_F(UNI_LOG_TEST, GOLD_usr_of_class_and_func)
{
    const auto len_0 = UniLog::logLen(logName_);
    {
        ClassUsr classUsr(logName_);
        const auto len_1 = UniLog::logLen(logName_);
        EXPECT_GT(len_1, len_0);           // req: can log

        ClassUsr classUsr_2(logName_);
        const auto len_2 = UniLog::logLen(logName_);
        EXPECT_GT(len_2, len_1);           // req: can log more in same log

        funcUsr(classUsr);                 // req: classUsr can call func & log into same smartlog
        const auto len_3 = UniLog::logLen(logName_);
        EXPECT_GT(len_3, len_2);           // req: can log more in same log

        funcUsr(classUsr_2);               // req: classUsr_2 can call func & log into same smartlog
        const auto len_4 = UniLog::logLen(logName_);
        EXPECT_GT(len_4, len_3);           // req: can log more in same log

        classUsr.needLog();                // req: shall output log to screen
    }
}

// ***********************************************************************************************
TEST_F(UNI_LOG_TEST, low_couple_objects)
{
    auto classUsr = make_shared<ClassUsr>((logName_));
    const auto len_1 = UniLog::logLen(logName_);
    EXPECT_GT(len_1, 0);                   // req: can log

    auto classUsr_2 = make_shared<ClassUsr>(logName_);
    const auto len_2 = UniLog::logLen(logName_);
    EXPECT_GT(len_2, len_1);               // req: can log

    classUsr.reset();
    const auto len_3 = UniLog::logLen(logName_);
    EXPECT_GT(len_3, len_2);               // req: ClassUsr-destructed shall not crash/impact ClassUsr's logging

    if (Test::HasFailure()) classUsr_2->needLog();
    classUsr_2.reset();
}
TEST_F(UNI_LOG_TEST, low_couple_between_copies)
{
    auto classUsr = make_shared<ClassUsr>((logName_));
    const auto len_1 = UniLog::logLen(logName_);
    EXPECT_GT(len_1, 0);                   // req: can log

    auto copy = make_shared<ClassUsr>(*classUsr);
    const auto len_2 = UniLog::logLen(logName_);
    EXPECT_EQ(len_2, len_1);               // req: log still there

    classUsr.reset();
    const auto len_3 = UniLog::logLen(logName_);
    EXPECT_GT(len_3, len_2);               // req: ClassUsr-destructed shall not crash/impact copy's logging

    if (Test::HasFailure()) copy->needLog();
    copy.reset();
}
TEST_F(UNI_LOG_TEST, low_couple_callbackFunc)
{
    auto classUsr = make_shared<ClassUsr>((logName_));
    const auto len_1 = UniLog::logLen(logName_);
    EXPECT_GT(len_1, 0);                   // req: can log
    {
        function<void()> cb = [oneLog = *classUsr]() mutable { INF("hello world, I'm a callback func"); };
        const auto len_2 = UniLog::logLen(logName_);
        EXPECT_GE(len_2, len_1);           // req: log still there (more log since no move-construct of ClassUsr)

        classUsr.reset();
        cb();
        const auto len_3 = UniLog::logLen(logName_);
        EXPECT_GT(len_3, len_2);           // req: can log
    }
}

// ***********************************************************************************************
TEST_F(UNI_LOG_TEST, no_explicit_CellLog_like_legacy)
{
    {
        ClassUsr classUsr;                 // req: no explicit UniLog
        ClassUsr classUsr_2;               // req: no explicit UniLog
        funcUsr();                         // req: no explicit UniLog

        ClassUseDefaultLog nonCell;        // req: class not based on UniLog
        funcUseDefaultLog();               // req: func w/o UniLog para
    }
    UniLog::defaultUniLog_->needLog();
    UniLog::defaultUniLog_.reset();        // dump log in time
}

}  // namespace
