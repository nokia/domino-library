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

#include <string>

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

#define GTEST_LOG_FAIL { if (Test::HasFailure()) UniLog::needLog(); }

namespace RLib
{
using UniLogName = std::string;
const char ULN_DEFAULT[] = "DEFAULT";
}

// ***********************************************************************************************
#if 1  // base on smartlog
#include "UniSmartLog.hpp"
namespace RLib
{
using UniLog = UniSmartLog;
inline SmartLog& oneLog() { return UniLog::defaultUniLog().oneLog(); }
}
// ***********************************************************************************************
#else  // base on cout (LoggingSystem is similar)
#include "UniCoutLog.hpp"
namespace RLib
{
using UniLog = UniCoutLog;
inline std::ostream& oneLog() { return UniLog::defaultUniLog().oneLog(); }
}
#endif

#endif  // UNI_LOG_HPP_
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2020-08-11  CSZ       1)create as CppLog
// 2020-10-29  CSZ       2)base on SmartLog
// 2021-02-13  CSZ       - empty to inc branch coverage (66%->75%)
// 2022-08-16  CSZ       4)replaced by UniLog
// 2022-08-25  CSZ       - support both SmartLog & cout
// ***********************************************************************************************
