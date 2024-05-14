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
// - performance optimization need 5 lines code, with a little benfit in most cases
//   . so giveup until real need
bool MsgSelf::handleOneMsg_()
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
    // validate
    if (! aMsgCB)
    {
        WRN("(MsgSelf) failed!!! aMsgCB=nullptr doesn't make sense.");
        return;
    }
    if (aMsgPri >= EMsgPri_MAX)
    {
        WRN("(MsgSelf) failed!!! outbound aMsgPri=" << aMsgPri);
        return;
    }

    // store
    msgQueues_[aMsgPri].emplace_back(aMsgCB);
    ++nMsg_;
    HID("(MsgSelf) nMsg=" << nMsg_);

    // ping main thread
    mt_pingMainTH();
}

}  // namespace
