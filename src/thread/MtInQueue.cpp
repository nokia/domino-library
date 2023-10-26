/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include "MtInQueue.hpp"

namespace RLib
{
// ***********************************************************************************************
size_t MtInQueue::mt_clear()
{
    lock_guard<mutex> guard(mutex_);

    const auto sizeQueue = queue_.size();
    queue_.clear();

    const auto sizeCache = cache_.size();
    cache_.clear();

    return sizeQueue + sizeCache;
}

// ***********************************************************************************************
size_t MtInQueue::mt_size()
{
    lock_guard<mutex> guard(mutex_);
    return queue_.size() + cache_.size();
}

// ***********************************************************************************************
ElePair MtInQueue::pop()
{
    if (cache_.empty())
    {
        unique_lock<mutex> guard(mutex_, try_to_lock);  // avoid block main thread
        if (! guard.owns_lock())  // avoid block main thread
        {
            mt_pingMainTH();  // since waste this wakeup as not own the lock
            return ElePair(nullptr, typeid(void).hash_code());
        }
        if (queue_.empty())
            return ElePair(nullptr, typeid(void).hash_code());
        cache_.swap(queue_);  // fast & for at most ele
    }
    // unlocked

    auto elePair = cache_.front();
    cache_.pop_front();
    return elePair;
}

}  // namespace
