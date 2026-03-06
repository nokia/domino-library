/**
 * Copyright 2024 Nokia. All rights reserved.
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "ThreadBack.hpp"

#include <cassert>

using namespace std;

namespace rlib
{
// ***********************************************************************************************
size_t ThreadBack::hdlDoneFut(UniLog& oneLog) noexcept
{
    assert(mt_inMyMainTH());  // non-thread-safe, must call from main thread
    size_t nHandledFut = 0;
    const auto nDoneFut = mt_nDoneFut_.load(memory_order_relaxed);  // since mt_nDoneFut_+1 may before future::ready
    if (nDoneFut == 0) return 0;
    // HID("(ThreadBack) nHandled=" << nHandledFut << '/' << nDoneFut << '|' << nFut());

    // bugFix: may mt_nDoneFut_+1 before future::ready, so must check size
    for (size_t i = 0; i < fut_backFN_S_.size() && nHandledFut < nDoneFut;)
    {
        // - async() failure will throw exception -> terminate since compiling forbid exception
        // - valid async()'s future never invalid
        // - valid packaged_task's get_future() never invalid
        auto& fut = fut_backFN_S_[i].first;
        if (fut.wait_for(0s) == future_status::ready)
        {
            auto task_pair = std::move(fut_backFN_S_[i]);
            // swap-erase: move last element into this slot, then pop_back
            if (i + 1 < fut_backFN_S_.size())
                fut_backFN_S_[i] = std::move(fut_backFN_S_.back());
            fut_backFN_S_.pop_back();
            ++nHandledFut;

            SafePtr ret;
            try { ret = task_pair.first.get(); }
            catch(...) { ERR("(ThreadBack) entryFN() except"); }  // ERR() ok since in main thread

            try { task_pair.second(move(ret)); }  // callback
            catch(...) { ERR("(ThreadBack) backFN() except"); }  // ERR() ok since in main thread
        }
        else
            ++i;
    }  // 1 loop, simple & safe

    mt_nDoneFut_.fetch_sub(nHandledFut, memory_order_relaxed);  // memory_order_relaxed is faster but not so realtime
    return nHandledFut;
}

// ***********************************************************************************************
bool ThreadBack::newTaskOK(MT_TaskEntryFN mt_aEntryFN, TaskBackFN aBackFN, UniLog& oneLog)
{
    assert(mt_inMyMainTH());  // non-thread-safe, must call from main thread
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
