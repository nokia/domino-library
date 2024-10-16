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

#if 0
namespace rlib
{
// std::~, or ambiguous with boost::~
using   UniPtr =         std::shared_ptr<void>;
#define MAKE_PTR         std::make_shared
#define PTR              std::shared_ptr
#define DYN_PTR_CAST     std::dynamic_pointer_cast
#define STATIC_PTR_CAST  std::static_pointer_cast

#else
#include "SafePtr.hpp"
namespace rlib
{
using   UniPtr =         SafePtr<void>;
#define MAKE_PTR         make_safe
#define PTR              SafePtr
#define DYN_PTR_CAST     rlib::dynPtrCast
#define STATIC_PTR_CAST  rlib::staticPtrCast
// 4th req: SafePtr.get() return same as shared_ptr
#endif

}  // namespace
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2024-02-13  CSZ       1)create
// ***********************************************************************************************
