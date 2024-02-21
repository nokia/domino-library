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
    UNI_LOG_TEST()
    {
        UNI_LOG::reset_thenMemRisk();  // clean env
        nLog_  = UNI_LOG::nLog();
    }
    ~UNI_LOG_TEST()
    {
        UNI_LOG::reset_thenMemRisk();
        EXPECT_EQ(nLog_, UNI_LOG::nLog());   // log(s) freed for next case

        EXPECT_EQ(0u, UNI_LOG::logLen());    // log content cleaned for next case
        EXPECT_EQ(0u, UNI_LOG::logLen(logName_));
    }

    // -------------------------------------------------------------------------------------------
    size_t nLog_;
    const string logName_ = string(UnitTest::GetInstance()->current_test_info()->test_suite_name())
        + '.' + UnitTest::GetInstance()->current_test_info()->name();

    // -------------------------------------------------------------------------------------------
    struct ClassUsr : public UNI_LOG
    {
        // req: can log
        ClassUsr(const UniLogName aUniLogName = ULN_DEFAULT) : UNI_LOG(aUniLogName)
        {
            DBG("hello, this=" << this << ", nLog=" << nLog());
        }
        ClassUsr(const ClassUsr& rhs) : UNI_LOG(rhs)
        {
            DBG("hello copy=" << this << " from=" << &rhs << ", nLog=" << nLog());
        }
        ClassUsr(ClassUsr&& rhs) : UNI_LOG(move(rhs))
        {
            DBG("hello move=" << this << " from=" << &rhs << ", nLog=" << nLog());
            mvCalled_ = true;
        }
        ~ClassUsr()
        {
            DBG("bye, this=" << this << ", nLog=" << nLog());
        }

        bool mvCalled_ = false;
    };

    static void funcUsr(UNI_LOG& oneLog = *UNI_LOG::defaultUniLog())
    {
        DBG("hello");                          // req: can log, same API
    }

    // -------------------------------------------------------------------------------------------
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
TEST_F(UNI_LOG_TEST, GOLD_OneCellWith_classes_and_funcs)
{
    for (size_t i=0; i < 100; ++i) // for perf bug
    {
        SCOPED_TRACE(i);  // debug which "i" failed

        ClassUsr classUsr(logName_);
        const auto len_1 = UNI_LOG::logLen(logName_);
        ASSERT_GT(len_1, 0) << "REQ: can log";

        ClassUsr classUsr_2(logName_);
        const auto len_2 = UNI_LOG::logLen(logName_);
        ASSERT_GT(len_2, len_1) << "REQ: can log more in same log";

        funcUsr(classUsr);  // req: classUsr can call func & log into same smartlog
        const auto len_3 = UNI_LOG::logLen(logName_);
        ASSERT_GT(len_3, len_2) << "REQ: can log more in same log";

        funcUsr(classUsr_2);  // REQ: classUsr_2 can call func & log into same smartlog
        const auto len_4 = UNI_LOG::logLen(logName_);
        ASSERT_GT(len_4, len_3) << "REQ: can log more in same log";

        if (Test::HasFailure()) classUsr.needLog();
    }
}

