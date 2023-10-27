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
    EXPECT_EQ(0u, msgSelf_->nMsg()) << "REQ: init states" << endl;

    msgSelf_->newMsg(d1MsgHdlr_);
    EXPECT_EQ(1u, msgSelf_->nMsg(EMsgPri_NORM));
    EXPECT_EQ(1u, msgSelf_->nMsg());
    EXPECT_EQ(queue<int>(), hdlrIDs_) << "REQ: not immediate call d1MsgHdlr_ but wait msg-to-self" << endl;

    msgSelf_->handleAllMsg(msgSelf_->getValid());  // simulate main() callback
    EXPECT_EQ(queue<int>({1}), hdlrIDs_) << "REQ: call d1MsgHdlr_" << endl;
    EXPECT_EQ(0u, msgSelf_->nMsg(EMsgPri_NORM));
    EXPECT_FALSE(msgSelf_->nMsg());
}
TEST_F(MsgSelfTest, dupSendMsg)
{
    msgSelf_->newMsg(d1MsgHdlr_);
    msgSelf_->newMsg(d1MsgHdlr_);                 // req: dup
    EXPECT_EQ(2u, msgSelf_->nMsg(EMsgPri_NORM));
    EXPECT_EQ(queue<int>(), hdlrIDs_);

    msgSelf_->handleAllMsg(msgSelf_->getValid());
    EXPECT_EQ(queue<int>({1, 1}), hdlrIDs_);      // req: call all
    EXPECT_EQ(0u, msgSelf_->nMsg(EMsgPri_NORM));
}
TEST_F(MsgSelfTest, sendInvalidMsg_noCrash)
{
    msgSelf_->newMsg(d1MsgHdlr_);  // valid cb to get pongMainFN_
    msgSelf_->handleAllMsg(msgSelf_->getValid());
    EXPECT_EQ(queue<int>({1}), hdlrIDs_);

    msgSelf_->newMsg(MsgCB());  // req: invalid cb
    msgSelf_->handleAllMsg(msgSelf_->getValid());  // req: no crash (& inc cov of empty queue)
    EXPECT_EQ(queue<int>({1}), hdlrIDs_);
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

    msgSelf_->handleAllMsg(msgSelf_->getValid());
    EXPECT_EQ(0u, msgSelf_->nMsg(EMsgPri_NORM)) << "REQ: all normal" << endl;
    EXPECT_EQ(0u, msgSelf_->nMsg(EMsgPri_HIGH)) << "REQ: all high" << endl;
    EXPECT_EQ(2u, msgSelf_->nMsg(EMsgPri_LOW))  << "REQ: 1/loopback()" << endl;

    msgSelf_->handleAllMsg(msgSelf_->getValid());
    EXPECT_EQ(1u, msgSelf_->nMsg(EMsgPri_LOW));

    msgSelf_->handleAllMsg(msgSelf_->getValid());
    EXPECT_EQ(0u, msgSelf_->nMsg(EMsgPri_LOW));

    msgSelf_->handleAllMsg(msgSelf_->getValid());
    EXPECT_EQ(0u, msgSelf_->nMsg(EMsgPri_LOW));
}

#define PRI
// ***********************************************************************************************
TEST_F(MsgSelfTest, GOLD_highPriority_first)
{
    msgSelf_->newMsg(d1MsgHdlr_);
    msgSelf_->newMsg(d2MsgHdlr_, EMsgPri_HIGH);

    msgSelf_->handleAllMsg(msgSelf_->getValid());
    EXPECT_EQ(queue<int>({2, 1}), hdlrIDs_);
}
TEST_F(MsgSelfTest, GOLD_samePriority_fifo)
{
    msgSelf_->newMsg(d1MsgHdlr_);
    msgSelf_->newMsg(d2MsgHdlr_, EMsgPri_HIGH);
    msgSelf_->newMsg(d3MsgHdlr_);
    msgSelf_->newMsg(d4MsgHdlr_, EMsgPri_HIGH);

    msgSelf_->handleAllMsg(msgSelf_->getValid());
    EXPECT_EQ(queue<int>({2, 4, 1, 3}), hdlrIDs_);
}
TEST_F(MsgSelfTest, newHighPri_first)
{
    msgSelf_->newMsg(d1MsgHdlr_);
    msgSelf_->newMsg(d5MsgHdlr_, EMsgPri_HIGH);
    msgSelf_->newMsg(d3MsgHdlr_);
    msgSelf_->newMsg(d4MsgHdlr_, EMsgPri_HIGH);

    msgSelf_->handleAllMsg(msgSelf_->getValid());
    EXPECT_EQ(queue<int>({5, 4, 2, 1, 3}), hdlrIDs_);
}

#define DESTRUCT_MSGSELF
// ***********************************************************************************************
TEST_F(MsgSelfTest, destructMsgSelf_noCallback_noMemLeak_noCrash)  // mem leak is checked by valgrind upon UT
{
    EXPECT_CALL(*this, d6MsgHdlr()).Times(0);  // req: no call
    msgSelf_->newMsg([&](){ this->d6MsgHdlr(); });
    EXPECT_EQ(1u, msgSelf_->nMsg(EMsgPri_NORM));

    auto valid = msgSelf_->getValid();
    msgSelf_.reset();  // rm msgSelf
    msgSelf_->handleAllMsg(valid);  // req: no crash
}

#define WAIT_NOTIFY
// ***********************************************************************************************
TEST_F(MsgSelfTest, wait_notify)
{
    auto now = high_resolution_clock::now();
    msgSelf_->newMsg(d1MsgHdlr_);
    g_sem.mt_timedwait();  // REQ: 1 msg will wakeup mt_timedwait()
    auto dur = duration_cast<std::chrono::milliseconds>(high_resolution_clock::now() - now);
    EXPECT_LT(dur.count(), 100) << "REQ: newMsg() shall notify";

    msgSelf_->newMsg(d1MsgHdlr_);
    msgSelf_->newMsg(d1MsgHdlr_);  // req: dup
    g_sem.mt_timedwait();  // REQ: multi-msg will wakeup mt_timedwait()

    msgSelf_->handleAllMsg(msgSelf_->getValid());  // clean msg queue
}

}  // namespace
