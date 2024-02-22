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
// - MT safe: NO!!! since eg logStore_; so shall NOT cross-thread use
//   . SmartLog is to min log scope for better debug
//   . cross-thread is not SmartLog purpose, but user can add mutex if really need
// - mem safe: yes except
//   . keep ret of oneLog() - doesn't make sense, but then may use-after-free
//     . UniSmartLog can provide template<T/...Args> operator<<() but both can't deduce endl, etc
//     . so it's much simpler to ret SmartLog&/ref instead of UniSmartLog/copy
//   . uniLogName() is same
// ***********************************************************************************************
#pragma once

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

    SmartLog& oneLog() const;  // for logging; not mem-safe
    SmartLog& operator()() const { return oneLog(); }  // not mem-safe
    void needLog() { smartLog_->needLog(); }  // flag to dump
    const UniLogName& uniLogName() const { return uniLogName_; }  // not mem-safe

    static size_t nLog() { return logStore_.size(); }

private:
    // -------------------------------------------------------------------------------------------
    shared_ptr<SmartLog> smartLog_;
    const UniLogName     uniLogName_;

    static LogStore logStore_;
public:
    static UniSmartLog defaultUniLog_;


    // -------------------------------------------------------------------------------------------
#ifdef RLIB_UT
public:
    static void dumpAll_forUt()  // for ut case clean at the end; mem-risk=use-after-free, so ut ONLY
    {
        defaultUniLog_.oneLog().forceSave();  // dump
        logStore_.clear();  // simplest way to dump
    }

    static size_t logLen(const UniLogName& aUniLogName = ULN_DEFAULT)
    {
        auto&& it = logStore_.find(aUniLogName);
        return it == logStore_.end()
            ? 0
            : it->second->str().size();
    }
#endif
};

// ***********************************************************************************************
static SmartLog& oneLog() { return UniSmartLog::defaultUniLog_.oneLog(); }  // not mem-safe

}  // namespace
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2022-08-25  CSZ       1)mv major from UniLog.hpp
// 2022-12-02  CSZ       - simple & natural
// 2024-02-21  CSZ       2)mem-safe
// ***********************************************************************************************
