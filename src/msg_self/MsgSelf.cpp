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
MsgSelf::MsgSelf(const PingMainFN& aPingMainFN, const UniLogName& aUniLogName)
    : UniLog(aUniLogName)
    , pingMainFN_(aPingMainFN)
{}

// ***********************************************************************************************
MsgSelf::~MsgSelf()
{
    *isValid_ = false;
    if (nMsg_)
        DBG("discard nMsg=" << nMsg_);
}

// ***********************************************************************************************
void MsgSelf::handleAllMsg(const shared_ptr<bool> aValidMsgSelf)
{
    if (*aValidMsgSelf)          // impossible aValidMsgSelf==nullptr till 022-Mar-11
    {
        // may be called after MsgSelf destructed, not use this at all
        HID("(MsgSelf) How many reference to this MsgSelf? " << aValidMsgSelf.use_count());

        if (not hasMsg())
            return;
        while (handleOneMsg());  // handleOneMsg() may create new high priority msg(s)
    }
}

// ***********************************************************************************************
bool MsgSelf::handleOneMsg()
{
    for (auto msgPri = EMsgPri_MAX-1; msgPri >= EMsgPri_MIN ; msgPri--)
    {
        auto&& oneQueue = msgQueues_[msgPri];
        if (oneQueue.empty())
            continue;

        oneQueue.front()();  // run 1st MsgCB; newMsg() prevent nullptr into msgQueues_
        oneQueue.pop();
        --nMsg_;

        if (not hasMsg())
            return false;
        if (isLowPri(EMsgPriority(msgPri)))
        {
            pingMainFN_([this, isValid = isValid_]() mutable { handleAllMsg(isValid); });
            return false;  // 1 low msg per loopReq so queue with eg IM CB, syscom, etc. (lower)
        }
        return true;
    }
    return false;
}

// ***********************************************************************************************
void MsgSelf::newMsg(const MsgCB& aMsgCB, const EMsgPriority aMsgPri)
{
    if (aMsgCB == nullptr)
    {
        WRN("(MsgSelf) failed!!! aMsgCB=nullptr doesn't make sense.");
        return;
    }

    msgQueues_[aMsgPri].push(aMsgCB);
    ++nMsg_;
    if (nMsg_ > 1)
        return;

    pingMainFN_([this, isValid = isValid_]() mutable { handleAllMsg(isValid); });
}
}  // namespace
