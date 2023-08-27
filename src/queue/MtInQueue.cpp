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
size_t MtInQueue::mt_clear()
{
    lock_guard<mutex> guard(mutex_);

    auto queueSwap = queue<shared_ptr<void> >();
    cache_.swap(queueSwap);

    auto cacheSwap = queue<shared_ptr<void> >();
    queue_.swap(cacheSwap);

    return queueSwap.size() + cacheSwap.size();
}

// ***********************************************************************************************
shared_ptr<void> MtInQueue::pop()
{
    if (cache_.empty())
    {
        unique_lock<mutex> guard(mutex_, try_to_lock);  // avoid block main thread
        if (! guard.owns_lock())  // avoid block main thread
            return nullptr;
        if (queue_.empty())
            return nullptr;
        cache_.swap(queue_);  // fast & for at most ele
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

