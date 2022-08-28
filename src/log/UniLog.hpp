/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - CONSTITUTION:
//   * log is always needed
//   * impact no usr code when switch log among eg cout, LoggingSystem, SmartLog/cell-log
// - VALUE:
//   . unified DBG/etc interface to all usrs
//   . min oneLog() interface to DBG/etc for all scenarios: inner class & func, default global log
//   . UniLogName to avoid para travel
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
