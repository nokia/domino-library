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
    queue_.pop();
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
    return queue_.size();
}
}  // namespace

