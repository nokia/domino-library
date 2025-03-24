/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include <atomic>
#include <chrono>
#include <gtest/gtest.h>

#define IN_GTEST
#include "ThreadBack.hpp"
#undef IN_GTEST

#include "AsyncBack.hpp"
#include "MT_PingMainTH.hpp"
#include "MtInQueue.hpp"
#include "ThPoolBack.hpp"
#include "ThreadBackViaMsgSelf.hpp"
#include "UniLog.hpp"

using namespace std;
using namespace std::chrono;
using namespace testing;

namespace rlib
{
// ***********************************************************************************************
struct THREAD_BACK_TEST : public Test, public UniLog
{
    THREAD_BACK_TEST() : UniLog(UnitTest::GetInstance()->current_test_info()->name())
    {
        EXPECT_EQ(0, threadBack_.nFut()) << "REQ: clear env";
    }

    ~THREAD_BACK_TEST()
    {
        EXPECT_EQ(0, threadBack_.nFut()) << "REQ: handle all";
        mt_getQ().clearHdlrPool();
        ObjAnywhere::deinit();
        GTEST_LOG_FAIL
    }

    // -------------------------------------------------------------------------------------------
    THREAD_BACK_TYPE threadBack_;
    S_PTR<MsgSelf> msgSelf_ = MAKE_PTR<MsgSelf>(uniLogName());
};

#define THREAD_AND_BACK
// ***********************************************************************************************
//                                  [main thread]
//                                       |
//                                       | std::async()    [new thread]
//               ThreadBack::newTaskOK() |--------------------->| MT_TaskEntryFN()
//   [future, TaskBackFN]->fut_backFN_S_ |                      |
//                                       |                      |
//                                       |                      |
//                                       |<.....................| future<>
//        ThreadBack::hdlDoneFut() |
//                                       |
TEST_F(THREAD_BACK_TEST, GOLD_entryFn_inNewThread_thenBackFn_inMainThread_withTimedWait)
{
    EXPECT_TRUE(ThreadBack::mt_inMyMainTH()) << "REQ: OK in main thread";
    EXPECT_TRUE(threadBack_.newTaskOK(
        // MT_TaskEntryFN
        [this]()
        {
            EXPECT_FALSE(ThreadBack::mt_inMyMainTH()) << "REQ: in new thread";
            return make_safe<bool>(true);
        },
        // TaskBackFN
        [this](SafePtr<void>)
        {
            EXPECT_TRUE(ThreadBack::mt_inMyMainTH()) << "REQ: in main thread";
        }
    ));

    while (threadBack_.hdlDoneFut() == 0)
    {
        INF("new thread not end yet...");
        timedwait();  // REQ: timedwait() is more efficient than keep hdlDoneFut()
    }
}
TEST_F(THREAD_BACK_TEST, GOLD_entryFnResult_toBackFn_withoutTimedWait)
{
    const size_t maxThread = 2;  // req: test entryFn result true(succ) / false(fail)
    for (size_t idxThread = 0; idxThread < maxThread; ++idxThread)
    {
        SCOPED_TRACE(idxThread);
        EXPECT_TRUE(threadBack_.newTaskOK(
            // MT_TaskEntryFN
            [idxThread]()
            {
                return make_safe<bool>(idxThread % 2 != 0);  // ret true / false
            },
            // TaskBackFN
            [idxThread](SafePtr<void> aRet)
            {
                EXPECT_EQ(idxThread % 2 != 0, *(safe_cast<bool>(aRet).get())) << "REQ: check true & false";
            }
        ));
    }

    // REQ: no timedwait() but keep asking; cost most time in CI
    for (size_t nHandled = 0; nHandled < maxThread; nHandled += threadBack_.hdlDoneFut())
    {
        INF("nHandled=" << nHandled);
    }
}
TEST_F(THREAD_BACK_TEST, canHandle_someThreadDone_whileOtherRunning)
{
    atomic<bool> canEnd(false);
    threadBack_.newTaskOK(
        // MT_TaskEntryFN
        [&canEnd]()
        {
            while (not canEnd)
                this_thread::yield();  // not end until instruction
            return make_safe<bool>(true);
        },
        // TaskBackFN
        [](SafePtr<void>) {}
    );

    threadBack_.newTaskOK(
        // MT_TaskEntryFN
        []()
        {
            return make_safe<bool>(false);  // quick end
        },
        // TaskBackFN
        [](SafePtr<void>) {}
    );

    while (threadBack_.hdlDoneFut() == 0)
    {
        INF("both threads not end yet, wait...");
        timedwait();
    }

    canEnd = true;  // 1st thread keep running while 2nd is done
    while (threadBack_.hdlDoneFut() == 0)  // no timedwait() so keep occupy cpu
    {
        INF("2nd thread done, wait 1st done...")
        timedwait();
    }
}

#define WAIT_NOTIFY
// ***********************************************************************************************
TEST_F(THREAD_BACK_TEST, GOLD_entryFn_notify_insteadof_timeout)
{
    auto start = high_resolution_clock::now();
    threadBack_.newTaskOK(
        [] { return make_safe<bool>(true); },  // entryFn
        [](SafePtr<void>) {}  // backFn
    );
    timedwait(0, 500'000'000);  // long timer to ensure thread done beforehand
    auto dur = duration_cast<std::chrono::milliseconds>(high_resolution_clock::now() - start);
    EXPECT_LT(dur.count(), 500) << "REQ: entryFn end shall notify g_semToMainTH instead of timeout";

    while (threadBack_.hdlDoneFut() == 0)  // clear all threads
        timedwait();
}

#define ABNORMAL
// ***********************************************************************************************
TEST_F(THREAD_BACK_TEST, GOLD_except_entryFN_backFN)
{
    atomic<int> step{0};
    EXPECT_TRUE(threadBack_.newTaskOK(
        [&step]  // entryFN()
        {
            ++step;
            throw runtime_error("entryFN() exception");
            return make_safe<bool>(true);
        },
        [&step](SafePtr<void> aRet)  // backFN()
        {
            EXPECT_EQ(1, step.load()) << "REQ: entryFN() executed";
            EXPECT_EQ(nullptr, aRet.get()) << "REQ: entryFN() except -> aRet=null";
            ++step;
            throw runtime_error("backFN() exception");
        }
    ));
    while (threadBack_.mt_nDoneFut().load() < 1) {  // REQ: counter++ after entryFN() except
        timedwait();
    }
    threadBack_.hdlDoneFut();
    EXPECT_EQ(2, step.load()) << "REQ: backFN() executed";
    EXPECT_EQ(0, threadBack_.nFut()) << "REQ: task removed from fut_backFN_S_ after exceptions";
    EXPECT_EQ(0, threadBack_.mt_nDoneFut().load()) << "REQ: ok after backFN() except";
}
TEST_F(THREAD_BACK_TEST, emptyThreadList_ok)
{
    size_t nHandled = threadBack_.hdlDoneFut();
    EXPECT_EQ(0u, nHandled);
}
TEST_F(THREAD_BACK_TEST, invalid_msgSelf_entryFN_backFN)
{
    EXPECT_FALSE(threadBack_.newTaskOK(
        [] { return make_safe<bool>(true); },  // entryFn
        viaMsgSelf([](SafePtr<void>) {})  // invalid since msgSelf==nullptr
    ));

    ObjAnywhere::init();
    ObjAnywhere::emplaceObjOK(msgSelf_);  // valid MSG_SELF
    EXPECT_FALSE(threadBack_.newTaskOK(
        [] { return make_safe<bool>(true); },  // entryFn
        viaMsgSelf(nullptr)  // invalid since backFn==nullptr
    ));

    EXPECT_FALSE(threadBack_.newTaskOK(
        MT_TaskEntryFN(),  // invalid entryFn
        [](SafePtr<void>) {}  // backFn
    ));

    EXPECT_EQ(0, threadBack_.nFut());
}
TEST_F(THREAD_BACK_TEST, bugFix_nDoneFut_before_futureReady)
{
    atomic<bool> canEnd(false);
    threadBack_.newTaskOK(
        // MT_TaskEntryFN
        [&canEnd]()
        {
            while (not canEnd)
                this_thread::yield();  // not end until instruction
            return make_safe<bool>(true);
        },
        // TaskBackFN
        [](SafePtr<void>) {}
    );

    threadBack_.mt_nDoneFut()++;  // force +1 before future ready, threadBack_ shall not crash

    // clean
    canEnd = true;
    while (threadBack_.hdlDoneFut() == 0)
        timedwait();
}

#define SEM_TEST
// ***********************************************************************************************
TEST_F(THREAD_BACK_TEST, GOLD_integrate_MsgSelf_ThreadBack_MtInQueue)  // simulate real world
{
    set<string> cb_info;

    // setup msg handler table for mt_getQ()
    EXPECT_EQ(0u, mt_getQ().nHdlr())  << "REQ: init no hdlr";
    mt_getQ().setHdlrOK<string>([this, &cb_info](UniPtr aMsg)
    {
        msgSelf_->newMsg(  // REQ: via MsgSelf
            [aMsg, &cb_info]
            {
                EXPECT_EQ("a", *(STATIC_PTR_CAST<string>(aMsg).get()));
                cb_info.emplace("REQ: a's Q hdlr via MsgSelf");
            }
        );
    });
    EXPECT_EQ(1u, mt_getQ().nHdlr())  << "REQ: count hdlr";
    mt_getQ().setHdlrOK<int>([this, &cb_info](UniPtr aMsg)
    {
        msgSelf_->newMsg(
            [aMsg, &cb_info]
            {
                EXPECT_EQ(2, *(STATIC_PTR_CAST<int>(aMsg).get()));
                cb_info.emplace("REQ: 2's Q hdlr via MsgSelf");
            }
        );
    });
    EXPECT_EQ(2u, mt_getQ().nHdlr())  << "REQ: count hdlr";

    // push
    ObjAnywhere::init();
    ObjAnywhere::emplaceObjOK(msgSelf_);  // valid MSG_SELF
    threadBack_.newTaskOK(
        // entryFn
        [] {
            mt_getQ().mt_push(MAKE_PTR<string>("a"));
            return make_safe<bool>(true);
        },
        // backFn
        viaMsgSelf(  // REQ: via MsgSelf
            [this, &cb_info](SafePtr<void> aRet)
            {
                EXPECT_TRUE(*(safe_cast<bool>(aRet).get())) << "entryFn succ";
                cb_info.emplace("REQ: a's backFn via MsgSelf");
            }
        )
    );
    threadBack_.newTaskOK(
        // entryFn
        [] {
            mt_getQ().mt_push(MAKE_PTR<int>(2));
            return make_safe<bool>(true);
        },
        // backFn
        viaMsgSelf(
            [this, &cb_info](SafePtr<void> aRet)
            {
                EXPECT_TRUE(*(safe_cast<bool>(aRet).get())) << "entryFn succ";
                cb_info.emplace("REQ: 2's backFn via MsgSelf");
            }
        )
    );

    // simulate main()
    const set<string> expect = {"REQ: a's Q hdlr via MsgSelf", "REQ: a's backFn via MsgSelf",
        "REQ: 2's Q hdlr via MsgSelf", "REQ: 2's backFn via MsgSelf"};
    for (;;)
    {
        // handle all done Thread
        INF("nMsg=" << msgSelf_->nMsg() << ", nQ=" << mt_getQ().mt_size(true) << ", nTh=" << threadBack_.nFut());
        threadBack_.hdlDoneFut();

        // handle all existing in mt_getQ()
        INF("nMsg=" << msgSelf_->nMsg() << ", nQ=" << mt_getQ().mt_size(true) << ", nTh=" << threadBack_.nFut());
        mt_getQ().handleAllEle();

        INF("nMsg=" << msgSelf_->nMsg() << ", nQ=" << mt_getQ().mt_size(true) << ", nTh=" << threadBack_.nFut());
        msgSelf_->handleAllMsg();

        INF("nMsg=" << msgSelf_->nMsg() << ", nQ=" << mt_getQ().mt_size(true) << ", nTh=" << threadBack_.nFut());
        if (expect == cb_info)
            return;

        INF("nMsg=" << msgSelf_->nMsg() << ", nQ=" << mt_getQ().mt_size(true) << ", nTh=" << threadBack_.nFut());
        timedwait();
    }
}
TEST_F(THREAD_BACK_TEST, timeout)
{
    timedwait(0, size_t(-1));  // REQ: invalid ns>=1000ms, no die (& clear previous mt_notify if existed)

    auto now = high_resolution_clock::now();
    INF("start");
    timedwait();
    INF("end");
    auto dur = duration_cast<std::chrono::milliseconds>(high_resolution_clock::now() - now);
    EXPECT_GE(dur.count(), 100) << "REQ: default timeout=100ms";

    timedwait(0, 0);  // REQ: immediate timeout to inc cov
}

}  // namespace
