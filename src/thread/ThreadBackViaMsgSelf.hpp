/**
 * Copyright 2023 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - VALUE/why:
//   . align with MsgSelf since all "msg" shall queue in MsgSelf
//   . viaMsgSelf() out of ThreadBack so ThreadBack can standalone without MsgSelf
//   . same for MsgSelf
//   . avoid block main thread
//
// - mem safe: yes
// ***********************************************************************************************
#include "MsgSelf.hpp"
#include "ThreadBack.hpp"

namespace RLib
{
// ***********************************************************************************************
// wrap TaskBackFN to queue in MsgSelf
inline
TaskBackFN viaMsgSelf(const TaskBackFN& aBackFN, std::shared_ptr<MsgSelf> aMsgSelf, EMsgPriority aPri = EMsgPri_NORM)
{
    return ! aBackFN || aMsgSelf == nullptr
        ? TaskBackFN(nullptr)  // empty fn
        : [aBackFN, aMsgSelf, aPri](bool aRet)  // must cp aBackFN since lambda run later in diff lifecycle
        {
            aMsgSelf->newMsg(bind(aBackFN, aRet), aPri);  // wrap aBackFN to queue in MsgSelf
        };
}

}  // namespace
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2023-09-15  CSZ       1)create
// ***********************************************************************************************
