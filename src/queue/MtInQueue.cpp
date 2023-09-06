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
void MtInQueue::mt_push(shared_ptr<void> aEle)
{
    lock_guard<mutex> guard(mutex_);
    queue_.push_back(aEle);
}

// ***********************************************************************************************
size_t MtInQueue::mt_size()
{
    lock_guard<mutex> guard(mutex_);
    return queue_.size() + cache_.size();
}
}  // namespace

