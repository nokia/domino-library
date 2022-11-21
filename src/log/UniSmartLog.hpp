/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - CONSTITUTION:
//   . implement UniLog based on SmartLog
//   * so can log only failed UT case(s) - no log for all succ case(s)
//
// - CORE:
//   . smartLog_
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
    explicit UniSmartLog(const UniLogName& = ULN_DEFAULT);
    ~UniSmartLog() { if (smartLog_.use_count() == 2) logStore_.erase(uniLogName_); }

    SmartLog& oneLog();  // for logging
    SmartLog& operator()() { return oneLog(); }
    void needLog() { smartLog_->needLog(); }  // flag to dump
    const UniLogName& uniLogName() const { return uniLogName_; }

    static UniSmartLog& defaultUniLog();  // usage like legacy
    // for ut
    static size_t logLen(const UniLogName& = ULN_DEFAULT);
    static size_t nLog() { return logStore_.size(); }
    static void reset();

private:
    // -------------------------------------------------------------------------------------------
    shared_ptr<SmartLog> smartLog_;
    const UniLogName     uniLogName_;

    static LogStore logStore_;
public:
    static shared_ptr<UniSmartLog> defaultUniLog_;
};

// ***********************************************************************************************
static SmartLog& oneLog() { return UniSmartLog::defaultUniLog().oneLog(); }

}  // namespace
#endif  // UNI_SMART_LOG_HPP_
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2020-08-25  CSZ       1)mv major from UniLog.hpp
// ***********************************************************************************************
