// ***********************************************************************************************
// - why/value:
//   . cpp style
//   . compatible CMT & SWM
//   . __LINE__ to locate
//   * SmartLog UT
// ***********************************************************************************************
#ifndef CPPLOG_HPP_
#define CPPLOG_HPP_

#include <gtest/gtest.h>

#include "StrCoutFSL.hpp"

namespace RLib
{
#if 1
// log_ instead of this->log_ so support static log_
#define BUF(content) log_.prefix_ << "] " << __func__ << "()" << __LINE__ << "#; " << content << std::endl
#define DBG(content) { CppLog::smartLog_ << "[DBG/" << BUF(content); }
#define INF(content) { CppLog::smartLog_ << "[INF/" << BUF(content); }
#define WRN(content) { CppLog::smartLog_ << "[WRN/" << BUF(content); }
#define ERR(content) { CppLog::smartLog_ << "[ERR/" << BUF(content); }
#define HID(content) { CppLog::smartLog_ << "[HID/" << BUF(content); }
#else  // eg code coverage
#define DBG(content) {}
#define INF(content) {}
#define WRN(content) {}
#define ERR(content) {}
#define HID(content) {}
#endif

class CppLog
{
public:
    explicit CppLog(std::string aPrefix) : prefix_(aPrefix) {}
    const std::string prefix_;

    static StrCoutFSL smartLog_;
};
}  // namespace
#endif  // CPPLOG_HPP_
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2020-08-11  CSZ       1)create
// 2020-10-29  CSZ       2)base on SmartLog
// 2021-02-13  CSZ       - empty to inc branch coverage (66%->75%)
// ***********************************************************************************************
