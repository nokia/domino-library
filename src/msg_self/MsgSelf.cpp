/**
 * Copyright 2016-2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include "MsgSelf.hpp"

namespace RLib
{
// ***********************************************************************************************
MsgSelf::MsgSelf(LoopReqFUNC aFunc)
    : loopReq_(aFunc)
    , log_("MsgSelf")
{}

// ***********************************************************************************************
MsgSelf::~MsgSelf()
{
    *isValid_ = false;
    if (nMsg_) DBG("Succeed, & discard nMsg=" << nMsg_);
}

// ***********************************************************************************************
bool MsgSelf::handleOneMsg()
{
    for (auto&& priority = EMsgPri_MAX-1; priority >=EMsgPri_MIN ; priority--)
    {
        auto&& oneQueue = msgQueues_[priority];
        if (oneQueue.empty()) continue;

        auto&& hdlr = oneQueue.front().lock();
        if (hdlr && *hdlr) (*hdlr)();
        oneQueue.pop();
        --nMsg_;
        HID("after call, nHdlrRef=" << hdlr.use_count());

        if (!isLowPri(EMsgPriority(priority))) return true;
        if (hasMsg()) loopReq_([this, isValid = isValid_]() mutable { loopBack(isValid); });
        return false;  // 1 low msg per loopReq so queue with eg IM CB, syscom, etc. (lower)
    }
    return false;
}

// ***********************************************************************************************
void MsgSelf::loopBack(const std::shared_ptr<bool> aValidMsgSelf)
{
    if (*aValidMsgSelf)  // impossible aValidMsgSelf==nullptr till 022-Mar-11
    {
        // may be called after MsgSelf destructed, not use this at all
        HID("How many reference to this MsgSelf? " << aValidMsgSelf.use_count());

        if (not hasMsg()) return;
        while (handleOneMsg()) {}  // handleOneMsg() may create new high priority msg(s)
    }
}

// ***********************************************************************************************
void MsgSelf::newMsg(const WeakMsgCB& aMsgCB, const EMsgPriority aPriority)
{
    msgQueues_[aPriority].push(aMsgCB);
    ++nMsg_;
    if (nMsg_ > 1) return;

    loopReq_([this, isValid = isValid_]() mutable { loopBack(isValid); });
}
}  // namespace
