/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - Issue/why:
//   . all common of eg UniSmartLog, UniCoutLog
//
// - MT safe: no
// - mem safe: yes
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
#define BUF(content) __func__ << "():" << __LINE__ << ": " << content << endl  // __FILE_NAME__ since GCC 12
#define DBG(content) { oneLog() << "DBG] " << BUF(content); }
#define INF(content) { oneLog() << "INF] " << BUF(content); }
#define WRN(content) { oneLog() << "WRN] " << BUF(content); }
#define ERR(content) { oneLog() << "ERR] " << BUF(content); }
#if WITH_HID_LOG  // more debug but NOT exist in product code
#define HID(content) { cout << "[HID] " << BUF(content); }  // simple cout & supports multi-thread
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
// - MT safe : yes
// - mem safe: yes
inline char* timestamp()
{
    static thread_local char buf[] = "ddd/HH:MM:SS.123456";  // ddd is days/year; thread_local is MT safe

    auto now_tp = system_clock::now();
    auto now_tt = system_clock::to_time_t(now_tp);
    strftime(buf, sizeof(buf), "%j/%T.", localtime(&now_tt));
    snprintf(buf + sizeof(buf) - 7, 7, "%06u",  // snprintf is safer than sprintf
        unsigned(duration_cast<microseconds>(now_tp.time_since_epoch()).count() % 1'000'000));  // can milliseconds
    return buf;
}
}  // namespace
#endif  // UNI_BASE_HPP_
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2022-08-29  CSZ       1)create
// 2023-05-29  CSZ       - ms/us in timestamp
// ***********************************************************************************************
