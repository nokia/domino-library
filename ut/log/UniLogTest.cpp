/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include <gtest/gtest.h>

#include "UniLog.hpp"

using namespace testing;

namespace RLib
{
// ***********************************************************************************************
struct ClassUsr : public UniLog
{
    // req: can log
    ClassUsr(const UniLogName aUniLogName = ULN_DEFAULT) : UniLog(aUniLogName) { DBG("hello, this=" << this); }
    ~ClassUsr() { DBG("bye, this=" << this); }
};

void funcUsr(UniLog& oneLog = UniLog::defaultUniLog())
{
    DBG("hello");  // req: can log, same API
}

// ***********************************************************************************************
TEST(UniLogTest, GOLD_usr_of_class_and_func)
{
    const auto nLogBegin = UniLog::nLog();

    const char LOG_NAME[] = "GOLD_usr_of_class_and_func";
    const auto len_0 = UniLog::logLen(LOG_NAME);
    {
        ClassUsr classUsr(LOG_NAME);
        const auto len_1 = UniLog::logLen(LOG_NAME);
        EXPECT_GT(len_1, len_0);           // req: can log

        ClassUsr classUsr_2(LOG_NAME);
        const auto len_2 = UniLog::logLen(LOG_NAME);
        EXPECT_GT(len_2, len_1);           // req: can log more in same log

        funcUsr(classUsr);                 // req: classUsr can call func & log into same smartlog
        const auto len_3 = UniLog::logLen(LOG_NAME);
        EXPECT_GT(len_3, len_2);           // req: can log more in same log

        funcUsr(classUsr_2);               // req: classUsr_2 can call func & log into same smartlog
        const auto len_4 = UniLog::logLen(LOG_NAME);
        EXPECT_GT(len_4, len_3);           // req: can log more in same log

        UniLog::needLog();                 // req: shall output log to screen
    }
    EXPECT_EQ(nLogBegin, UniLog::nLog());  // req: del log when no user
}

// ***********************************************************************************************
TEST(UniLogTest, low_couple_objects)
{
    const auto nLogBegin = UniLog::nLog();

    const char LOG_NAME[] = "low_couple_objects";
    auto classUsr = make_shared<ClassUsr>((LOG_NAME));
    const auto len_1 = UniLog::logLen(LOG_NAME);
    EXPECT_GT(len_1, 0);                   // req: can log

    auto classUsr_2 = make_shared<ClassUsr>(LOG_NAME);
    const auto len_2 = UniLog::logLen(LOG_NAME);
    EXPECT_GT(len_2, len_1);               // req: can log

    classUsr.reset();
    const auto len_3 = UniLog::logLen(LOG_NAME);
    EXPECT_GT(len_3, len_2);               // req: ClassUsr-destructed shall not crash/impact ClassUsr's logging

    if (Test::HasFailure()) UniLog::needLog();
    classUsr_2.reset();
    EXPECT_EQ(nLogBegin, UniLog::nLog());  // req: del log when no user
}
TEST(UniLogTest, low_couple_between_copies)
{
    const auto nLogBegin = UniLog::nLog();

    const char LOG_NAME[] = "low_couple_between_copies";
    auto classUsr = make_shared<ClassUsr>((LOG_NAME));
    const auto len_1 = UniLog::logLen(LOG_NAME);
    EXPECT_GT(len_1, 0);                   // req: can log

    auto copy = make_shared<ClassUsr>(*classUsr);
    const auto len_2 = UniLog::logLen(LOG_NAME);
    EXPECT_EQ(len_2, len_1);               // req: log still there

    classUsr.reset();
    const auto len_3 = UniLog::logLen(LOG_NAME);
    EXPECT_GT(len_3, len_2);               // req: ClassUsr-destructed shall not crash/impact copy's logging

    if (Test::HasFailure()) UniLog::needLog();
    copy.reset();
    EXPECT_EQ(nLogBegin, UniLog::nLog());  // req: del log when no user
}
TEST(UniLogTest, low_couple_callbackFunc)
{
    const auto nLogBegin = UniLog::nLog();

    const char LOG_NAME[] = "low_couple_callbackFunc";
    auto classUsr = make_shared<ClassUsr>((LOG_NAME));
    const auto len_1 = UniLog::logLen(LOG_NAME);
    EXPECT_GT(len_1, 0);                   // req: can log
    {
        function<void()> cb = [oneLog = *classUsr]() mutable { INF("hello world, I'm a callback func"); };
        const auto len_2 = UniLog::logLen(LOG_NAME);
        EXPECT_GE(len_2, len_1);           // req: log still there (more log since no move-construct of ClassUsr)

        classUsr.reset();
        cb();
        const auto len_3 = UniLog::logLen(LOG_NAME);
        EXPECT_GT(len_3, len_2);           // req: can log

        if (Test::HasFailure()) UniLog::needLog();
    }
    EXPECT_EQ(nLogBegin, UniLog::nLog());  // req: del log when no user
}

// ***********************************************************************************************
struct ClassUseDefaultLog
{
    ClassUseDefaultLog() { DBG("hello"); } // req: can log
    ~ClassUseDefaultLog() { DBG("bye"); }  // req: can log
};

void funcUseDefaultLog()
{
    DBG("hello");                          // req: can log, same API
}

// ***********************************************************************************************
TEST(UniLogTest, no_explicit_CellLog_like_legacy)
{
    const auto nLogBegin = UniLog::nLog();
    {
        ClassUsr classUsr;                 // req: no explicit UniLog
        ClassUsr classUsr_2;               // req: no explicit UniLog
        funcUsr();                         // req: no explicit UniLog

        ClassUseDefaultLog nonCell;        // req: class not based on UniLog
        funcUseDefaultLog();               // req: func w/o UniLog para
    }
    if (Test::HasFailure()) UniLog::needLog();
    UniLog::defaultUniLog_.reset();        // dump log in time
    EXPECT_EQ(nLogBegin, UniLog::nLog());  // req: del log when no user
}

}  // namespace
