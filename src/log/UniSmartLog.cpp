/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include "UniSmartLog.hpp"

namespace RLib
{
// ***********************************************************************************************
UniSmartLog::UniSmartLog(const UniLogName& aUniLogName) : uniLogName_(aUniLogName)
{
    //for(auto&& it : logStore_) cout << it.first<<", p=" << it.second.get() << endl;
    auto&& it = logStore_.find(aUniLogName);
    if (it == logStore_.end())
    {
        smartLog_ = make_shared<SmartLog>();
        logStore_[aUniLogName] = smartLog_;
        HID("creatd new log=" << smartLog_.get() << aUniLogName << ", nLog=" << nLog());
    }
    else
    {
        smartLog_ = it->second;
        HID("reused existing log=" << smartLog_.get() << aUniLogName << ", nLog=" << nLog());
    }
}

// ***********************************************************************************************
UniSmartLog& UniSmartLog::defaultUniLog()
{
    if (not defaultUniLog_) defaultUniLog_ = make_shared<UniSmartLog>(ULN_DEFAULT);
    return *defaultUniLog_;
}

// ***********************************************************************************************
size_t UniSmartLog::logLen(const UniLogName& aUniLogName)
{
    auto&& it = logStore_.find(aUniLogName);
    return it == logStore_.end() ? 0 : it->second->str().size();
}

// ***********************************************************************************************
SmartLog& UniSmartLog::oneLog()
{
    *smartLog_ << "s[" << timestamp() << ' ' << uniLogName_ << '/';
    return *smartLog_;
}

// ***********************************************************************************************
void UniSmartLog::reset()
{
    defaultUniLog_.reset();
    logStore_.clear();
}

// ***********************************************************************************************
LogStore UniSmartLog::logStore_;
shared_ptr<UniSmartLog> UniSmartLog::defaultUniLog_;

}  // namespaces
