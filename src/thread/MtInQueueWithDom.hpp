/**
 * Copyright 2024 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - why: auto move an ele in MtQ to DataDom
// ***********************************************************************************************
#pragma once

#include "MtInQueue.hpp"

namespace rlib
{
const char EN_MTQ[] = "/MtQ/";

// ***********************************************************************************************
inline
void defaultHdlr(ELE_TID& aEle_tid)
{
    auto&& dom = DAT_DOM;
    const auto en = std::string(EN_MTQ) + aEle_tid.second.name();
    dom->replaceData(en, aEle_tid.first);
    dom->forceAllHdlr(en);
}

}  // namespace
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2024-10-05  CSZ       1)create
// ***********************************************************************************************
