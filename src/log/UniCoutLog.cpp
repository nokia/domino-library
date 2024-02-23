/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include "UniCoutLog.hpp"

namespace RLib
{
// ***********************************************************************************************
ostream& UniCoutLog::oneLog()
{
    ++nLogLine_;  // ut only; no impact product's MT safe & mem safe
    cout << "c[" << timestamp() << ' ' << ULN_DEFAULT << '/';
    return cout;
}

// ***********************************************************************************************
UniCoutLog UniCoutLog::defaultUniLog_;
size_t     UniCoutLog::nLogLine_ = 0;

}  // namespaces
