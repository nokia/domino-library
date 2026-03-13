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
        *out_ << "c[" << mt_timestamp() << ' ' << ULN_DEFAULT << '/';
    } catch(...) {}
    return *out_;
}

// ***********************************************************************************************
bool UniCoutLog::setLogFileOK(const string& aFileName) noexcept
{
    try {
        if (aFileName.empty())
        {
            cout << "INF(UniCoutLog): switch to cout" << endl;
            out_ = &std::cout;
            return true;
        }

        ofstream newFile(aFileName, ios::app);
        if (! newFile)
        {
            cout << "ERR(UniCoutLog): can't open log file " << aFileName << endl;
            return false;
        }

        cout << "INF(UniCoutLog): switch to log file " << aFileName << endl;
        file_ = std::move(newFile);
        out_ = &file_;
        return true;
    } catch (...)
    {
        cout << "ERR(UniCoutLog): exception when open log file " << aFileName << endl;
        return false;
    }
}

// ***********************************************************************************************
UniCoutLog           UniCoutLog::defaultUniLog_;
std::atomic<size_t>  UniCoutLog::nLogLine_ = 0;
std::ostream*        UniCoutLog::out_ = &std::cout;
std::ofstream        UniCoutLog::file_;

}  // namespaces
