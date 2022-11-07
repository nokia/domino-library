/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - ISSUE/what/req:
//   .
// - Usage:
//   .
// - VALUE/why:
//   .
// - core:
// ***********************************************************************************************
#pragma once

using namespace std;
using FnInMainThread = function<void()>;

namespace RLib
{
// ***********************************************************************************************
class MainRouser
{
public:
    virtual void toMainThread(const FnInMainThread&) = 0;
};

}  // namespace
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2022-10-27  CSZ       1)create
// ***********************************************************************************************
