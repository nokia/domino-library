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
    HID("(MsgSelf) How many reference to this MsgSelf? " << aValidMsgSelf.use_count());

    if (*aValidMsgSelf)  // impossible aValidMsgSelf==nullptr since 022-Mar-11
    {
        // if (not hasMsg()) return;  // rare nMsg_==0 when PongMainFN()
        while (handleOneMsg());  // handleOneMsg() may create new high priority msg(s)
    }
}

// ***********************************************************************************************
// - performance optimization need 5 lines code, with a little benfit in most cases
//   . so giveup until real need
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
            return false;    // no more to continue
        if (isLowPri(EMsgPriority(msgPri)))
        {
            pingMainFN_([this, isValid = isValid_]() mutable { handleAllMsg(isValid); });
            return false;    // not continue for low priority until next ping-pong
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

    if (nMsg_ == 1)  // ist MsgCB
        pingMainFN_([this, isValid = isValid_]() mutable { handleAllMsg(isValid); });
}
}  // namespace
