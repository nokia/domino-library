/**
 * Copyright 2016-2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include <chrono>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>  // for shared_ptr
#include <queue>

#include "MsgSelf.hpp"
#include "MT_PingMainTH.hpp"

using namespace std::chrono;
using namespace testing;

namespace RLib
{
// ***********************************************************************************************
struct MsgSelfTest : public Test, public UniLog
{
    MsgSelfTest() : UniLog(UnitTest::GetInstance()->current_test_info()->name()) {}
    ~MsgSelfTest() { GTEST_LOG_FAIL }

    // -------------------------------------------------------------------------------------------
    shared_ptr<MsgSelf> msgSelf_ = make_shared<MsgSelf>(uniLogName());

    MsgCB d1MsgHdlr_ = [&](){ hdlrIDs_.push(1); };
    MsgCB d2MsgHdlr_ = [&](){ hdlrIDs_.push(2); };
    MsgCB d3MsgHdlr_ = [&](){ hdlrIDs_.push(3); };
    MsgCB d4MsgHdlr_ = [&](){ hdlrIDs_.push(4); };
    MsgCB d5MsgHdlr_ = [&]()
    {
        hdlrIDs_.push(5);
        msgSelf_->newMsg(d2MsgHdlr_, EMsgPri_HIGH);
    };
    MOCK_METHOD(void, d6MsgHdlr, ());

    queue<int> hdlrIDs_;
};

#define SEND_MSG
// ***********************************************************************************************
TEST_F(MsgSelfTest, GOLD_sendMsg)
{
    EXPECT_EQ(0u, msgSelf_->nMsg()) << "REQ: init states";

    msgSelf_->newMsg(d1MsgHdlr_);
    EXPECT_EQ(1u, msgSelf_->nMsg(EMsgPri_NORM));
    EXPECT_EQ(1u, msgSelf_->nMsg());
    EXPECT_EQ(queue<int>(), hdlrIDs_) << "REQ: not immediate call d1MsgHdlr_ but wait msg-to-self";

    msgSelf_->handleAllMsg();  // simulate main() callback
    EXPECT_EQ(queue<int>({1}), hdlrIDs_) << "REQ: call d1MsgHdlr_";
    EXPECT_EQ(0u, msgSelf_->nMsg(EMsgPri_NORM));
    EXPECT_FALSE(msgSelf_->nMsg());
}
TEST_F(MsgSelfTest, dupSendMsg)
{
    msgSelf_->newMsg(d1MsgHdlr_);
    msgSelf_->newMsg(d1MsgHdlr_);  // dup
    EXPECT_EQ(2u, msgSelf_->nMsg(EMsgPri_NORM));
    EXPECT_EQ(queue<int>(), hdlrIDs_);

    msgSelf_->handleAllMsg();
    EXPECT_EQ(queue<int>({1, 1}), hdlrIDs_);  // req: call all
    EXPECT_EQ(0u, msgSelf_->nMsg(EMsgPri_NORM));
}
TEST_F(MsgSelfTest, sendInvalidMsg_noCrash)
{
    msgSelf_->newMsg(MsgCB());  // invalid cb
    msgSelf_->newMsg(d1MsgHdlr_, EMsgPri_MAX);  // invalid priority
    msgSelf_->newMsg(nullptr, EMsgPriority(-1));  // both invalid
    msgSelf_->handleAllMsg();  // req: no crash (& inc cov of empty queue)
    EXPECT_EQ(queue<int>(), hdlrIDs_);
    EXPECT_EQ(0, msgSelf_->nMsg());
}
TEST_F(MsgSelfTest, GOLD_loopback_handleAll_butOneByOneLowPri)
{
    msgSelf_->newMsg(d1MsgHdlr_);
    msgSelf_->newMsg(d1MsgHdlr_);
    msgSelf_->newMsg(d1MsgHdlr_, EMsgPri_HIGH);
    msgSelf_->newMsg(d1MsgHdlr_, EMsgPri_HIGH);
    msgSelf_->newMsg(d1MsgHdlr_, EMsgPri_LOW);
    msgSelf_->newMsg(d1MsgHdlr_, EMsgPri_LOW);
    msgSelf_->newMsg(d1MsgHdlr_, EMsgPri_LOW);
    EXPECT_EQ(2u, msgSelf_->nMsg(EMsgPri_NORM));
    EXPECT_EQ(2u, msgSelf_->nMsg(EMsgPri_HIGH));
    EXPECT_EQ(3u, msgSelf_->nMsg(EMsgPri_LOW));

    msgSelf_->handleAllMsg();
    EXPECT_EQ(0u, msgSelf_->nMsg(EMsgPri_NORM)) << "REQ: all normal";
    EXPECT_EQ(0u, msgSelf_->nMsg(EMsgPri_HIGH)) << "REQ: all high";
    EXPECT_EQ(2u, msgSelf_->nMsg(EMsgPri_LOW))  << "REQ: 1/loopback()";

    msgSelf_->handleAllMsg();
    EXPECT_EQ(1u, msgSelf_->nMsg(EMsgPri_LOW));

    msgSelf_->handleAllMsg();
    EXPECT_EQ(0u, msgSelf_->nMsg(EMsgPri_LOW));

    msgSelf_->handleAllMsg();
    EXPECT_EQ(0u, msgSelf_->nMsg(EMsgPri_LOW));
}

#define PRI
// ***********************************************************************************************
TEST_F(MsgSelfTest, GOLD_highPriority_first)
{
    msgSelf_->newMsg(d1MsgHdlr_);
    msgSelf_->newMsg(d2MsgHdlr_, EMsgPri_HIGH);

    msgSelf_->handleAllMsg();
    EXPECT_EQ(queue<int>({2, 1}), hdlrIDs_);
}
TEST_F(MsgSelfTest, GOLD_samePriority_fifo)
{
    msgSelf_->newMsg(d1MsgHdlr_);
    msgSelf_->newMsg(d2MsgHdlr_, EMsgPri_HIGH);
    msgSelf_->newMsg(d3MsgHdlr_);
    msgSelf_->newMsg(d4MsgHdlr_, EMsgPri_HIGH);

    msgSelf_->handleAllMsg();
    EXPECT_EQ(queue<int>({2, 4, 1, 3}), hdlrIDs_);
}
TEST_F(MsgSelfTest, newHighPri_first)
{
    msgSelf_->newMsg(d1MsgHdlr_);
    msgSelf_->newMsg(d5MsgHdlr_, EMsgPri_HIGH);
    msgSelf_->newMsg(d3MsgHdlr_);
    msgSelf_->newMsg(d4MsgHdlr_, EMsgPri_HIGH);

    msgSelf_->handleAllMsg();
    EXPECT_EQ(queue<int>({5, 4, 2, 1, 3}), hdlrIDs_);
}

#define DESTRUCT_MSGSELF
// ***********************************************************************************************
TEST_F(MsgSelfTest, destructMsgSelf_noCallback_noMemLeak_noCrash)  // mem leak is checked by valgrind upon UT
{
    EXPECT_CALL(*this, d6MsgHdlr()).Times(0);  // REQ: no call
    msgSelf_->newMsg([&](){ this->d6MsgHdlr(); });
    EXPECT_EQ(1u, msgSelf_->nMsg(EMsgPri_NORM));

    msgSelf_.reset();  // rm msgSelf
    INF("~MsgSelf() shall print msg discarded");
}

#define WAIT_NOTIFY
// ***********************************************************************************************
TEST_F(MsgSelfTest, wait_notify)
{
    auto start = high_resolution_clock::now();
    msgSelf_->newMsg(d1MsgHdlr_);
    timedwait(0, 100);  // REQ: 1 msg will wakeup MT_Semaphore::timedwait()
    auto dur = duration_cast<std::chrono::milliseconds>(high_resolution_clock::now() - start);
    EXPECT_LT(dur.count(), 100) << "REQ: newMsg() shall notify instead of timeout";

    msgSelf_->handleAllMsg();  // clear msg queue
}

#define SAFE
// ***********************************************************************************************
TEST_F(MsgSelfTest, invalid_nMsg)
{
    EXPECT_EQ(0, msgSelf_->nMsg(EMsgPri_MAX))      << "REQ: not accept out bound priority";
    EXPECT_EQ(0, msgSelf_->nMsg(EMsgPriority(-1))) << "REQ: not accept out bound priority";
}

}  // namespace
