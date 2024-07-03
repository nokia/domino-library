/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include <chrono>
#include <future>
#include <gtest/gtest.h>
#include <set>
#include <thread>
#include <unistd.h>
#include <unordered_map>

#include "MsgSelf.hpp"
#include "MT_PingMainTH.hpp"
#include "MtInQueue.hpp"
#include "ThreadBackViaMsgSelf.hpp"
#include "UniLog.hpp"

using namespace std::chrono;
using namespace testing;

namespace RLib {
// ***********************************************************************************************
struct MT_SemaphoreTest : public Test, public UniLog
{
    MT_SemaphoreTest()
        : UniLog(UnitTest::GetInstance()->current_test_info()->name())
    {}
    void SetUp() override
    {
        mt_getQ().mt_clear();  // avoid other case interfere
    }
    ~MT_SemaphoreTest() { GTEST_LOG_FAIL }

    shared_ptr<MsgSelf> msgSelf_ = make_shared<MsgSelf>(uniLogName());
};

#define INTEGRATION
// ***********************************************************************************************
TEST_F(MT_SemaphoreTest, GOLD_integrate_MsgSelf_ThreadBack_MtInQueue)  // simulate real world
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
    ThreadBack::newThread(
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
    ThreadBack::newThread(
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
        INF("nMsg=" << msgSelf_->nMsg() << ", nQ=" << mt_getQ().mt_size(true) << ", nTh=" << ThreadBack::nThread());
        ThreadBack::hdlFinishedThreads();

        // handle all existing in mt_getQ()
        INF("nMsg=" << msgSelf_->nMsg() << ", nQ=" << mt_getQ().mt_size(true) << ", nTh=" << ThreadBack::nThread());
        mt_getQ().handleAllEle();

        INF("nMsg=" << msgSelf_->nMsg() << ", nQ=" << mt_getQ().mt_size(true) << ", nTh=" << ThreadBack::nThread());
        msgSelf_->handleAllMsg();

        INF("nMsg=" << msgSelf_->nMsg() << ", nQ=" << mt_getQ().mt_size(true) << ", nTh=" << ThreadBack::nThread());
        if (expect == cb_info)
            return;

        INF("nMsg=" << msgSelf_->nMsg() << ", nQ=" << mt_getQ().mt_size(true) << ", nTh=" << ThreadBack::nThread());
        timedwait();
    }
}
TEST_F(MT_SemaphoreTest, invalid_msgSelf)
{
    ThreadBack::newThread(
        [] { return true; },  // entryFn
        viaMsgSelf([](bool) {}, nullptr)  // invalid since msgSelf==nullptr
    );
    ThreadBack::newThread(
        [] { return true; },  // entryFn
        viaMsgSelf(nullptr, msgSelf_)  // invalid since backFn==nullptr
    );
    ThreadBack::newThread(
        MT_ThreadEntryFN(nullptr),  // invalid since entryFn==nullptr
        [](bool) {}  // backFn
    );
    EXPECT_EQ(0, ThreadBack::nThread());
}

// ***********************************************************************************************
TEST_F(MT_SemaphoreTest, timeout)
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
