/**
 * Copyright 2024 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - why:
//   . example to support SafePtr & shared_ptr as UniPtr
//   . users can modify this file for their req
// - why support shared_ptr?
//   . from mem-safe pov, SafePtr is the only choice
//   . shared_ptr may be same safe as SafePtr, then SafePtr is unnecessary
// ***********************************************************************************************
#pragma once

#include <memory>  // make_shared

using namespace std;

#if 0
namespace RLib
{
// std::~, or ambiguous with boost::~
using   UniPtr =  shared_ptr<void>;
#define MAKE_PTR  make_shared
#define PTR       shared_ptr

#else
#include "SafePtr.hpp"
namespace RLib
{
using   UniPtr =  SafePtr<void>;
#define MAKE_PTR  make_safe
#define PTR       SafePtr
// 4th req: SafePtr.get() return same as shared_ptr
#endif

}  // namespace
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2024-02-13  CSZ       1)create
// ***********************************************************************************************
