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
UniSmartLog::UniSmartLog(const LogName& aUniLogName) : uniLogName_(aUniLogName)
{
    //for(auto&& it : logStore_) cout << it.first<<", p=" << it.second.get() << endl;
    auto&& it = logStore_.find(aUniLogName);
    if (it == logStore_.end())
    {
        smartLog_ = make_shared<SmartLog>();
        logStore_[aUniLogName] = smartLog_;
        HID("creatd new log=" << (void*)(smartLog_.get()) << ", name=" << aUniLogName << ", nLog=" << nLog());
    }
    else
    {
        smartLog_ = it->second;
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
LogStore    UniSmartLog::logStore_;
UniSmartLog UniSmartLog::defaultUniLog_;

}  // namespaces
