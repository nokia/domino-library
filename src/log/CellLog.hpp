/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - why/value:
//   . provide smartlog & related functionality to any cell
// - req:
//   . init smartlog in CellLog()
//   . store in DatDom / ObjAnywhere so easily get without para shipping
//   . del smartlog in ~CellLog()
//   . easily support DBG/etc macros to log
//   . unify interface as CellLogRef
// ***********************************************************************************************
#ifndef CELLLOG_HPP_
#define CELLLOG_HPP_

#include <memory>
#include "StrCoutFSL.hpp"

namespace RLib
{
class CellLog
{
private:
    std::shared_ptr<StrCoutFSL> smart_log_;
};
}  // namespace
#endif  // CELLLOG_HPP_
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2022-08-16  CSZ       1)create
// ***********************************************************************************************
