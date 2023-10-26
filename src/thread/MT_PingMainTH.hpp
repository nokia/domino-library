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
extern MT_Semaphore g_sem;  // can't use ObjAnywhere that is not MT safe
// ***********************************************************************************************
inline void mt_pingMainTH()
{
    g_sem.mt_notify();
}

}  // namespace
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2023-10-25  CSZ       1)create
// ***********************************************************************************************
