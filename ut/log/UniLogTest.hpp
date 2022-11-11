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
        UniLog::reset();
        EXPECT_EQ(nLog_, UniLog::nLog());      // log(s) freed for next case

        EXPECT_EQ(0u, UniLog::logLen());       // log content cleaned for next case
        EXPECT_EQ(0u, UniLog::logLen(logName_));
    }
    const size_t nLog_ = UniLog::nLog();

    const string logName_ = string(UnitTest::GetInstance()->current_test_info()->test_suite_name())
        + '.' + UnitTest::GetInstance()->current_test_info()->name();

    // -------------------------------------------------------------------------------------------
    struct ClassUsr : public UniLog
    {
        // req: can log
        ClassUsr(const UniLogName aUniLogName = ULN_DEFAULT) : UniLog(aUniLogName) { DBG("hello, this=" << this); }
        ClassUsr(const ClassUsr& rhs) : UniLog(rhs) { DBG("hello copy=" << this << " from=" << &rhs); }
        ClassUsr(ClassUsr&& rhs) : UniLog(move(rhs))
        {
            DBG("hello move=" << this << " from=" << &rhs);
            mvCalled_ = true;
        }
        ~ClassUsr() { DBG("bye, this=" << this); }

        bool mvCalled_ = false;
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
    ClassUsr classUsr(logName_);
    const auto len_1 = UniLog::logLen(logName_);
    EXPECT_GT(len_1, 0);                       // req: can log

    ClassUsr classUsr_2(logName_);
    const auto len_2 = UniLog::logLen(logName_);
    EXPECT_GT(len_2, len_1);                   // req: can log more in same log

    funcUsr(classUsr);                         // req: classUsr can call func & log into same smartlog
    const auto len_3 = UniLog::logLen(logName_);
    EXPECT_GT(len_3, len_2);                   // req: can log more in same log

    funcUsr(classUsr_2);                       // req: classUsr_2 can call func & log into same smartlog
    const auto len_4 = UniLog::logLen(logName_);
    EXPECT_GT(len_4, len_3);                   // req: can log more in same log

    if (Test::HasFailure()) classUsr.needLog();
}

// ***********************************************************************************************
TEST_F(UNI_LOG_TEST, low_couple_objects)
{
    auto classUsr = make_shared<ClassUsr>((logName_));
    const auto len_1 = UniLog::logLen(logName_);
    EXPECT_GT(len_1, 0);                       // req: can log

    auto classUsr_2 = ClassUsr(logName_);
    const auto len_2 = UniLog::logLen(logName_);
    EXPECT_GT(len_2, len_1);                   // req: can log

    classUsr.reset();
    const auto len_3 = UniLog::logLen(logName_);
    EXPECT_GT(len_3, len_2);                   // req: ClassUsr-destructed shall not crash/impact ClassUsr's log

    if (Test::HasFailure()) classUsr_2.needLog();
}
TEST_F(UNI_LOG_TEST, low_couple_between_copies)
{
    auto classUsr = make_shared<ClassUsr>((logName_));
    const auto len_1 = UniLog::logLen(logName_);
    EXPECT_GT(len_1, 0);                       // req: can log

    auto copy = *classUsr;
    const auto len_2 = UniLog::logLen(logName_);
    EXPECT_GT(len_2, len_1);                   // req: log still there

    classUsr.reset();
    const auto len_3 = UniLog::logLen(logName_);
    EXPECT_GT(len_3, len_2);                   // req: ClassUsr-destructed shall not crash/impact copy's logging

    auto mv = move(copy);
    EXPECT_TRUE(mv.mvCalled_);
    const auto len_4 = UniLog::logLen(logName_);
    EXPECT_GT(len_4, len_3);                   // req: log support mv construct

    copy.oneLog() << "ClassUsr's mv actually call UniLog's cp by compiler" << endl;
    const auto len_5 = UniLog::logLen(logName_);
    EXPECT_GT(len_5, len_4);                   // req: copy's log must works well

    // req: UniLog not support assignemt, copy is enough

    copy.needLog();                            // req: can still output log to screen
}
TEST_F(UNI_LOG_TEST, low_couple_callbackFunc)
{
    auto classUsr = make_shared<ClassUsr>((logName_));
    const auto len_1 = UniLog::logLen(logName_);
    EXPECT_GT(len_1, 0);                       // req: can log

    function<void()> cb = [oneLog = *classUsr]() mutable { INF("hello world, I'm a callback func"); };
    const auto len_2 = UniLog::logLen(logName_);
    EXPECT_GT(len_2, len_1);                   // req: log still there (more log since no move-construct of ClassUsr)

    if (Test::HasFailure()) classUsr->needLog();
    classUsr.reset();
    cb();
    const auto len_3 = UniLog::logLen(logName_);
    EXPECT_GT(len_3, len_2);                   // req: can log
}

// ***********************************************************************************************
TEST_F(UNI_LOG_TEST, no_explicit_CellLog_like_legacy)
{
    const auto len_1 = UniLog::logLen();
    ClassUsr classUsr;                         // no explicit UniLog
    const auto len_2 = UniLog::logLen();
    EXPECT_GE(len_2, len_1);                   // req: can log

    ClassUsr classUsr_2;                       // dup no explicit UniLog
    const auto len_3 = UniLog::logLen();
    EXPECT_GE(len_3, len_2);                   // req: can log

    funcUsr();                                 // no explicit UniLog
    const auto len_4 = UniLog::logLen();
    EXPECT_GE(len_4, len_3);                   // req: can log

    ClassUseDefaultLog nonCell;                // class not based on UniLog
    const auto len_5 = UniLog::logLen();
    EXPECT_GE(len_5, len_4);                   // req: can log

    funcUseDefaultLog();                       // func w/o UniLog para
    const auto len_6 = UniLog::logLen();
    EXPECT_GE(len_6, len_5);                   // req: can log

    if (Test::HasFailure()) UniLog::defaultUniLog_->needLog();
}

}  // namespace
