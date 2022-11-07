/**
 * Copyright 2016-2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>  // for shared_ptr
#include <queue>

#include "MsgSelf.hpp"

using namespace testing;

namespace RLib
{
// ***********************************************************************************************
struct MsgSelfTest : public Test, public UniLog
{
    MsgSelfTest() : UniLog(UnitTest::GetInstance()->current_test_info()->name()) {}
    ~MsgSelfTest() { GTEST_LOG_FAIL }

    MOCK_METHOD(void, toMain, (const FromMainFN&));  // simulate toMain() which may send msg to self

    // -------------------------------------------------------------------------------------------
    shared_ptr<MsgSelf> msgSelf_ = make_shared<MsgSelf>(bind(&MsgSelfTest::toMain, this, placeholders::_1), uniLogName());
    FromMainFN fromMainFN_;

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
    EXPECT_EQ(0u, msgSelf_->nMsg(EMsgPri_NORM));  // req: init states
    EXPECT_FALSE(msgSelf_->hasMsg());

    EXPECT_CALL(*this, toMain(_)).WillOnce(SaveArg<0>(&fromMainFN_));  // req: newMsg->toMain that eg send msg to self
    msgSelf_->newMsg(d1MsgHdlr_);
    EXPECT_EQ(1u, msgSelf_->nMsg(EMsgPri_NORM));
    EXPECT_TRUE(msgSelf_->hasMsg());
    EXPECT_EQ(queue<int>(), hdlrIDs_);            // req: not immediate call d1MsgHdlr_ but wait msg-to-self

    EXPECT_CALL(*this, toMain(_)).Times(0);       // req: no more no call
    fromMainFN_();                                // simulate eg msg-to-self received, then call fromMainFN_()
    EXPECT_EQ(queue<int>({1}), hdlrIDs_);         // req: call d1MsgHdlr_
    EXPECT_EQ(0u, msgSelf_->nMsg(EMsgPri_NORM));
    EXPECT_FALSE(msgSelf_->hasMsg());
}
TEST_F(MsgSelfTest, dupSendMsg)
{
    EXPECT_CALL(*this, toMain(_)).WillOnce(SaveArg<0>(&fromMainFN_));
    msgSelf_->newMsg(d1MsgHdlr_);
    msgSelf_->newMsg(d1MsgHdlr_);                 // req: dup
    EXPECT_EQ(2u, msgSelf_->nMsg(EMsgPri_NORM));
    EXPECT_EQ(queue<int>(), hdlrIDs_);

    EXPECT_CALL(*this, toMain(_)).Times(0);
    fromMainFN_();
    EXPECT_EQ(queue<int>({1, 1}), hdlrIDs_);      // req: call all
    EXPECT_EQ(0u, msgSelf_->nMsg(EMsgPri_NORM));
}
TEST_F(MsgSelfTest, sendInvalidMsg_noCrash)
{
    EXPECT_CALL(*this, toMain(_)).WillOnce(SaveArg<0>(&fromMainFN_));
    msgSelf_->newMsg(MsgCB());                    // invalid cb
    fromMainFN_();
    EXPECT_EQ(queue<int>(), hdlrIDs_);
}
TEST_F(MsgSelfTest, GOLD_loopback_handleAll_butOneByOneLowPri)
{
    EXPECT_CALL(*this, toMain(_)).WillOnce(SaveArg<0>(&fromMainFN_));
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

    EXPECT_CALL(*this, toMain(_)).WillOnce(SaveArg<0>(&fromMainFN_));
    fromMainFN_();
    EXPECT_EQ(0u, msgSelf_->nMsg(EMsgPri_NORM));  // req: all normal
    EXPECT_EQ(0u, msgSelf_->nMsg(EMsgPri_HIGH));  // req: all high
    EXPECT_EQ(2u, msgSelf_->nMsg(EMsgPri_LOW));   // req: 1/loopback()

    EXPECT_CALL(*this, toMain(_)).WillOnce(SaveArg<0>(&fromMainFN_));
    fromMainFN_();
    EXPECT_EQ(1u, msgSelf_->nMsg(EMsgPri_LOW));

    EXPECT_CALL(*this, toMain(_)).Times(0);       // req: last low need not trigger toMain()
    fromMainFN_();
    EXPECT_EQ(0u, msgSelf_->nMsg(EMsgPri_LOW));

    EXPECT_CALL(*this, toMain(_)).Times(0);       // req: no msg shall not trigger toMain()
    fromMainFN_();
    EXPECT_EQ(0u, msgSelf_->nMsg(EMsgPri_LOW));
}

#define PRI
// ***********************************************************************************************
TEST_F(MsgSelfTest, GOLD_highPriority_first)
{
    EXPECT_CALL(*this, toMain(_)).WillOnce(SaveArg<0>(&fromMainFN_));
    msgSelf_->newMsg(d1MsgHdlr_);
    msgSelf_->newMsg(d2MsgHdlr_, EMsgPri_HIGH);
    fromMainFN_();

    EXPECT_EQ(queue<int>({2, 1}), hdlrIDs_);
}
TEST_F(MsgSelfTest, GOLD_samePriority_fifo)
{
    EXPECT_CALL(*this, toMain(_)).WillOnce(SaveArg<0>(&fromMainFN_));
    msgSelf_->newMsg(d1MsgHdlr_);
    msgSelf_->newMsg(d2MsgHdlr_, EMsgPri_HIGH);
    msgSelf_->newMsg(d3MsgHdlr_);
    msgSelf_->newMsg(d4MsgHdlr_, EMsgPri_HIGH);
    fromMainFN_();

    EXPECT_EQ(queue<int>({2, 4, 1, 3}), hdlrIDs_);
}
TEST_F(MsgSelfTest, newHighPri_first)
{
    EXPECT_CALL(*this, toMain(_)).WillOnce(SaveArg<0>(&fromMainFN_));
    msgSelf_->newMsg(d1MsgHdlr_);
    msgSelf_->newMsg(d5MsgHdlr_, EMsgPri_HIGH);
    msgSelf_->newMsg(d3MsgHdlr_);
    msgSelf_->newMsg(d4MsgHdlr_, EMsgPri_HIGH);
    fromMainFN_();

    EXPECT_EQ(queue<int>({5, 4, 2, 1, 3}), hdlrIDs_);
}

#define DESTRUCT_MSGSELF
// ***********************************************************************************************
TEST_F(MsgSelfTest, invalidMsgSelf_fromMainFnShallNotCrash)
{
    EXPECT_CALL(*this, toMain(_)).WillOnce(SaveArg<0>(&fromMainFN_));
    msgSelf_->newMsg(d1MsgHdlr_);
    auto valid = msgSelf_->getValid();

    EXPECT_EQ(1, msgSelf_.use_count());
    msgSelf_.reset();                                      // rm msgSelf
    EXPECT_FALSE(*valid);

    fromMainFN_();                                         // req: no crash
}
TEST_F(MsgSelfTest, destructMsgSelf_noCallback_noMemLeak)  // mem leak is checked by valgrind upon UT
{
    EXPECT_CALL(*this, toMain(_)).WillOnce(SaveArg<0>(&fromMainFN_));
    EXPECT_CALL(*this, d6MsgHdlr()).Times(0);              // req: no call
    msgSelf_->newMsg(bind(&MsgSelfTest::d6MsgHdlr, this));
    EXPECT_EQ(1u, msgSelf_->nMsg(EMsgPri_NORM));
    msgSelf_.reset();                                      // rm msgSelf
}
}  // namespace
