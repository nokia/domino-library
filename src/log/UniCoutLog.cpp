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
shared_ptr<UniCoutLog> UniCoutLog::defaultUniLog()
{
    if (not defaultUniLog_)
        defaultUniLog_ = make_shared<UniCoutLog>();
    return defaultUniLog_;
}

// ***********************************************************************************************
ostream& UniCoutLog::oneLog() const
{
    cout << "c[" << timestamp() << ' ' << ULN_DEFAULT << '/';
    ++nLogLine_;
    return cout;
}

// ***********************************************************************************************
size_t                 UniCoutLog::nLogLine_ = 0;
shared_ptr<UniCoutLog> UniCoutLog::defaultUniLog_;

}  // namespaces
