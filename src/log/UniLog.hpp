/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - CONSTITUTION:
//   * impact no usr code when switch log eg cout, LoggingSystem, SmartLog/cell-log
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
//   . real log eg cout, smartLog_
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
#ifndef UNI_LOG_HPP_
#define UNI_LOG_HPP_

#include <memory>
#include <unordered_map>

#include "StrCoutFSL.hpp"

namespace RLib
{
// ***********************************************************************************************
#if 1
#define BUF(content) __func__ << "()" << __LINE__ << "# " << content << std::endl
#define DBG(content) { oneLog() << "DBG] " << BUF(content); }
#define INF(content) { oneLog() << "INF] " << BUF(content); }
#define WRN(content) { oneLog() << "WRN] " << BUF(content); }
#define ERR(content) { oneLog() << "ERR] " << BUF(content); }
#define HID(content) { oneLog() << "HID] " << BUF(content); }
#else  // eg code coverage
#define DBG(content) {}
#define INF(content) {}
#define WRN(content) {}
#define ERR(content) {}
#define HID(content) {}
#endif

using UniLogName = std::string;

#define GTEST_LOG_FAIL { if (Test::HasFailure()) UniLog::needLog(); }

// ***********************************************************************************************
#if 1  // base on SmartLog
using SmartLog   = StrCoutFSL;
using LogStore   = std::unordered_map<UniLogName, std::shared_ptr<SmartLog> >;

const char ULN_DEFAULT[] = "DEFAULT";

class UniLog
{
public:
    explicit UniLog(const UniLogName& aUniLogName = ULN_DEFAULT);
    ~UniLog() { if (smartLog_.use_count() == 2) logStore_.erase(uniLogName_); }

    SmartLog& oneLog();
    SmartLog& operator()() { return oneLog(); }
    const UniLogName& uniLogName() const { return uniLogName_; }

    static size_t logLen(const UniLogName& aUniLogName);
    static void needLog() { for (auto&& it : logStore_) it.second->needLog(); }
    static auto nLog() { return logStore_.size(); }
    static UniLog& defaultUniLog();

private:
    // -------------------------------------------------------------------------------------------
    std::shared_ptr<SmartLog> smartLog_;
    const UniLogName            uniLogName_;

    static LogStore logStore_;
public:
    static std::shared_ptr<UniLog> defaultCellLog_;
};

inline SmartLog& oneLog() { return UniLog::defaultUniLog().oneLog(); }

// ***********************************************************************************************
#else  // base on cout
#endif

}  // namespace
#endif  // UNI_LOG_HPP_
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2020-08-11  CSZ       1)create as CppLog
// 2020-10-29  CSZ       2)base on SmartLog
// 2021-02-13  CSZ       - empty to inc branch coverage (66%->75%)
// 2022-08-16  CSZ       4)replaced by UniLog
// ***********************************************************************************************
