/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - why/value:
//   . a cell & its full-member & shared-member/participant shall log into 1 smartlog
// - req:
//   . for cell: create smartlog & store globally with cell name
//   . for member: find smartlog by cell name (so no para shipping)
//   . clean logStore_: del smartlog from logStore_ when cell destructed
//   * easily support DBG/etc macros
//   * unify interface for DBG/etc macros
//   . cell name as log prefix
//   . low couple:
//     . del cell, cell-member still can log
//     . del CellLog, copied one still can log (CellLog can't be assigned since const member)
//     . same for cell-participant
//   * no CellLog, all user code shall work well & as simple as legacy
//     . class based on CellLog: default using CellLog(CELL_NAME_DEFAULT)
//     . func with CellLog para: default using CellLog::defaultCellLog()
//     . class & func w/o CellLog: 
// ***********************************************************************************************
#ifndef CELLLOG_HPP_
#define CELLLOG_HPP_

#include <memory>
#include <unordered_map>

#include "StrCoutFSL.hpp"

namespace RLib
{
using SmartLog = StrCoutFSL;

// ***********************************************************************************************
#if 1  // log_ instead of this->log_ so support static log_
#define BUF(content) __func__ << "()" << __LINE__ << "# " << content << std::endl
#define DBG(content) { ssLog() << "DBG] " << BUF(content); }
#define INF(content) { ssLog() << "INF] " << BUF(content); }
#define WRN(content) { ssLog() << "WRN] " << BUF(content); }
#define ERR(content) { ssLog() << "ERR] " << BUF(content); }
#define HID(content) { ssLog() << "HID] " << BUF(content); }
#else  // eg code coverage
#define DBG(content) {}
#define INF(content) {}
#define WRN(content) {}
#define ERR(content) {}
#define HID(content) {}
#endif

#define GTEST_LOG_FAIL { if (Test::HasFailure()) needLog(); }

// ***********************************************************************************************
using CellName = std::string;
using LogStore = std::unordered_map<CellName, std::shared_ptr<SmartLog> >;

const char CELL_NAME_DEFAULT[] = "DEFAULT";

class CellLog
{
public:
    explicit CellLog(const CellName& aCellName = CELL_NAME_DEFAULT);
    ~CellLog() { if (smartLog_.use_count() == 2) logStore_.erase(cellName_); }

    std::stringstream& ssLog();
    std::stringstream& operator()() { return ssLog(); }
    const CellName& cellName() const { return cellName_; }
    void needLog() { smartLog_->needLog(); }

    static auto nCellLog() { return logStore_.size(); }
    static size_t logLen(const CellName& aCellName);
    static CellLog& defaultCellLog();

private:
    // -------------------------------------------------------------------------------------------
    std::shared_ptr<SmartLog> smartLog_;
    const CellName            cellName_;

    static LogStore logStore_;
public:
    static std::shared_ptr<CellLog> defaultCellLog_;
};

// ***********************************************************************************************
inline std::stringstream& ssLog() { return CellLog::defaultCellLog().ssLog(); }

}  // namespace
#endif  // CELLLOG_HPP_
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2020-08-11  CSZ       1)create as CppLog
// 2020-10-29  CSZ       2)base on SmartLog
// 2021-02-13  CSZ       - empty to inc branch coverage (66%->75%)
// 2022-08-16  CSZ       4)replaced by CellLog
// ***********************************************************************************************
