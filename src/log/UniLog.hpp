/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - CONSTITUTION:
//   * Cell-concept can use SmartLog
//   . a cell & its full-member & shared-member/participant shall log into 1 smartlog
//   . convenient all cells/members/participants to use SmartLog
// - REQ:
//   . for cell: create smartlog & store globally with cell name
//   . for member: find smartlog by cell name (so no para shipping)
//   . clean logStore_: del smartlog from logStore_ when no user
//   * easily support DBG/etc macros
//   * unify interface for DBG/etc macros
//   . readable: cell name as log prefix
//   . low couple:
//     . del cell, cell-member still can log
//     . del UniLog, copied one still can log (UniLog can't be assigned since const member)
//     . callback func can independ logging/no crash
//   * if no UniLog, all user code shall work well & as simple as legacy
//     . class based on UniLog: default using UniLog(ULN_DEFAULT)
//     . func with UniLog para: default using UniLog::defaultCellLog()
//     . class & func w/o UniLog: using global oneLog()
// - CORE:
//   . smartLog_
// - note:
//   . why oneLog() as func than var: more flexible, eg can print prefix in oneLog()
//   . why UniLog& to participant func:
//     . unify cell/member/participant: own its UniLog
//     . fast to pass reference from cell/member to participant
//     . can create new member within participant
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
using SmartLog = StrCoutFSL;

// ***********************************************************************************************
#if 1  // log_ instead of this->log_ so support static log_
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

#define GTEST_LOG_FAIL { if (Test::HasFailure()) UniLog::needLog(); }

// ***********************************************************************************************
using UniLogName = std::string;
using LogStore   = std::unordered_map<UniLogName, std::shared_ptr<SmartLog> >;

const char ULN_DEFAULT[] = "DEFAULT";

class UniLog
{
public:
    explicit UniLog(const UniLogName& aCellName = ULN_DEFAULT);
    ~UniLog() { if (smartLog_.use_count() == 2) logStore_.erase(cellName_); }

    SmartLog& oneLog();
    SmartLog& operator()() { return oneLog(); }
    const UniLogName& cellName() const { return cellName_; }

    static size_t logLen(const UniLogName& aCellName);
    static void needLog() { for (auto&& it : logStore_) it.second->needLog(); }
    static auto nLog() { return logStore_.size(); }
    static UniLog& defaultCellLog();

private:
    // -------------------------------------------------------------------------------------------
    std::shared_ptr<SmartLog> smartLog_;
    const UniLogName            cellName_;

    static LogStore logStore_;
public:
    static std::shared_ptr<UniLog> defaultCellLog_;
};

// ***********************************************************************************************
inline SmartLog& oneLog() { return UniLog::defaultCellLog().oneLog(); }

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
