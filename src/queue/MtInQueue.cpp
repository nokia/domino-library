/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include <algorithm>
#include "MtInQueue.hpp"

namespace RLib
{
// ***********************************************************************************************
shared_ptr<void> MtInQueue::mt_pop()
{
    if (cache_.empty())
    {
        unique_lock<mutex> guard(mutex_, try_to_lock);  // avoid block main thread
        if (! guard.owns_lock())  // avoid block main thread
            return nullptr;
        if (queue_.empty())
            return nullptr;
        cache_.swap(queue_);
    }
    // unlocked

    auto ele = cache_.front();
    cache_.pop();
    return ele;
}

// ***********************************************************************************************
void MtInQueue::mt_push(shared_ptr<void> aEle)
{
    lock_guard<mutex> guard(mutex_);
    queue_.push(aEle);
}

// ***********************************************************************************************
size_t MtInQueue::mt_size()
{
    lock_guard<mutex> guard(mutex_);
    return queue_.size() + cache_.size();
}
}  // namespace

