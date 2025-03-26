/**
 * Copyright 2016 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include "MsgSelf.hpp"
#include "MT_PingMainTH.hpp"

namespace rlib
{
// ***********************************************************************************************
bool MsgSelf::handleOneMsg_() noexcept
{
    for (auto msgPri = EMsgPri_MAX-1; msgPri >= EMsgPri_MIN ; msgPri--)
    {
        auto&& oneQueue = msgQueues_[msgPri];
        if (oneQueue.empty())
            continue;

        try { oneQueue.front()(); } // run 1st MsgCB; newMsgOK() prevent nullptr into msgQueues_
        catch(...) { ERR("(MsgSelf) except->failed!!!"); }

        oneQueue.pop_front();  // except can't recover->terminate
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
bool MsgSelf::newMsgOK(const MsgCB& aMsgCB, const EMsgPriority aMsgPri) noexcept
{
    // validate
    if (! aMsgCB)
    {
        WRN("(MsgSelf) failed!!! aMsgCB=nullptr doesn't make sense.");
        return false;
    }
    if (aMsgPri >= EMsgPri_MAX)
    {
        WRN("(MsgSelf) failed!!! outbound aMsgPri=" << aMsgPri);
        return false;
    }

    // store
    msgQueues_[aMsgPri].emplace_back(aMsgCB);  // except eg bad_alloc: can't recover->terminate
    ++nMsg_;
    HID("(MsgSelf) nMsg=" << nMsg_);

    // ping main thread
    mt_pingMainTH();
    return true;
}

}  // namespace
