/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include "UniSmartLog.hpp"

using namespace std;

namespace rlib
{
// ***********************************************************************************************
UniSmartLog::UniSmartLog(const LogName& aUniLogName) : uniLogName_(aUniLogName)
{
    // for(auto&& name_log : name_log_S_) cout << name_log.first<<", p=" << name_log.second.get() << endl;
    auto&& name_log = name_log_S_.find(aUniLogName);
    if (name_log == name_log_S_.end())
    {
        smartLog_ = make_shared<SmartLog>();
        name_log_S_[aUniLogName] = smartLog_;
        HID("creatd new log=" << (void*)(smartLog_.get()) << ", name=" << aUniLogName << ", nLog=" << nLog());
    }
    else
    {
        smartLog_ = name_log->second;
        HID("reused existing log=" << (void*)(smartLog_.get()) << ", name=" << aUniLogName << ", nLog=" << nLog());
    }
}

// ***********************************************************************************************
SmartLog& UniSmartLog::oneLog() const
{
    *smartLog_ << "s[" << timestamp() << ' ' << uniLogName_ << '/';
    return *smartLog_;
}

// ***********************************************************************************************
LogStore    UniSmartLog::name_log_S_;
UniSmartLog UniSmartLog::defaultUniLog_;

}  // namespaces
