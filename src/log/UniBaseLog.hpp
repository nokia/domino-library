/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - CONSTITUTION:
//   . all common of eg UniSmartLog, UniCoutLog
// ***********************************************************************************************
#ifndef UNI_BASE_HPP_
#define UNI_BASE_HPP_

#include <string>

using namespace std;

// ***********************************************************************************************
#if 1  // normal
#define BUF(content) __func__ << "()" << __LINE__ << "# " << content << endl
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

// ***********************************************************************************************
#define GTEST_LOG_FAIL { if (Test::HasFailure()) UniLog::needLog(); }

// ***********************************************************************************************
namespace RLib
{
using UniLogName = string;
const char ULN_DEFAULT[] = "DEFAULT";
}  // namespace

#endif  // UNI_BASE_HPP_
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2020-08-29  CSZ       1)create
// ***********************************************************************************************
