/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include "UniCoutLog.hpp"

using namespace std;

namespace rlib
{
// ***********************************************************************************************
ostream& UniCoutLog::oneLog() noexcept
{
    try {
        ++nLogLine_;  // ut only; no impact product's MT safe & mem safe
        cout << "c[" << mt_timestamp() << ' ' << ULN_DEFAULT << '/';
    } catch(...) {}
    return cout;
}

// ***********************************************************************************************
UniCoutLog UniCoutLog::defaultUniLog_;
size_t     UniCoutLog::nLogLine_ = 0;

}  // namespaces
