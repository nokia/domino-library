/**
 * Copyright 2024 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "AsyncBack.hpp"

using namespace std;

namespace RLib
{
// ***********************************************************************************************
bool AsyncBack::newTaskOK(const MT_TaskEntryFN& mt_aEntryFN, const TaskBackFN& aBackFN, UniLog& oneLog)
{
    // validate
    if (! aBackFN)
    {
        ERR("(AsyncBack) aBackFN=null doesn't make sense!!! Why not async() directly?");
        return false;
    }
    if (! mt_aEntryFN)
    {
        ERR("(AsyncBack) NOT support mt_aEntryFN=null!!! Necessary?");
        return false;
    }

    // create new thread
    fut_backFN_S_.emplace_back(  // save future<> & aBackFN()
        async(
            launch::async,
            [mt_aEntryFN]()  // must cp than ref, otherwise dead loop
            {
                const bool ret = mt_aEntryFN();
                mt_pingMainTH();
                return ret;
            }
        ),
        aBackFN
    );
    HID("(AsyncBack) valid=" << fut_backFN_S_.back().first.valid() << ", backFn=" << &(fut_backFN_S_.back().second));
    return true;
}

}  // namespace
