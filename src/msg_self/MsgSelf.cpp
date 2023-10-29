/**
 * Copyright 2016 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include "MsgSelf.hpp"
#include "MT_PingMainTH.hpp"

namespace RLib
{
// ***********************************************************************************************
MsgSelf::MsgSelf(const UniLogName& aUniLogName)
    : UniLog(aUniLogName)
{}

// ***********************************************************************************************
MsgSelf::~MsgSelf()
{
    *isValid_ = false;
    if (nMsg_)
        WRN("discard nMsg=" << nMsg_);
}

// ***********************************************************************************************
void MsgSelf::handleAllMsg(const shared_ptr<bool> aValidMsgSelf)
{
    HID("(MsgSelf) How many reference to this MsgSelf? " << aValidMsgSelf.use_count());

    if (*aValidMsgSelf)  // impossible aValidMsgSelf==nullptr since 022-Mar-11
    {
        // if (not nMsg()) return;  // rare nMsg_==0 when PongMainFN()
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
        oneQueue.pop_front();
        --nMsg_;

        if (not nMsg())
            return false;    // no more to continue
        if (isLowPri(EMsgPriority(msgPri)))
        {
            mt_pingMainTH();
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

    msgQueues_[aMsgPri].push_back(aMsgCB);
    ++nMsg_;

    mt_pingMainTH();
}
}  // namespace
