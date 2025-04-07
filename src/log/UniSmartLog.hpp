/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - ISSUE/why:
//   . implement UniLog based on SmartLog
//   * so can log eg only failed UT case(s) - no log for all succ case(s)
//
// - CORE:
//   . smartLog_
//
// - MT safe: NO!!! since eg name_log_S_; so shall NOT cross-thread use
//   . SmartLog is to min log scope for better debug
//   . cross-thread is not SmartLog purpose, but user can add mutex if really need
// - class safe: yes except
//   . keep ret of oneLog() - doesn't make sense, but then may use-after-free
//     . UniSmartLog can provide template<T/...Args> operator<<() but both can't deduce endl, etc
//     . so it's much simpler to ret SmartLog&/ref instead of UniSmartLog/copy
// ***********************************************************************************************
#pragma once

#include <memory>
#include <unordered_map>

#include "StrCoutFSL.hpp"
#include "UniBaseLog.hpp"

namespace rlib
{
// ***********************************************************************************************
using SmartLog = StrCoutFSL;
using LogStore = std::unordered_map<LogName, std::shared_ptr<SmartLog> >;

// ***********************************************************************************************
class UniSmartLog
{
public:
    explicit UniSmartLog(const LogName& = ULN_DEFAULT);
    ~UniSmartLog() { if (smartLog_.use_count() == 2) name_log_S_.erase(uniLogName_); }

    SmartLog& oneLog() const;  // for logging; ret ref is not mem-safe when use the ref after del
    SmartLog& operator()() const { return oneLog(); }  // not mem-safe as oneLog()
    void needLog() { smartLog_->needLog(); }  // flag to dump
    LogName uniLogName() const { return uniLogName_; }

    static size_t nLog() { return name_log_S_.size(); }

private:
    // -------------------------------------------------------------------------------------------
    std::shared_ptr<SmartLog>  smartLog_;
    const LogName  uniLogName_;

    static LogStore name_log_S_;
public:
    static UniSmartLog defaultUniLog_;


#ifdef IN_GTEST
    // -------------------------------------------------------------------------------------------
    // MT safe : no
    // mem safe: yes
public:
    static void dumpAll_forUt()  // for ut case clean at the end; mem-risk=use-after-free, so ut ONLY
    {
        defaultUniLog_.oneLog().forceSave();  // dump
        name_log_S_.clear();  // simplest way to dump
    }

    static size_t logLen(const LogName& aUniLogName = ULN_DEFAULT)
    {
        auto&& name_log = name_log_S_.find(aUniLogName);
        return name_log == name_log_S_.end()
            ? 0
            : name_log->second->str().size();
    }
#endif
};

// ***********************************************************************************************
// static than inline, avoid ut conflict when coexist both UniLog
static SmartLog& oneLog() { return UniSmartLog::defaultUniLog_.oneLog(); }  // ret ref is not mem-safe

using UniLog = UniSmartLog;

}  // namespace
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2022-08-25  CSZ       1)mv major from UniLog.hpp
// 2022-12-02  CSZ       - simple & natural
// 2024-02-21  CSZ       2)mem-safe
// 2025-04-07  CSZ       3)tolerate exception
// ***********************************************************************************************
