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
#include <thread>

using LogName = std::string;

// ***********************************************************************************************
// - mem safe: yes
// - MT safe : yes of UniCoutLog, no of UniSmartLog
#ifdef __FILE_NAME__  // g++12/c++20
#define BUF(content) __FILE_NAME__ << ':' << __func__ << "():" << __LINE__ << ": " << content << std::endl
#else
#define BUF(content)                         __func__ << "():" << __LINE__ << ": " << content << std::endl
#endif
#define INF(content) { oneLog() << "INF/" << std::this_thread::get_id() << "] " << BUF(content); }
#define WRN(content) { oneLog() << "WRN/" << std::this_thread::get_id() << "] " << BUF(content); }
#define ERR(content) { oneLog() << "ERR/" << std::this_thread::get_id() << "] " << BUF(content); }

// - HID() is to be more debug but product code shall disable them
// - HID() uses cout since UniSmartLog is NOT MT safe
//   . under single thread, can change cout back to oneLog() for smart log
// - HID() is MT safe also upon UniSmartLog
#if WITH_HID_LOG
#define HID(content) { std::cout << "cout[" << rlib::mt_timestamp() << "/HID/" \
    << std::this_thread::get_id() << "] " << BUF(content) << std::dec; }
#else
#define HID(content) {}
#endif

#define GTEST_LOG_FAIL { if (Test::HasFailure()) needLog(); }  // UT only

namespace rlib
{
// ***********************************************************************************************
// - MT safe : yes
// - mem safe: yes
inline const char* mt_timestamp() noexcept
{
    static thread_local char buf[] = "ddd/HH:MM:SS.123456";  // ddd is days/year; thread_local is MT safe

    auto now_tp = std::chrono::system_clock::now();
    auto now_tt = std::chrono::system_clock::to_time_t(now_tp);
    struct tm now_tm;
    strftime(buf, sizeof(buf), "%j/%T.", localtime_r(&now_tt, &now_tm));  // cpplint asks localtime_r (MT safe)
    snprintf(buf + sizeof(buf) - 7, 7, "%06u",  // snprintf is safer than sprintf
        unsigned(std::chrono::duration_cast<std::chrono::microseconds>(now_tp.time_since_epoch()).count()
        % 1'000'000));
    return buf;
}

const char ULN_DEFAULT[] = "DEFAULT";

// ***********************************************************************************************
// - MT safe : yes (aStr is copied)
// - mem safe: yes
inline void cout_ascii(std::string aStr) noexcept
{
    try {  // cout may except(eg ios_base::failure) though rare; try-catch is safest as a log
        std::cout << std::hex;
        unsigned char preC = 0;
        for (unsigned char c : aStr)
        {
            if (std::isprint(c))
            {
                if (preC == 0xd || preC == 0xa)
                {
                    std::cout << std::endl;
                    preC = 0;
                }
                std::cout << c;
            }
            else
            {
                std::cout << "[`" << static_cast<size_t>(c) << "'}";  // [`'} is rare
                preC = c;
            }
        }
        std::cout << std::dec << std::endl;
    }
    catch(...) {}
}
}  // namespace
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2022-08-29  CSZ       1)create
// 2023-05-29  CSZ       - ms/us in mt_timestamp
// 2024-02-22  CSZ       2)mem-safe
// 2025-04-07  CSZ       3)tolerate exception; MT safe
// ***********************************************************************************************
