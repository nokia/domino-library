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
#include <memory>

#include "MsgSelf.hpp"
#include "ObjAnywhere.hpp"
#include "ThreadBack.hpp"

namespace rlib
{
// ***********************************************************************************************
// wrap TaskBackFN to MsgSelf
inline
TaskBackFN viaMsgSelf(const TaskBackFN& aBackFN, EMsgPriority aPri = EMsgPri_NORM) noexcept
{
    auto&& msgSelf = MSG_SELF;
    return ! aBackFN || msgSelf.get() == nullptr
        ? TaskBackFN()  // empty fn
        : [aBackFN, msgSelf, aPri](SafePtr<void> aRet)  // must cp aBackFN since lambda run later in diff lifecycle
        {
            msgSelf->newMsg(bind(aBackFN, aRet), aPri);  // wrap aBackFN to queue in MsgSelf
        };
}

}  // namespace
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2023-09-15  CSZ       1)create
// 2024-10-05  CSZ       - simpler by MSG_SELF instead of para
// ***********************************************************************************************
