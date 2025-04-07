/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - ISSUE/why:
//   . encapsulate cout for eg:
//     . UT
//     . simplest log for debug
//     . gtest case destructor can't catch EXPECT_CALL() failure, so SmartLog doesn't work
//     . other unknown issue(s) that SmartLog can't fix
//
// - CORE:
//   . cout
//
// - MT safe : yes
// - class safe: yes
// ***********************************************************************************************
#pragma once

#include <memory>
#include <iostream>

#include "UniBaseLog.hpp"

namespace rlib
{
// ***********************************************************************************************
class UniCoutLog
{
public:
    explicit UniCoutLog(const LogName&) noexcept {}  // compatible UniSmartLog
    UniCoutLog() = default;

    static std::ostream& oneLog() noexcept;
    std::ostream& operator()() const noexcept { return oneLog(); }
    static void needLog() noexcept {}

    static LogName uniLogName() noexcept { return ULN_DEFAULT; }
    static size_t nLog() noexcept { return 1; }

    // -------------------------------------------------------------------------------------------
public:
    static UniCoutLog defaultUniLog_;
    static size_t     nLogLine_;  // ut only, simpler here

#ifdef IN_GTEST
    // -------------------------------------------------------------------------------------------
    // MT safe : no (since nLogLine_ is not atomic & no worth for ut only)
    // mem safe: yes
public:
    static void dumpAll_forUt() { nLogLine_ = 0; }  // for ut case clean at the end
    static size_t logLen(const LogName& = ULN_DEFAULT) { return nLogLine_; }
#endif
};

// ***********************************************************************************************
// static than inline, avoid ut conflict when coexist both UniLog
static std::ostream& oneLog() { return UniCoutLog::oneLog(); }

using UniLog = UniCoutLog;

}  // namespace
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2022-08-26  CSZ       1)create
// 2022-12-02  CSZ       - simple & natural
// 2024-02-21  CSZ       2)mem-safe
// 2025-04-07  CSZ       3)tolerate exception
// ***********************************************************************************************
