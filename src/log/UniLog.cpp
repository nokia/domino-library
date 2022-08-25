/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include "UniLog.hpp"

namespace RLib
{
// ***********************************************************************************************
UniLog::UniLog(const UniLogName& aUniLogName) : uniLogName_(aUniLogName)
{
    auto&& it = logStore_.find(aUniLogName);
    if (it == logStore_.end())
    {
        smartLog_ = std::make_shared<SmartLog>();
        logStore_[aUniLogName] = smartLog_;
        DBG("creatd new log, name=" << aUniLogName);
    }
    else
    {
        smartLog_ = it->second;
        DBG("reused existing log, name=" << aUniLogName);
    }
}

// ***********************************************************************************************
UniLog& UniLog::defaultCellLog()
{
    if (not defaultCellLog_) defaultCellLog_ = std::make_shared<UniLog>(ULN_DEFAULT);
    return *defaultCellLog_;
}

// ***********************************************************************************************
size_t UniLog::logLen(const UniLogName& aUniLogName)
{
    auto&& it = logStore_.find(aUniLogName);
    return it == logStore_.end() ? 0 : it->second->str().size();
}

// ***********************************************************************************************
SmartLog& UniLog::oneLog()
{
    *smartLog_ << '[' << uniLogName_ << '/';
    return *smartLog_;
}

// ***********************************************************************************************
LogStore UniLog::logStore_;
std::shared_ptr<UniLog> UniLog::defaultCellLog_;

}  // namespaces
