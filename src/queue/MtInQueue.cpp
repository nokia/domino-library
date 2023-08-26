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
    lock_guard<mutex> guard(mutex_);

    if (queue_.empty())
        return nullptr;

    auto ele = queue_.front();
    queue_.pop_front();
    return ele;
}

// ***********************************************************************************************
shared_ptr<void> MtInQueue::mt_fetch(const MatcherFN& aMatcherFN)
{
    lock_guard<mutex> guard(mutex_);

    auto it = std::find_if(queue_.begin(), queue_.end(), aMatcherFN);
    if (it == queue_.end())
        return nullptr;

    auto ele = *it;  // ensure erase(it) not destruct shared_ptr!!!
    queue_.erase(it);
    return ele;
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
    return queue_.size();
}
}  // namespace

