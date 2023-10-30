/**
 * Copyright 2023 Nokia. All rights reserved.
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - Why/VALUE:
//   . help eg MsgSelf, ThreadBack, MtInQueue to wakeup main thread by mt_pingMainTH()
//     . simpler than para to constructing above class(es)
//   . can base on MT_Semaphore
//     . can be empty so no wakeup at all
//     . this file is an example, users can define their own MT_PingMainTH.hpp
// ***********************************************************************************************
#pragma once

#include "MT_Semaphore.hpp"

namespace RLib
{
// ***********************************************************************************************
// - can't use ObjAnywhere that is not MT safe
// - REQ: usr shall not use g_sem, otherwise impl change may impact his code
extern MT_Semaphore g_sem;

// ***********************************************************************************************
// - REQ: can provide diff impl w/o usr code change
inline void mt_pingMainTH()
{
    g_sem.mt_notify();
}

// ***********************************************************************************************
// - REQ: can provide diff impl w/o usr code change
// - no mt_ since only main thread shall call it
inline void timedwait(const size_t aSec = 0, const size_t aRestNsec = 100'000'000)
{
    g_sem.mt_timedwait(aSec, aRestNsec);
}

}  // namespace
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2023-10-25  CSZ       1)create
// ***********************************************************************************************
