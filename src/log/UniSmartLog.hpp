/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - CONSTITUTION:
//   . implement UniLog based on SmartLog
//   * so can log only failed UT case(s) - no log for all succ case(s)
// - REQ:
//   * uni-interface (DBG/...) for all users (eg DomLib, swm, rfsw)
//     * easily support DBG/etc macros
//     * uni-interface for DBG/etc macros
//   . clean logStore_: del smartlog from logStore_ when no user
//   . readable: log name as log prefix
//   . low couple:
//     . 2 obj sharing 1 log, del 1 obj, another obj still can log
//     . del 1 UniLog, copied one still can log (UniLog can't be assigned since const member)
//     . callback func can independ logging/no crash
//   * support default / global UniLog as if legacy
//     . class based on UniLog: default using UniLog(ULN_DEFAULT)
//     . func with UniLog para: default using UniLog::defaultUniLog()
//     . class & func w/o UniLog: using global oneLog()
// - CORE:
//   . smartLog_
// - note:
//   . why oneLog() as func than var: more flexible, eg can print prefix in oneLog()
//   . why UniLog& to func:
//     . unify usr class & func: own its UniLog
//     . fast to pass reference from cell/member to participant
//     . can create new member within func
//   . why name as oneLog:
//     . vs ssLog: oneLog can represent SmartLog or UniLog
//     . vs log: too common, possible comflict with user definition
// ***********************************************************************************************
#ifndef UNI_SMART_LOG_HPP_
#define UNI_SMART_LOG_HPP_

#include <memory>
#include <unordered_map>

#include "StrCoutFSL.hpp"

namespace RLib
{
// ***********************************************************************************************
using SmartLog   = StrCoutFSL;
using LogStore   = std::unordered_map<UniLogName, std::shared_ptr<SmartLog> >;

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
    std::shared_ptr<SmartLog> smartLog_;
    const UniLogName          uniLogName_;

    static LogStore logStore_;
public:
    static std::shared_ptr<UniSmartLog> defaultUniLog_;
};

}  // namespace
#endif  // UNI_SMART_LOG_HPP_
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2020-08-25  CSZ       1)mv major from UniLog.hpp
// ***********************************************************************************************