// ***********************************************************************************************
TEST_F(UNI_LOG_TEST, withinOneCell_decouple_objects)
{
    auto classUsr = make_shared<ClassUsr>((logName_));
    const auto len_1 = UNI_LOG::logLen(logName_);
    ASSERT_GT(len_1, 0) << "REQ: can log";

    auto classUsr_2 = ClassUsr(logName_);
    const auto len_2 = UNI_LOG::logLen(logName_);
    ASSERT_GT(len_2, len_1) << "REQ: can log";

    classUsr.reset();
    const auto len_3 = UNI_LOG::logLen(logName_);
    ASSERT_GT(len_3, len_2) << "REQ: ClassUsr-destructed shall not crash/impact ClassUsr's log";

    if (Test::HasFailure()) classUsr_2.needLog();
}
TEST_F(UNI_LOG_TEST, withinOneCell_decouple_copies)
{
    auto classUsr = make_shared<ClassUsr>((logName_));
    const auto len_1 = UNI_LOG::logLen(logName_);
    ASSERT_GT(len_1, 0) << "REQ: can log";

    auto copy = *classUsr;
    const auto len_2 = UNI_LOG::logLen(logName_);
    ASSERT_GT(len_2, len_1) << "REQ: log still there";

    classUsr.reset();
    const auto len_3 = UNI_LOG::logLen(logName_);
    ASSERT_GT(len_3, len_2) << "REQ: ClassUsr-destructed shall not crash/impact copy's logging";

    auto mv = move(copy);
    ASSERT_TRUE(mv.mvCalled_);
    const auto len_4 = UNI_LOG::logLen(logName_);
    ASSERT_GT(len_4, len_3) << "REQ: log support mv construct";

    copy.oneLog() << "ClassUsr's mv actually call UNI_LOG's cp by compiler";
    const auto len_5 = UNI_LOG::logLen(logName_);
    ASSERT_GT(len_5, len_4) << "REQ: copy's log must works well";

    // req: UNI_LOG not support assignemt, copy is enough

    copy.needLog();  // req: can still output log to screen
}
TEST_F(UNI_LOG_TEST, withinOneCell_decouple_callbackFuncs)
{
    auto classUsr = make_shared<ClassUsr>((logName_));
    const auto len_1 = UNI_LOG::logLen(logName_);
    ASSERT_GT(len_1, 0) << "REQ: can log";

    function<void()> cb = [oneLog = *classUsr]() mutable { DBG("hello world, I'm a callback func"); };
    const auto len_2 = UNI_LOG::logLen(logName_);
    ASSERT_GT(len_2, len_1) << "REQ: log still there (more log since no move-construct of ClassUsr)";

    /*if (Test::HasFailure())*/ classUsr->needLog();
    classUsr.reset();
    cb();
    const auto len_3 = UNI_LOG::logLen(logName_);
    ASSERT_GT(len_3, len_2) << "REQ: can log";
}

// ***********************************************************************************************
TEST_F(UNI_LOG_TEST, GOLD_no_explicit_CellLog_like_legacy)
{
    const auto len_1 = UNI_LOG::logLen();
    ClassUsr classUsr;  // no explicit UNI_LOG
    const auto len_2 = UNI_LOG::logLen();
    ASSERT_GE(len_2, len_1) << "REQ: can log";
    ASSERT_EQ(ULN_DEFAULT, classUsr.uniLogName()) << "REQ: default";

    ClassUsr classUsr_2;  // dup no explicit UNI_LOG
    const auto len_3 = UNI_LOG::logLen();
    ASSERT_GE(len_3, len_2) << "REQ: can log";
    ASSERT_EQ(ULN_DEFAULT, classUsr_2.uniLogName()) << "REQ: default";

    funcUsr();  // no explicit UNI_LOG
    const auto len_4 = UNI_LOG::logLen();
    ASSERT_GE(len_4, len_3) << "REQ: can log";

    ClassUseDefaultLog nonCell;  // class not based on UNI_LOG
    const auto len_5 = UNI_LOG::logLen();
    ASSERT_GE(len_5, len_4) << "REQ: can log";

    funcUseDefaultLog();  // func w/o UNI_LOG para
    const auto len_6 = UNI_LOG::logLen();
    ASSERT_GE(len_6, len_5) << "REQ: can log";

    if (Test::HasFailure()) UNI_LOG::defaultUniLog_->needLog();
}

// ***********************************************************************************************
TEST_F(UNI_LOG_TEST, GOLD_const_usr)
{
    const ClassUsr classUsr(logName_);
    const auto len_1 = UNI_LOG::logLen(logName_);
    ASSERT_GT(len_1, 0) << "REQ: can log";
}

}  // namespace
