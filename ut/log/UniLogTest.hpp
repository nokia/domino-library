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
    struct CLASS_USR : public UniLog
    {
        // req: can log
        CLASS_USR(const UniLogName aUniLogName = ULN_DEFAULT) : UniLog(aUniLogName) { DBG("hello, this=" << this); }
        ~CLASS_USR() { DBG("bye, this=" << this); }
    };

    void funcUsr(UniLog& oneLog = UniLog::defaultUniLog())
    {
        DBG("hello");                          // req: can log, same API
    }

    struct CLASS_USE_DFT_LOG
    {
        CLASS_USE_DFT_LOG() { DBG("hello"); } // req: can log
        ~CLASS_USE_DFT_LOG() { DBG("bye"); }  // req: can log
    };

    void funcUseDefaultLog()
    {
        DBG("hello");                          // req: can log, same API
    }

    const string LOG_NAME = string(UnitTest::GetInstance()->current_test_info()->test_suite_name()) + '.' + UnitTest::GetInstance()->current_test_info()->name();
};

// ***********************************************************************************************
TEST_F(UNI_LOG_TEST, GOLD_usr_of_class_and_func)
{
    const auto nLogBegin = UniLog::nLog();

    const auto len_0 = UniLog::logLen(LOG_NAME);
    {
        CLASS_USR classUsr(LOG_NAME);
        const auto len_1 = UniLog::logLen(LOG_NAME);
        EXPECT_GT(len_1, len_0);           // req: can log

        CLASS_USR classUsr_2(LOG_NAME);
        const auto len_2 = UniLog::logLen(LOG_NAME);
        EXPECT_GT(len_2, len_1);           // req: can log more in same log

        funcUsr(classUsr);                 // req: classUsr can call func & log into same smartlog
        const auto len_3 = UniLog::logLen(LOG_NAME);
        EXPECT_GT(len_3, len_2);           // req: can log more in same log

        funcUsr(classUsr_2);               // req: classUsr_2 can call func & log into same smartlog
        const auto len_4 = UniLog::logLen(LOG_NAME);
        EXPECT_GT(len_4, len_3);           // req: can log more in same log

        classUsr.needLog();                // req: shall output log to screen
    }
    EXPECT_EQ(nLogBegin, UniLog::nLog());  // req: del log when no user
}

// ***********************************************************************************************
TEST_F(UNI_LOG_TEST, low_couple_objects)
{
    const auto nLogBegin = UniLog::nLog();

    auto classUsr = make_shared<CLASS_USR>((LOG_NAME));
    const auto len_1 = UniLog::logLen(LOG_NAME);
    EXPECT_GT(len_1, 0);                   // req: can log

    auto classUsr_2 = make_shared<CLASS_USR>(LOG_NAME);
    const auto len_2 = UniLog::logLen(LOG_NAME);
    EXPECT_GT(len_2, len_1);               // req: can log

    classUsr.reset();
    const auto len_3 = UniLog::logLen(LOG_NAME);
    EXPECT_GT(len_3, len_2);               // req: CLASS_USR-destructed shall not crash/impact CLASS_USR's logging

    if (Test::HasFailure()) classUsr_2->needLog();
    classUsr_2.reset();
    EXPECT_EQ(nLogBegin, UniLog::nLog());  // req: del log when no user
}
TEST_F(UNI_LOG_TEST, low_couple_between_copies)
{
    const auto nLogBegin = UniLog::nLog();

    auto classUsr = make_shared<CLASS_USR>((LOG_NAME));
    const auto len_1 = UniLog::logLen(LOG_NAME);
    EXPECT_GT(len_1, 0);                   // req: can log

    auto copy = make_shared<CLASS_USR>(*classUsr);
    const auto len_2 = UniLog::logLen(LOG_NAME);
    EXPECT_EQ(len_2, len_1);               // req: log still there

    classUsr.reset();
    const auto len_3 = UniLog::logLen(LOG_NAME);
    EXPECT_GT(len_3, len_2);               // req: CLASS_USR-destructed shall not crash/impact copy's logging

    if (Test::HasFailure()) copy->needLog();
    copy.reset();
    EXPECT_EQ(nLogBegin, UniLog::nLog());  // req: del log when no user
}
TEST_F(UNI_LOG_TEST, low_couple_callbackFunc)
{
    const auto nLogBegin = UniLog::nLog();

    auto classUsr = make_shared<CLASS_USR>((LOG_NAME));
    const auto len_1 = UniLog::logLen(LOG_NAME);
    EXPECT_GT(len_1, 0);                   // req: can log
    {
        function<void()> cb = [oneLog = *classUsr]() mutable { INF("hello world, I'm a callback func"); };
        const auto len_2 = UniLog::logLen(LOG_NAME);
        EXPECT_GE(len_2, len_1);           // req: log still there (more log since no move-construct of CLASS_USR)

        classUsr.reset();
        cb();
        const auto len_3 = UniLog::logLen(LOG_NAME);
        EXPECT_GT(len_3, len_2);           // req: can log
    }
    EXPECT_EQ(nLogBegin, UniLog::nLog());  // req: del log when no user
}

// ***********************************************************************************************
TEST_F(UNI_LOG_TEST, no_explicit_CellLog_like_legacy)
{
    const auto nLogBegin = UniLog::nLog();
cerr<<__LINE__<<endl;
    {
        CLASS_USR classUsr;                 // req: no explicit UniLog
cerr<<__LINE__<<endl;
        CLASS_USR classUsr_2;               // req: no explicit UniLog
        funcUsr();                         // req: no explicit UniLog
cerr<<__LINE__<<endl;

        CLASS_USE_DFT_LOG nonCell;        // req: class not based on UniLog
cerr<<__LINE__<<endl;
        funcUseDefaultLog();               // req: func w/o UniLog para
cerr<<__LINE__<<endl;
    }
    UniLog::defaultUniLog_->needLog();
cerr<<__LINE__<<endl;
    UniLog::defaultUniLog_.reset();        // dump log in time
cerr<<__LINE__<<endl;
    EXPECT_EQ(nLogBegin, UniLog::nLog());  // req: del log when no user
cerr<<__LINE__<<endl;
}

}  // namespace
