/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - ISSUE/why:
//   . implement UniLog based on cout
//     . so failed case log can perfectly integrate with gtest output - easy debug
//     (SmartLog always gtest outputs then case log)
//     . case destructor can't catch EXPECT_CALL() failure, so SmartLog doesn't work
//     . other unknown issue(s) that SmartLog can't fix
//
// - CORE:
//   . cout
//
// - MT safe : yes (no for ut)
// - mem safe: yes
// ***********************************************************************************************
#pragma once

#include <memory>
#include <iostream>

#include "UniBaseLog.hpp"

using namespace std;

namespace RLib
{
// ***********************************************************************************************
class UniCoutLog
{
public:
    explicit UniCoutLog(const UniLogName& = ULN_DEFAULT) { HID("ignore LogName but default, this=" << this) }

    static ostream& oneLog();
    ostream& operator()() const { return oneLog(); }
    static void needLog() {}

    static const UniLogName uniLogName() { return ULN_DEFAULT; }
    static size_t nLog() { return 1; }

    // -------------------------------------------------------------------------------------------
public:
    static UniCoutLog defaultUniLog_;
    static size_t     nLogLine_;  // ut only, simpler here

#ifdef RLIB_UT
    // -------------------------------------------------------------------------------------------
    // MT safe : no
    // mem safe: yes
public:
    static size_t logLen(const UniLogName& = ULN_DEFAULT) { return nLogLine_; }
    static void dumpAll_forUt() { nLogLine_ = 0; }  // for ut case clean at the end
#endif
};

// ***********************************************************************************************
// static than inline, avoid ut conflict when coexist both UniLog
static ostream& oneLog() { return UniCoutLog::oneLog(); }

}  // namespace
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2022-08-26  CSZ       1)create
// 2022-12-02  CSZ       - simple & natural
// 2024-02-21  CSZ       2)mem-safe
// ***********************************************************************************************
