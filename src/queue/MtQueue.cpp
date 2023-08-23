/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include <algorithm>
#include "MtQueue.hpp"

namespace RLib
{
// ***********************************************************************************************
shared_ptr<void> MtQueue::mt_pop()
{
    lock_guard<mutex> guard(mutex_);

    if (queue_.empty())
        return nullptr;

    auto ele = queue_.front();
    queue_.pop_front();
    return ele;
}

// ***********************************************************************************************
shared_ptr<void> MtQueue::mt_fetch(const MatcherFN& aMatcherFN)
{
    lock_guard<mutex> guard(mutex_);

    auto ele = std::find_if(queue_.begin(), queue_.end(), aMatcherFN);
    if (ele == queue_.end())
        return nullptr;

    queue_.erase(ele);
    return *ele;
}

// ***********************************************************************************************
void MtQueue::mt_push(shared_ptr<void> aEle)
{
    lock_guard<mutex> guard(mutex_);
    queue_.push_back(aEle);
}

// ***********************************************************************************************
size_t MtQueue::mt_size()
{
    lock_guard<mutex> guard(mutex_);
    return queue_.size();
}
}  // namespace

