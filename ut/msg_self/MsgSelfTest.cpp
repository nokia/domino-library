/**
 * Copyright 2016-2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include <memory>  // for shared_ptr
#include <queue>
#include <gtest/gtest.h>

#include "MsgSelf.hpp"

using namespace testing;

namespace RLib
{
// ***********************************************************************************************
struct MsgSelfTests : public Test
{
    MsgSelfTests()
    {
        *d1MsgHdlr_ = [this](){ hdlrIDs_.push(1); };
        *d2MsgHdlr_ = [this](){ hdlrIDs_.push(2); };
        *d3MsgHdlr_ = [this](){ hdlrIDs_.push(3); };
        *d4MsgHdlr_ = [this](){ hdlrIDs_.push(4); };
        *d5MsgHdlr_ = [this](){ hdlrIDs_.push(5); msgSelf_->newMsg(d2MsgHdlr_, EMsgPri_HIGH); };
    }

    // -------------------------------------------------------------------------------------------
    std::shared_ptr<MsgSelf> msgSelf_ = std::make_shared<MsgSelf>([this](LoopBackFUNC aFunc){ loopbackFunc_ = aFunc; });
    LoopBackFUNC loopbackFunc_;

    SharedMsgCB nullMsgHdlr_ = std::make_shared<MsgCB>();
    SharedMsgCB d1MsgHdlr_ = std::make_shared<MsgCB>();
    SharedMsgCB d2MsgHdlr_ = std::make_shared<MsgCB>();
    SharedMsgCB d3MsgHdlr_ = std::make_shared<MsgCB>();
    SharedMsgCB d4MsgHdlr_ = std::make_shared<MsgCB>();
    SharedMsgCB d5MsgHdlr_ = std::make_shared<MsgCB>();

    std::queue<int> hdlrIDs_;
};

#define SEND_MSG
// ***********************************************************************************************
TEST_F(MsgSelfTests, GOLD_sendMsg)
{
    EXPECT_EQ(0u, msgSelf_->nMsg(EMsgPri_NORM));
    EXPECT_FALSE(msgSelf_->hasMsg());

    msgSelf_->newMsg(d1MsgHdlr_);               // req: send msg
    EXPECT_EQ(1u, msgSelf_->nMsg(EMsgPri_NORM));
    EXPECT_TRUE(msgSelf_->hasMsg());

    loopbackFunc_();
    EXPECT_EQ(std::queue<int>({1}), hdlrIDs_);  // req: callback
    EXPECT_EQ(0u, msgSelf_->nMsg(EMsgPri_NORM));
    EXPECT_FALSE(msgSelf_->hasMsg());
}
TEST_F(MsgSelfTests, dupSendMsg)
{
    msgSelf_->newMsg(d1MsgHdlr_);
    msgSelf_->newMsg(d1MsgHdlr_);                  // req: dup
    EXPECT_EQ(2u, msgSelf_->nMsg(EMsgPri_NORM));
    EXPECT_TRUE(msgSelf_->hasMsg());

    loopbackFunc_();
    EXPECT_EQ(std::queue<int>({1, 1}), hdlrIDs_);  // req: callback
    EXPECT_EQ(0u, msgSelf_->nMsg(EMsgPri_NORM));
    EXPECT_FALSE(msgSelf_->hasMsg());
}
TEST_F(MsgSelfTests, sendInvalidMsg_noCrash)
{
    msgSelf_->newMsg(WeakMsgCB());
    msgSelf_->newMsg(nullMsgHdlr_);
    loopbackFunc_();
}
TEST_F(MsgSelfTests, GOLD_loopback_handleAll_butOneByOneLowPri)
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

    loopbackFunc_();
    EXPECT_EQ(0u, msgSelf_->nMsg(EMsgPri_NORM));  // req: all normal
    EXPECT_EQ(0u, msgSelf_->nMsg(EMsgPri_HIGH));  // req: all high
    EXPECT_EQ(2u, msgSelf_->nMsg(EMsgPri_LOW));   // req: 1/loopback()

    loopbackFunc_();
    EXPECT_EQ(1u, msgSelf_->nMsg(EMsgPri_LOW));

    loopbackFunc_();
    EXPECT_EQ(0u, msgSelf_->nMsg(EMsgPri_LOW));

    loopbackFunc_();                         // req: can loopBack w/o any msg
    EXPECT_EQ(0u, msgSelf_->nMsg(EMsgPri_LOW));
}

#define PRI
// ***********************************************************************************************
TEST_F(MsgSelfTests, GOLD_highPriority_first)
{
    msgSelf_->newMsg(d1MsgHdlr_);
    msgSelf_->newMsg(d2MsgHdlr_, EMsgPri_HIGH);
    loopbackFunc_();

    EXPECT_EQ(std::queue<int>({2, 1}), hdlrIDs_);
}
TEST_F(MsgSelfTests, GOLD_samePriority_fifo)
{
    msgSelf_->newMsg(d1MsgHdlr_);
    msgSelf_->newMsg(d2MsgHdlr_, EMsgPri_HIGH);
    msgSelf_->newMsg(d3MsgHdlr_);
    msgSelf_->newMsg(d4MsgHdlr_, EMsgPri_HIGH);
    loopbackFunc_();

    EXPECT_EQ(std::queue<int>({2, 4, 1, 3}), hdlrIDs_);
}
TEST_F(MsgSelfTests, newHighPri_first)
{
    msgSelf_->newMsg(d1MsgHdlr_);
    msgSelf_->newMsg(d5MsgHdlr_, EMsgPri_HIGH);
    msgSelf_->newMsg(d3MsgHdlr_);
    msgSelf_->newMsg(d4MsgHdlr_, EMsgPri_HIGH);
    loopbackFunc_();

    EXPECT_EQ(std::queue<int>({5, 4, 2, 1, 3}), hdlrIDs_);
}

#define INVALID_MSGSELF
// ***********************************************************************************************
TEST_F(MsgSelfTests, invalidMsgSelf_callbackShallNotCrash)
{
    msgSelf_->newMsg(d1MsgHdlr_);
    auto valid = msgSelf_->getValid();

    EXPECT_EQ(1, msgSelf_.use_count());
    msgSelf_.reset();  // rm msgSelf
    EXPECT_FALSE(*valid);

    loopbackFunc_();   // req: no crash
}
}  // namespace
