/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include "MtQueue.hpp"

namespace RLib {
// ***********************************************************************************************
std::shared_ptr<void> MtQueue::pop()
{
    std::lock_guard<std::mutex> guard(mutex_);

    if (queue_.size() == 0) return std::shared_ptr<void>();

    auto ele = queue_.front();
    queue_.pop_front();
    return ele;
}

// ***********************************************************************************************
std::shared_ptr<void> MtQueue::fetch(std::function<bool(std::shared_ptr<void>)> aMatcher)
{
    std::lock_guard<std::mutex> guard(mutex_);

    for (auto&& it = queue_.begin(); it != queue_.end();)
    {
        auto ele = *it;
        if (not aMatcher(ele))
        {
            ++it;
            continue;
        }

        queue_.erase(it);
        return ele;
    }

    return nullptr;
}

// ***********************************************************************************************
void MtQueue::push(std::shared_ptr<void> aEle)
{
    std::lock_guard<std::mutex> guard(mutex_);
    queue_.push_back(aEle);
}

// ***********************************************************************************************
size_t MtQueue::size()
{
    std::lock_guard<std::mutex> guard(mutex_);
    return queue_.size();
}
}  // namespace

