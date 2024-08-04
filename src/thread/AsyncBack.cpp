/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "AsyncBack.hpp"

using namespace std;

namespace rlib
{
// ***********************************************************************************************
bool AsyncBack::newTaskOK(const MT_TaskEntryFN& mt_aEntryFN, const TaskBackFN& aBackFN, UniLog& oneLog)
{
    // validate
    if (! ThreadBack::newTaskOK(mt_aEntryFN, aBackFN, oneLog))
        return false;

    // create new thread
    fut_backFN_S_.emplace_back(  // save future<> & aBackFN()
        async(
            launch::async,
            [mt_aEntryFN, this]()  // must cp than ref, otherwise dead loop
            {
                const bool ret = mt_aEntryFN();
                this->nDoneTh_.fetch_add(1, std::memory_order_relaxed);  // fastest +1
                mt_pingMainTH();
                return ret;
            }
        ),
        aBackFN
    );
    // HID("(AsyncBack) valid=" << fut_backFN_S_.back().first.valid() << ", backFn=" << &(fut_backFN_S_.back().second));
    return true;
}

}  // namespace
