/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include <atomic>
#include <chrono>
#include <gtest/gtest.h>

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
        EXPECT_EQ(0, threadBack_.nThread()) << "REQ: clear env";
    }

    ~THREAD_BACK_TEST()
    {
        EXPECT_EQ(0, threadBack_.nThread()) << "REQ: handle all";
        mt_getQ().clearHdlrPool();
        GTEST_LOG_FAIL
    }

    // -------------------------------------------------------------------------------------------
    THREAD_BACK_TYPE threadBack_;
    std::shared_ptr<MsgSelf> msgSelf_ = std::make_shared<MsgSelf>(uniLogName());
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
//        ThreadBack::hdlFinishedTasks() |
//                                       |
TEST_F(THREAD_BACK_TEST, GOLD_entryFn_inNewThread_thenBackFn_inMainThread_withTimedWait)
{
    EXPECT_TRUE(ThreadBack::inMyMainTH()) << "REQ: OK in main thread";
    EXPECT_TRUE(threadBack_.newTaskOK(
        // MT_TaskEntryFN
        [this]() -> bool
        {
            EXPECT_FALSE(ThreadBack::inMyMainTH()) << "REQ: in new thread";
            return true;
        },
        // TaskBackFN
        [this](bool)
        {
            EXPECT_TRUE(ThreadBack::inMyMainTH()) << "REQ: in main thread";
        }
    ));

    while (true)
    {
        if (threadBack_.hdlFinishedTasks() == 0)
        {
            INF("new thread not end yet...");
            timedwait();  // REQ: timedwait() is more efficient than keep hdlFinishedTasks()
            continue;
        }
        return;
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
            [idxThread]() -> bool
            {
                return idxThread % 2 != 0;  // ret true / false
            },
            // TaskBackFN
            [idxThread](bool aRet)
            {
                EXPECT_EQ(idxThread % 2 != 0, aRet) << "REQ: check true & false";
            }
        ));
    }

    // REQ: no timedwait() but keep asking
    for (size_t nHandled = 0; nHandled < maxThread; nHandled += threadBack_.hdlFinishedTasks())
    {
        INF("nHandled=" << nHandled);
    }
}
TEST_F(THREAD_BACK_TEST, canHandle_someThreadDone_whileOtherRunning)
{
    atomic<bool> canEnd(false);
    threadBack_.newTaskOK(
        // MT_TaskEntryFN
        [&canEnd]() -> bool
        {
            while (not canEnd)
                this_thread::yield();  // not end until instruction
            return true;
        },
        // TaskBackFN
        [](bool) {}
    );

    threadBack_.newTaskOK(
        // MT_TaskEntryFN
        []() -> bool
        {
            return false;  // quick end
        },
        // TaskBackFN
        [](bool) {}
    );

    while (threadBack_.hdlFinishedTasks() == 0)
    {
        INF("both threads not end yet, wait...");
        this_thread::yield();
    }

    canEnd = true;  // 1st thread keep running while 2nd is done
    while (threadBack_.hdlFinishedTasks() == 0)  // no timedwait() so keep occupy cpu
    {
        INF("2nd thread done, wait 1st done...")
        this_thread::yield();
    }
}

#define WAIT_NOTIFY
// ***********************************************************************************************
TEST_F(THREAD_BACK_TEST, GOLD_entryFn_notify_insteadof_timeout)
{
    auto start = high_resolution_clock::now();
    threadBack_.newTaskOK(
        [] { return true; },  // entryFn
        [](bool) {}  // backFn
    );
    timedwait(0, 500'000'000);  // long timer to ensure thread done beforehand
    auto dur = duration_cast<std::chrono::milliseconds>(high_resolution_clock::now() - start);
    EXPECT_LT(dur.count(), 500) << "REQ: entryFn end shall notify g_semToMainTH instead of timeout";

    while (threadBack_.hdlFinishedTasks() == 0);  // clear all threads
}

#define ABNORMAL
// ***********************************************************************************************
TEST_F(THREAD_BACK_TEST, emptyThreadList_ok)
{
    size_t nHandled = threadBack_.hdlFinishedTasks();
    EXPECT_EQ(0u, nHandled);
}
TEST_F(THREAD_BACK_TEST, invalid_msgSelf_entryFN_backFN)
{
    EXPECT_FALSE(threadBack_.newTaskOK(
        [] { return true; },  // entryFn
        viaMsgSelf([](bool) {}, nullptr)  // invalid since msgSelf==nullptr
    ));
    EXPECT_FALSE(threadBack_.newTaskOK(
        [] { return true; },  // entryFn
        viaMsgSelf(nullptr, msgSelf_)  // invalid since backFn==nullptr
    ));
    EXPECT_FALSE(threadBack_.newTaskOK(
        MT_TaskEntryFN(nullptr),  // invalid since entryFn==nullptr
        [](bool) {}  // backFn
    ));
    EXPECT_EQ(0, threadBack_.nThread());
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
                EXPECT_EQ("a", *(static_pointer_cast<string>(aMsg).get()));
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
                EXPECT_EQ(2, *(static_pointer_cast<int>(aMsg).get()));
                cb_info.emplace("REQ: 2's Q hdlr via MsgSelf");
            }
        );
    });
    EXPECT_EQ(2u, mt_getQ().nHdlr())  << "REQ: count hdlr";

    // push
    threadBack_.newTaskOK(
        // entryFn
        [] {
            mt_getQ().mt_push(MAKE_PTR<string>("a"));
            return true;
        },
        // backFn
        viaMsgSelf(  // REQ: via MsgSelf
            [this, &cb_info](bool aRet)
            {
                EXPECT_TRUE(aRet) << "entryFn succ";
                cb_info.emplace("REQ: a's backFn via MsgSelf");
            },
            msgSelf_
        )
    );
    threadBack_.newTaskOK(
        // entryFn
        [] {
            mt_getQ().mt_push(MAKE_PTR<int>(2));
            return true;
        },
        // backFn
        viaMsgSelf(
            [this, &cb_info](bool aRet)
            {
                EXPECT_TRUE(aRet) << "entryFn succ";
                cb_info.emplace("REQ: 2's backFn via MsgSelf");
            },
            msgSelf_
        )
    );

    // simulate main()
    const set<string> expect = {"REQ: a's Q hdlr via MsgSelf", "REQ: a's backFn via MsgSelf",
        "REQ: 2's Q hdlr via MsgSelf", "REQ: 2's backFn via MsgSelf"};
    for (;;)
    {
        // handle all done Thread
        INF("nMsg=" << msgSelf_->nMsg() << ", nQ=" << mt_getQ().mt_size(true) << ", nTh=" << threadBack_.nThread());
        threadBack_.hdlFinishedTasks();

        // handle all existing in mt_getQ()
        INF("nMsg=" << msgSelf_->nMsg() << ", nQ=" << mt_getQ().mt_size(true) << ", nTh=" << threadBack_.nThread());
        mt_getQ().handleAllEle();

        INF("nMsg=" << msgSelf_->nMsg() << ", nQ=" << mt_getQ().mt_size(true) << ", nTh=" << threadBack_.nThread());
        msgSelf_->handleAllMsg();

        INF("nMsg=" << msgSelf_->nMsg() << ", nQ=" << mt_getQ().mt_size(true) << ", nTh=" << threadBack_.nThread());
        if (expect == cb_info)
            return;

        INF("nMsg=" << msgSelf_->nMsg() << ", nQ=" << mt_getQ().mt_size(true) << ", nTh=" << threadBack_.nThread());
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
