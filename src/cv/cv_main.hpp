/**
 * Copyright 2023 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - why:
//   . eg MtInQueue & ThreadBack use g_cvMainThread to wakeup main thread
// - core:
//   . g_cvMainThread
// ***********************************************************************************************
#pragma once

#include <condition_variable>

using namespace std;

namespace RLib
{
extern condition_variable g_cvMainThread;
}  // namespace
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2023-09-19  CSZ       1)create
// ***********************************************************************************************
