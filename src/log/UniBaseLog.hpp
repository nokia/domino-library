/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - Issue/why:
//   . all common of eg UniSmartLog, UniCoutLog
// ***********************************************************************************************
#ifndef UNI_BASE_HPP_
#define UNI_BASE_HPP_

#include <ctime>
#include <string>

using namespace std;

// ***********************************************************************************************
#define BUF(content) __func__ << "()#" << __LINE__ << ' ' << content << endl
#define DBG(content) { oneLog() << "DBG] " << BUF(content); }
#define INF(content) { oneLog() << "INF] " << BUF(content); }
#define WRN(content) { oneLog() << "WRN] " << BUF(content); }
#define ERR(content) { oneLog() << "ERR] " << BUF(content); }
#if WITH_HID_LOG  // more debug
#define HID(content) { oneLog() << "HID] " << BUF(content); }
#else
#define HID(content) {}
#endif

#define GTEST_LOG_FAIL { if (Test::HasFailure()) needLog(); }

namespace RLib
{
// ***********************************************************************************************
using UniLogName = string;
const char ULN_DEFAULT[] = "DEFAULT";

// ***********************************************************************************************
inline char* timestamp()
{
    static char buf[] = "ddd/HH:MM:SS.mmm";  // TODO: no simple way to impl ms
    auto now = time(nullptr);
    strftime(buf, sizeof(buf), "%j/%T", localtime(&now));
    return buf;
}
}  // namespace
#endif  // UNI_BASE_HPP_
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2022-08-29  CSZ       1)create
// ***********************************************************************************************
