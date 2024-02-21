/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - ISSUE/why:
//   . implement UniLog based on SmartLog
//   * so can log only failed UT case(s) - no log for all succ case(s)
//
// - CORE:
//   . smartLog_
//
// - MT safe: NO!!! since (static) logStore_; so shall NOT cross-thread use
// - mem safe: no
// ***********************************************************************************************
#ifndef UNI_SMART_LOG_HPP_
#define UNI_SMART_LOG_HPP_

#include <memory>
#include <unordered_map>

#include "StrCoutFSL.hpp"
#include "UniBaseLog.hpp"

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

    SmartLog& oneLog() const;  // for logging; const UniSmartLog can log; not mem-safe if keep ret
    SmartLog& operator()() const { return oneLog(); }  // not mem-safe if keep ret
    void needLog() { smartLog_->needLog(); }  // flag to dump
    const UniLogName& uniLogName() const { return uniLogName_; }

    static shared_ptr<UniSmartLog> defaultUniLog();  // usage as if global cout; mem-safe than ret UniSmartLog&

private:
    // -------------------------------------------------------------------------------------------
    shared_ptr<SmartLog> smartLog_;
    const UniLogName     uniLogName_;

    static LogStore logStore_;
public:
    static shared_ptr<UniSmartLog> defaultUniLog_;


    // -------------------------------------------------------------------------------------------
    // for ut
public:
    static size_t logLen(const UniLogName& = ULN_DEFAULT);
    static size_t nLog() { return logStore_.size(); }
    static void reset();  // for ut case clean at the end
};

// ***********************************************************************************************
static SmartLog& oneLog() { return UniSmartLog::defaultUniLog()->oneLog(); }

}  // namespace
#endif  // UNI_SMART_LOG_HPP_
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2022-08-25  CSZ       1)mv major from UniLog.hpp
// 2022-12-02  CSZ       - simple & natural
// ***********************************************************************************************
