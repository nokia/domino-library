/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include "CellLog.hpp"

namespace RLib
{
// ***********************************************************************************************
CellLog::CellLog(const CellName& aCellName)
    : it_(logStore_.find(aCellName))
    , isCell_(it_ == logStore_.end())
{
    if (isCell_)
    {
        logStore_[aCellName] = std::make_shared<SmartLog>();
        it_ = logStore_.find(aCellName);
        DBG("created new log=" << aCellName);
    }
}

// ***********************************************************************************************
std::stringstream& CellLog::ssLog()
{
    auto&& smartLog = *(it_->second);
    smartLog << '[' << it_->first << '/';
    return smartLog;
}

// ***********************************************************************************************
CellLog& CellLog::defaultCellLog()
{
    static CellLog staticLog(CELL_NAME_DEFAULT);
    return staticLog;
}

// ***********************************************************************************************
LogStore CellLog::logStore_;

}  // namespaces
