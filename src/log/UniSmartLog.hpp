/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - CONSTITUTION:
//   . implement UniLog based on SmartLog
//   * so can log only failed UT case(s) - no log for all succ case(s)
// - CORE:
//   . smartLog_
// - MT safe: false
// ***********************************************************************************************
#ifndef UNI_SMART_LOG_HPP_
#define UNI_SMART_LOG_HPP_

#include <memory>
#include <unordered_map>

#include "UniBaseLog.hpp"
#include "StrCoutFSL.hpp"

using namespace std;

namespace RLib
{
// ***********************************************************************************************
using SmartLog = StrCoutFSL;
using LogStore = unordered_map<UniLogName, shared_ptr<SmartLog> >;

// ***********************************************************************************************
class UniSmartLog
{
public:
    explicit UniSmartLog(const UniLogName& aUniLogName = ULN_DEFAULT);
    ~UniSmartLog() { if (smartLog_.use_count() == 2) logStore_.erase(uniLogName_); }

    SmartLog& oneLog();
    SmartLog& operator()() { return oneLog(); }
    const UniLogName& uniLogName() const { return uniLogName_; }

    static size_t logLen(const UniLogName& aUniLogName);
    static void needLog() { for (auto&& it : logStore_) it.second->needLog(); }
    static auto nLog() { return logStore_.size(); }
    static UniSmartLog& defaultUniLog();

private:
    // -------------------------------------------------------------------------------------------
    shared_ptr<SmartLog> smartLog_;
    const UniLogName     uniLogName_;

    static LogStore logStore_;
public:
    static shared_ptr<UniSmartLog> defaultUniLog_;
};

// ***********************************************************************************************
inline SmartLog& oneLog() { return UniSmartLog::defaultUniLog().oneLog(); }

// ***********************************************************************************************
using UniLog = UniSmartLog;

}  // namespace
#endif  // UNI_SMART_LOG_HPP_
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2020-08-25  CSZ       1)mv major from UniLog.hpp
// ***********************************************************************************************
