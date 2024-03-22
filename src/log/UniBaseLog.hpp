/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - Issue/why:
//   . all common of eg UniSmartLog, UniCoutLog
// ***********************************************************************************************
#pragma once

#include <chrono>
#include <cstdio>
#include <ctime>
#include <memory>
#include <string>

using namespace std;
using namespace std::chrono;

using LogName = string;

// ***********************************************************************************************
// - mem safe: yes
// - MT safe : yes upon UniCoutLog, no upon UniSmartLog
#define BUF(content) __func__ << "():" << __LINE__ << ": " << content << endl  // __FILE_NAME__ since GCC 12
#define INF(content) { oneLog() << "INF] " << BUF(content); }
#define WRN(content) { oneLog() << "WRN] " << BUF(content); }
#define ERR(content) { oneLog() << "ERR] " << BUF(content); }

// - HID() is to be more debug but product code shall disable them
// - HID() uses cout since UniSmartLog is NOT MT safe
//   . under single thread, can change cout back to oneLog() for smart log
// - HID() is MT safe also upon UniSmartLog
#if WITH_HID_LOG
#define HID(content) { cout << "cout[" << timestamp() << "/HID] " << BUF(content) << dec; }
#else
#define HID(content) {}
#endif

#define GTEST_LOG_FAIL { if (Test::HasFailure()) needLog(); }  // UT only

namespace RLib
{
// ***********************************************************************************************
// - MT safe : yes
// - mem safe: yes
inline const char* timestamp()
{
    static thread_local char buf[] = "ddd/HH:MM:SS.123456";  // ddd is days/year; thread_local is MT safe

    auto now_tp = system_clock::now();
    auto now_tt = system_clock::to_time_t(now_tp);
    strftime(buf, sizeof(buf), "%j/%T.", localtime(&now_tt));
    snprintf(buf + sizeof(buf) - 7, 7, "%06u",  // snprintf is safer than sprintf
        unsigned(duration_cast<microseconds>(now_tp.time_since_epoch()).count() % 1'000'000));  // can milliseconds
    return buf;
}

const char ULN_DEFAULT[] = "DEFAULT";

}  // namespace
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2022-08-29  CSZ       1)create
// 2023-05-29  CSZ       - ms/us in timestamp
// 2024-02-22  CSZ       2)mem-safe
// ***********************************************************************************************
