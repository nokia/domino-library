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
bool AsyncBack::newTaskOK(const MT_TaskEntryFN& mt_aEntryFN, const TaskBackFN& aBackFN, UniLog& oneLog) noexcept
{
    try {
        // validate
        if (! ThreadBack::newTaskOK(mt_aEntryFN, aBackFN, oneLog))
            return false;

        // create new thread
        // - impossible thread alive but AsyncBack destructed, since ~AsyncBack() will wait all fut over
        auto fut = async(
            launch::async,
            // - must cp mt_aEntryFN than ref, otherwise dead loop
            // - &mt_nDoneFut is better than "this" that can access other non-MT-safe member
            [mt_aEntryFN, &mt_nDoneFut = mt_nDoneFut_]() noexcept  // thread main
            {
                SafePtr ret;
                try { ret = mt_aEntryFN(); }
                catch(...) {}  // continue following

                mt_nDoneFut.fetch_add(1, std::memory_order_relaxed);  // fastest +1
                mt_pingMainTH();
                return ret;
            }
        );
        fut_backFN_S_.emplace_back(move(fut), aBackFN);
        return true;
    } catch(...) {  // - ut can't cover this branch
        ERR("(AsyncBack) exception to create new thread!!!");
        return false;
    }
}

}  // namespace
