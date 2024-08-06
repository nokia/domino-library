/**
 * Copyright 2024 Nokia. All rights reserved.
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <functional>
#include <future>
#include <iostream>
#include <memory>

#include "ThreadBack.hpp"

using namespace std;

namespace rlib
{
// ***********************************************************************************************
size_t ThreadBack::hdlDoneFut(UniLog& oneLog)
{
    size_t nHandledFut = 0;
    const auto nDoneFut = nDoneFut_.load(memory_order_relaxed);  // since possible nDoneFut_+1 before future::ready
    // HID("(ThreadBack) nHandled=" << nHandledFut << '/' << nDoneFut << '|' << nFut());

    // bugFix: if nDoneFut_+1 before future::ready, must check "fut_backFN != fut_backFN_S_.end()"
    for (auto&& fut_backFN = fut_backFN_S_.begin(); nHandledFut < nDoneFut && fut_backFN != fut_backFN_S_.end();)
    {
        // - async() failure will throw exception -> terminate since compiling forbid exception
        // - valid async()'s future never invalid
        // - valid packaged_task's get_future() never invalid
        auto& fut = fut_backFN->first;
        if (fut.wait_for(0s) == future_status::ready)
        {
            fut_backFN->second(fut.get());  // callback
            fut_backFN = fut_backFN_S_.erase(fut_backFN);
            ++nHandledFut;
        }
        else
            ++fut_backFN;
    }  // 1 loop, simple & safe

    nDoneFut_.fetch_sub(nHandledFut, memory_order_relaxed);
    return nHandledFut;
}

// ***********************************************************************************************
bool ThreadBack::newTaskOK(const MT_TaskEntryFN& mt_aEntryFN, const TaskBackFN& aBackFN, UniLog& oneLog)
{
    if (! aBackFN)
    {
        ERR("(ThreadBack) aBackFN=null doesn't make sense!!! Why not async() directly?");
        return false;
    }
    if (! mt_aEntryFN)
    {
        ERR("(ThreadBack) NOT support mt_aEntryFN=null!!! Necessary?");
        return false;
    }
    return true;
}

}  // namespace
