/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - ISSUE:
//   . log is always needed
//   * no usr code change when switch log among cout, LoggingSystem, SmartLog/cell-log, etc
//   * simple & natural
//
// - Usage:
//   * class A : public UniLog         // typical/inherit
//   . class B { UniLog oneLog; ... }  // alt/member, member name must be "oneLog"
//   . func_c(..., UniLog& oneLog)     // alt/ref, ref name must be "oneLog"
//   . func_d(..., UniLog  conLog)     // alt/copy, copy name must be "oneLog"
//
// - VALUE:
//   * unified logging interface: DBG/INF/WRN/ERR/HID
//   . min oneLog() to DBG/etc in all scenarios: inner class & func, default global log
//   . UniLogName to avoid para/UniLog travel (from creator, to mid ..., to end-usr)
//
// - File include:
//                          UniBaseLog.hpp
//                         /              \
//          UniSmartLog.hpp                UniCoutLog.hpp
//         /               \              /              \
//   UniSmartLog.cpp        \            /          UniCoutLog.cpp
//                           \          /
//                            UniLog.hpp
//                                :
//                                :
//                              users
//
// - UT include:
//                          UniBaseLog.hpp
//                         /              \
//          UniSmartLog.hpp                UniCoutLog.hpp
//         /           :                      :          \
//   UniSmartLog.cpp   :                      :     UniCoutLog.cpp
//                     :                      :
//                     :    UniLogTest.hpp    :
//                     :    :            :    :
//                     :    :            :    :
//         UniSmartLogTest.cpp          UniCoutLogTest.cpp
 //
// - REQ:
//   * uni-interface (DBG/...) for all users (eg DomLib, swm, rfsw)
//     * easily support DBG/etc macros
//     * uni-interface for DBG/etc macros
//   . clean logStore_: del smartlog from logStore_ when no user
//   . readable: log name as log prefix
//   . low couple:
//     . 2 obj sharing 1 log, del 1 obj, another obj still can log
//     . del 1 UniLog, copied one still can log (UniLog can't be assigned since const member)
//     . callback func can independ logging/no crash
//   * support default / global UniLog as if legacy
//     . class based on UniLog: default using UniLog(ULN_DEFAULT)
//     . func with UniLog para: default using UniLog::defaultUniLog()
//     . class & func w/o UniLog: using global oneLog()
//
// - MT safe: FALSE
//   * so can only use in main thread
//   . though cout is MT safe, UniCoutLog is NOT
//
// - Q&A:
//   . why oneLog() as func than var: more flexible, eg can print prefix in oneLog()
//   . why UniLog& to func:
//     . unify usr class & func: own its UniLog
//     . fast to pass reference from cell/member to participant
//     . can create new member within func
//   . why name as oneLog:
//     . vs ssLog: oneLog can represent SmartLog or UniLog
//     . vs log: too common, possible comflict with user definition
//   . why not support multi-thread:
//     . mainthread/logic is the most usage
//     . not worth to pay(lock-mechanism) for low-possible usage
//   * why DomTest's log not in smart log?
//     . since they're template test that need this->oneLog to access base
//     . workaround: "auto& oneLog = *this;" in case
// ***********************************************************************************************
#ifndef UNI_LOG_HPP_
#define UNI_LOG_HPP_

// ***********************************************************************************************
#if SMART_LOG  // base on smartlog
#include "UniSmartLog.hpp"
using UniLog = RLib::UniSmartLog;

// ***********************************************************************************************
#else  // base on cout (LoggingSystem is similar)
#include "UniCoutLog.hpp"
using UniLog = RLib::UniCoutLog;
#endif

#endif  // UNI_LOG_HPP_
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2020-08-11  CSZ       1)create as CppLog
// 2020-10-29  CSZ       2)base on SmartLog
// 2021-02-13  CSZ       - empty to inc branch coverage (66%->75%)
// 2022-08-16  CSZ       4)replaced by UniLog
// 2022-08-25  CSZ       - support both SmartLog & cout
// 2022-08-29  CSZ       - split common to UniBaseLog.hpp
// 2022-11-10  CSZ       - ut smartlog & cout simultaneously
// 2022-11-11  CSZ       - simple & natural
// 2023-05-26  CSZ       5)const UniLog & derived can log
// ***********************************************************************************************
