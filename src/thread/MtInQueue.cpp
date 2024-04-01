/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include <thread>

#include "MT_PingMainTH.hpp"
#include "MtInQueue.hpp"

namespace RLib
{
// ***********************************************************************************************
MtInQueue::~MtInQueue()
{
    const auto nEle = mt_sizeQ(true);
    if (nEle)
        WRN("discard nEle=" << nEle);  // main thread can WRN()
}

// ***********************************************************************************************
deque<ELE_TID>::iterator MtInQueue::begin_()
{
    if (cache_.empty())
    {
        unique_lock<mutex> guard(mutex_, try_to_lock);  // avoid block main thread
        if (! guard.owns_lock())
        {
            mt_pingMainTH();  // since waste this wakeup as not own the lock
            this_thread::yield();  // avoid main thread keep checking
            return cache_.end();
        }
        if (queue_.empty())
            return cache_.end();
        cache_.swap(queue_);  // fast & for at most ele
    }
    // unlocked

    HID("(MtQ) ptr=" << cache_.begin()->first.get() << ", nRef=" << cache_.begin()->first.use_count());
    return cache_.begin();
}

// ***********************************************************************************************
size_t MtInQueue::handleCacheEle_()
{
    const auto nEle = cache_.size();
    while (! cache_.empty())
    {
        auto ele_tid = cache_.front();
        cache_.pop_front();

        auto&& id_hdlr = tid_hdlr_S_.find(ele_tid.second);
        if (id_hdlr == tid_hdlr_S_.end())
        {
            WRN("(MtQ) discard 1 ele(tid=" << ele_tid.second->name() << ") since no handler.")
            continue;
        }
        id_hdlr->second(ele_tid.first);
    }  // while
    return nEle;
}

// ***********************************************************************************************
size_t MtInQueue::handleAllEle()
{
    const auto nEle = handleCacheEle_();

    {
        unique_lock<mutex> guard(mutex_, try_to_lock);  // avoid block main thread
        if (! guard.owns_lock())
        {
            mt_pingMainTH();  // for possible ele in queue_
            this_thread::yield();  // avoid main thread keep checking; no ut for optimization
            return nEle;
        }
        cache_.swap(queue_);
    }
    return nEle + handleCacheEle_();
}

// ***********************************************************************************************
void MtInQueue::mt_clear()
{
    lock_guard<mutex> guard(mutex_);
    queue_.clear();
    cache_.clear();
    tid_hdlr_S_.clear();
}

// ***********************************************************************************************
size_t MtInQueue::mt_sizeQ(bool canBlock)
{
    if (canBlock)
    {
        lock_guard<mutex> guard(mutex_);
        return queue_.size() + cache_.size();
    }

    // non block
    unique_lock<mutex> tryGuard(mutex_, try_to_lock);
    HID(__LINE__ << " owns=" << tryGuard.owns_lock());
    return tryGuard.owns_lock()
        ? queue_.size() + cache_.size()
        : cache_.size();
}

// ***********************************************************************************************
ELE_TID MtInQueue::pop()
{
    // nothing
    auto&& it = begin_();
    if (it == cache_.end())
        return ELE_TID(nullptr, nullptr);

    // pop
    auto ele_tid = *it;  // must copy
    cache_.pop_front();
    return ele_tid;
}


// ***********************************************************************************************
MtInQueue& mt_getQ()
{
    // - not exist if nobody call this func
    // - c++14 support MT safe for static var
    static MtInQueue s_mtQ;

    return s_mtQ;
}

}  // namespace
