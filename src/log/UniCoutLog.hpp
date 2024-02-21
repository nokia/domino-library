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
// - MT safe: NO!!! since (static) nLogLine_; so shall NOT cross-thread use
// - mem safe: no
// ***********************************************************************************************
#ifndef UNI_COUT_LOG_HPP_
#define UNI_COUT_LOG_HPP_

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

    ostream& oneLog() const;
    ostream& operator()() const { return oneLog(); }
    void needLog() {}

    static const UniLogName uniLogName() { return ULN_DEFAULT; }
    static size_t nLog() { return 1; }
    static shared_ptr<UniCoutLog> defaultUniLog();  // mem-safe than ret UniCoutLog&

    // -------------------------------------------------------------------------------------------
private:
    static size_t nLogLine_;
public:
    static shared_ptr<UniCoutLog> defaultUniLog_;

    // -------------------------------------------------------------------------------------------
#ifdef RLIB_UT
public:
    static size_t logLen(const UniLogName& = ULN_DEFAULT) { return nLogLine_; }
    static void reset_thenMemRisk()  // for ut case clean at the end; mem-risk=use-after-free, so ut ONLY
    {
        defaultUniLog_.reset();
        nLogLine_ = 0;
    }
#endif
};

// ***********************************************************************************************
// static than inline, avoid ut conflict when coexist both UniLog
// ***********************************************************************************************
static ostream& oneLog() { return UniCoutLog::defaultUniLog()->oneLog(); }

}  // namespace
#endif  // UNI_COUT_LOG_HPP_
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2022-08-26  CSZ       1)create
// 2022-12-02  CSZ       - simple & natural
// ***********************************************************************************************
