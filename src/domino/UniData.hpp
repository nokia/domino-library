/**
 * Copyright 2024 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - why:
//   . example to support SafePtr & shared_ptr as UniData
//   . users can modify this file for their req
// - why support shared_ptr?
//   . from mem-safe pov, SafePtr is the only choice
//   . shared_ptr may be same safe as SafePtr, then SafePtr is unnecessary
// ***********************************************************************************************
#pragma once

#include <memory>  // make_shared

using namespace std;

namespace RLib
{

#if 1
using UniData = shared_ptr<void>;
#define MAKE_UNI_DATA  make_shared
#else
using UniData = SafePtr<void>;
#define MAKE_UNI_DATA  make_safe
#endif

}  // namespace
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2024-02-13  CSZ       1)create
// ***********************************************************************************************
