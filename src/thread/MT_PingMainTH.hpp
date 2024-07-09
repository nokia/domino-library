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
//     . this file is an example, users can define their own MT_PingMainTH.hpp - eg condition_variable
// - MT safe: yes
// - mem safe: yes
// ***********************************************************************************************
#pragma once

#include "MT_Semaphore.hpp"

namespace RLib
{
// ***********************************************************************************************
// - can't use ObjAnywhere that is not MT safe
// - REQ: usr shall not use g_semToMainTH, otherwise impl change may impact his/her code
extern MT_Semaphore g_semToMainTH;

// ***********************************************************************************************
// - REQ: can provide diff impl w/o usr code change
// - sem is better since can wakeup later wait(), while condition_variable can NOT
inline void mt_pingMainTH()
{
    g_semToMainTH.mt_notify();
}

// ***********************************************************************************************
// - REQ: can provide diff impl w/o usr code change
// - no mt_ since only main thread shall call it
// - REQ: timer guarantee no wait forever
inline void timedwait(const size_t aSec = 0, const size_t aRestNsec = 100'000'000)
{
    g_semToMainTH.timedwait(aSec, aRestNsec);
}

}  // namespace
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2023-10-25  CSZ       1)create
// ***********************************************************************************************
