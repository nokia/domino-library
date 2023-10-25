/**
 * Copyright 2023 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - VALUE/why:
//   . viaMsgSelf() out of ThreadBack so ThreadBack can standalone without MsgSelf
//   . same for MsgSelf
// ***********************************************************************************************
#include "MsgSelf.hpp"
#include "ThreadBack.hpp"

namespace RLib
{
// ***********************************************************************************************
// wrap ThreadBackFN to queue in MsgSelf
inline
ThreadBackFN viaMsgSelf(const ThreadBackFN& aBackFn, shared_ptr<MsgSelf> aMsgSelf, EMsgPriority aPri = EMsgPri_NORM)
{
    return [aBackFn, aMsgSelf, aPri](bool aRet)  // must cp aBackFn since lambda run later in diff lifecycle
    {
cerr<<__LINE__<<' '<<__FILE__<<endl;
        aMsgSelf->newMsg(bind(aBackFn, aRet), aPri);  // wrap aBackFn to queue in MsgSelf
    };
}

}  // namespace
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2023-09-15  CSZ       1)create
// ***********************************************************************************************
