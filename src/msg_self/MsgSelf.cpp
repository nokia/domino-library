/**
 * Copyright 2016 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include "MsgSelf.hpp"

namespace RLib
{
// ***********************************************************************************************
MsgSelf::MsgSelf(const ToMainFN& aToMainFN, const UniLogName& aUniLogName)
    : UniLog(aUniLogName)
    , toMainFN_(aToMainFN)
{}

// ***********************************************************************************************
MsgSelf::~MsgSelf()
{
    *isValid_ = false;
    if (nMsg_) DBG("discard nMsg=" << nMsg_);
}

// ***********************************************************************************************
void MsgSelf::handleAllMsg(const shared_ptr<bool> aValidMsgSelf)
{
    if (*aValidMsgSelf)          // impossible aValidMsgSelf==nullptr till 022-Mar-11
    {
        // may be called after MsgSelf destructed, not use this at all
        HID("(MsgSelf) How many reference to this MsgSelf? " << aValidMsgSelf.use_count());

        if (not hasMsg()) return;
        while (handleOneMsg());  // handleOneMsg() may create new high priority msg(s)
    }
}

// ***********************************************************************************************
bool MsgSelf::handleOneMsg()
{
    for (auto&& priority = EMsgPri_MAX-1; priority >=EMsgPri_MIN ; priority--)
    {
        auto&& oneQueue = msgQueues_[priority];
        if (oneQueue.empty()) continue;

        auto&& cb = oneQueue.front();
        if (cb) cb();
        oneQueue.pop();
        --nMsg_;

        if (not isLowPri(EMsgPriority(priority))) return true;
        if (hasMsg()) toMainFN_([this, isValid = isValid_]() mutable { handleAllMsg(isValid); });
        return false;  // 1 low msg per loopReq so queue with eg IM CB, syscom, etc. (lower)
    }
    return false;
}

// ***********************************************************************************************
void MsgSelf::newMsg(const MsgCB& aMsgCB, const EMsgPriority aPriority)
{
    msgQueues_[aPriority].push(aMsgCB);
    ++nMsg_;
    if (nMsg_ > 1) return;

    toMainFN_([this, isValid = isValid_]() mutable { handleAllMsg(isValid); });
}
}  // namespace
