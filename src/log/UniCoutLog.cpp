/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include "UniLog.hpp"
#include "UniCoutLog.hpp"

namespace RLib
{

// ***********************************************************************************************
UniCoutLog& UniCoutLog::defaultUniLog()
{
    if (not defaultUniLog_) defaultUniLog_ = std::make_shared<UniCoutLog>();
    return *defaultUniLog_;
}

// ***********************************************************************************************
std::ostream& UniCoutLog::oneLog()
{
    std::cout << '[' << uniLogName_ << '/';
    ++nLogLine_;
    return std::cout;
}

// ***********************************************************************************************
size_t UniCoutLog::nLogLine_ = 0;
std::shared_ptr<UniCoutLog> UniCoutLog::defaultUniLog_;

}  // namespaces

