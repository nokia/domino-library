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

#include <chrono>
#include <cstdio>
#include <ctime>
#include <string>

using namespace std;
using namespace std::chrono;

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
    static char buf[] = "ddd/HH:MM:SS.mmm";  // ddd is days/year

    auto now_tp = system_clock::now();
    auto now_tt = system_clock::to_time_t(now_tp);
    strftime(buf, sizeof(buf), "%j/%T.", localtime(&now_tt));
    sprintf(buf + sizeof(buf) - 4, "%03u",
        unsigned(duration_cast<milliseconds>(now_tp.time_since_epoch()).count() % 1000));
    return buf;
}
}  // namespace
#endif  // UNI_BASE_HPP_
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2022-08-29  CSZ       1)create
// 2023-05-29  CSZ       - ms in timestamp
// ***********************************************************************************************
