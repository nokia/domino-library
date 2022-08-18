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
//   . easily support DBG/etc macros
//   . unify interface for DBG/etc macros
// ***********************************************************************************************
#ifndef CELLLOG_HPP_
#define CELLLOG_HPP_

#include <memory>
#include <unordered_map>

#include "StrCoutFSL.hpp"

namespace RLib
{
using CellName = std::string;
using SmartLog = StrCoutFSL;
using LogStore = std::unordered_map<CellName, std::shared_ptr<SmartLog> >;

class CellLog
{
public:
    explicit CellLog(const CellName& aCellName = "");
    ~CellLog() { if (isCell()) logStore_.erase(it_); }

    SmartLog& log() { return *(it_->second); }
    SmartLog& operator()() { return log(); }

    bool isCell() const { return isCell_; }
    static size_t nCellLog() { return logStore_.size(); }

private:
    LogStore::iterator it_;
    bool isCell_;

    static LogStore logStore_;
};

#if 1  // log_ instead of this->log_ so support static log_
#define SL_BUF(content) "] " << __func__ << "()" << __LINE__ << "#; " << content << std::endl
#define SL_DBG(content) { log() << "[DBG/" << SL_BUF(content); }
#define SL_INF(content) { log() << "[INF/" << SL_BUF(content); }
#define SL_WRN(content) { log() << "[WRN/" << SL_BUF(content); }
#define SL_ERR(content) { log() << "[ERR/" << SL_BUF(content); }
#define SL_HID(content) { log() << "[HID/" << SL_BUF(content); }
#else  // eg code coverage
#define SL_DBG(content) {}
#define SL_INF(content) {}
#define SL_WRN(content) {}
#define SL_ERR(content) {}
#define SL_HID(content) {}
#endif
}  // namespace
#endif  // CELLLOG_HPP_
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2022-08-16  CSZ       1)create
// ***********************************************************************************************
