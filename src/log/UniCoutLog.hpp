/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - CONSTITUTION:
//   . implement UniLog based on cout
//   * so failed case log can perfectly integrate with gtest output - easy debug
//     (SmartLog always gtest outputs then case log)
// - CORE:
//   . cout
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
    explicit UniCoutLog(const UniLogName& aUniLogName = ULN_DEFAULT) : uniLogName_(aUniLogName) {}
    ostream& oneLog();
    ostream& operator()() { return oneLog(); }
    void needLog() {}
    const UniLogName& uniLogName() const { return uniLogName_; }

    static size_t logLen(const UniLogName&) { return nLogLine_; }
    static auto nLog() { return 1; }
    static UniCoutLog& defaultUniLog();

private:
    // -------------------------------------------------------------------------------------------
    const UniLogName uniLogName_;
    static size_t nLogLine_;
public:
    static shared_ptr<UniCoutLog> defaultUniLog_;
};

// ***********************************************************************************************
inline ostream& oneLog() { return UniCoutLog::defaultUniLog().oneLog(); }

}  // namespace
#endif  // UNI_COUT_LOG_HPP_
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2020-08-26  CSZ       1)create
// ***********************************************************************************************
