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
CellLog::CellLog(const CellName& aCellName) : cellName_(aCellName)
{
    auto&& it = logStore_.find(aCellName);
    if (it == logStore_.end())
    {
        smartLog_ = std::make_shared<SmartLog>();
        logStore_[aCellName] = smartLog_;
        DBG("creatd new log, name=" << aCellName);
    }
    else
    {
        smartLog_ = it->second;
        DBG("reused existing log, name=" << aCellName);
    }
}

// ***********************************************************************************************
CellLog& CellLog::defaultCellLog()
{
    if (not defaultCellLog_) defaultCellLog_ = std::make_shared<CellLog>(CELL_NAME_DEFAULT);
    return *defaultCellLog_;
}

// ***********************************************************************************************
size_t CellLog::logLen(const CellName& aCellName)
{
    auto&& it = logStore_.find(aCellName);
    return it == logStore_.end() ? 0 : it->second->str().size();
}

// ***********************************************************************************************
std::stringstream& CellLog::ssLog()
{
    *smartLog_ << '[' << cellName_ << '/';
    return *smartLog_;
}

// ***********************************************************************************************
LogStore CellLog::logStore_;
std::shared_ptr<CellLog> CellLog::defaultCellLog_;

}  // namespaces
