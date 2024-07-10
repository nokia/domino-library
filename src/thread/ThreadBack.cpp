/**
 * Copyright 2022 Nokia. All rights reserved.
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <functional>
#include <future>
#include <iostream>
#include <memory>

#include "ThreadBack.hpp"

namespace RLib
{
// ***********************************************************************************************
size_t ThreadBack::hdlFinishedTasks(UniLog& oneLog)
{
    size_t nHandled = 0;
    for (auto&& fut_backFN = fut_backFN_S_.begin(); fut_backFN != fut_backFN_S_.end();)  // may limit# if too many
    {
        // safer to check valid here than in newTaskOK(), eg bug after newTaskOK()
        auto&& fut = fut_backFN->first;
        const bool over = ! fut.valid() || fut.wait_for(0s) == future_status::ready;
        HID("(ThreadBack) nHandled=" << nHandled << ", valid=" << fut.valid() << ", backFn=" << &(fut_backFN->second));
        if (over)
        {
            fut_backFN->second(fut.valid() && fut.get());  // callback
            fut_backFN = fut_backFN_S_.erase(fut_backFN);
            ++nHandled;
        }
        else
            ++fut_backFN;
    }  // 1 loop, simple & safe
    return nHandled;
}

}  // namespace
