/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - what:
//   * multi-thread-safe queue - multi-thread in & main-thread out - FIFO
//   . can store any type data
//   . high performance by pop-cache
//
// - why:
//   . multi-thread msg into MtInQueue, main thread read & handle (so call "In"Queue)
//     . "Out"Queue is not clear: eg main thread can write socket directly w/o block or via ThreadBack
//   . fetch() in the middle of the queue is not common, let class simple (eg std::queue than list)
//   . cache_: if mt_push() heavily, cache_ avoid ~all mutex from pop()
//
// - core:
//   . queue_
//
// - class safe: true (when use SafePtr instead of shared_ptr)
//   . MtInQueue is for normal/most scenario, may NOK for high throughput, etc
// ***********************************************************************************************
#pragma once

#include <deque>
#include <functional>
#include <mutex>
#include <typeindex>
#include <unordered_map>
#include <utility>

#include "MT_PingMainTH.hpp"
#include "UniLog.hpp"
#include "UniPtr.hpp"

namespace rlib
{
// - ele & its type_index(==/!= ok, but type_info* & hash_code nok)
// - shared_ptr is safe to cast void since type_index (but not safe as SafePtr's create)
using ELE_TID = std::pair<UniPtr, std::type_index>;
using EleHdlr = std::function<void(UniPtr)>;    // NO exception allowed
using DftHdlr = std::function<void(ELE_TID&)>;  // NO exception allowed

// ***********************************************************************************************
class MtInQueue
{
public:
    MtInQueue() { setHdlrOK(); }
    ~MtInQueue();

    // aEle may not mt-safe, so here mv
    template<class aEleType> void mt_push(PTR<aEleType>&& aEle);

    // - shall be called in main thread ONLY!!!
    // - high performance
    ELE_TID pop();
    template<class aEleType> PTR<aEleType> pop();

    size_t mt_size(bool canBlock);
    void   mt_clearElePool();

    // shall be called in main thread ONLY!!!
    template<class aEleType> bool setHdlrOK(const EleHdlr&);
    bool setHdlrOK(const DftHdlr& aHdlr = [](ELE_TID& aEle_tid)
    {
        WRN("(MtQ) discard 1 ele(=" << aEle_tid.second.name() << ") since no handler");
    });
    size_t handleAllEle();
    auto nHdlr() const { return tid_hdlr_S_.size(); }
    void clearHdlrPool();

private:
    std::deque<ELE_TID>::iterator begin_();
    size_t  handleCacheEle_();

    // -------------------------------------------------------------------------------------------
    std::deque<ELE_TID> queue_;  // my limit ele# if need; most suitable container
    std::deque<ELE_TID> cache_;  // main-thread use ONLY (so no mutex protect)
    std::mutex mutex_;

    std::unordered_map<std::type_index, EleHdlr> tid_hdlr_S_;
    DftHdlr defaultHdlr_;

    // -------------------------------------------------------------------------------------------
#ifdef IN_GTEST
public:
    std::mutex& backdoor() { return mutex_; }
#endif
};

// ***********************************************************************************************
template<class aEleType>
void MtInQueue::mt_push(PTR<aEleType>&& aEle)
{
    // validate aEle
    if (aEle.get() == nullptr)
    {
        HID("!!! can't push nullptr since pop empty will ret nullptr");
        return;
    }
    // for MT safe: mt_push shall take over aEle's content (so sender can't touch the content)
    if (aEle.use_count() > 1)
    {
        HID("!!! push failed since use_count=" << aEle.use_count());  // ERR() is not MT safe
        return;
    }

    // push
    {
        std::lock_guard<std::mutex> guard(mutex_);
        // HID("(MtQ) nRef=" << aEle.use_count() << ", ptr=" << aEle.get());  // HID supports MT
        queue_.push_back(ELE_TID(std::move(aEle), typeid(aEleType)));
    }

    // unlock then notify main thread
    mt_pingMainTH();
}

// ***********************************************************************************************
template<class aEleType>
PTR<aEleType> MtInQueue::pop()
{
    // nothing
    auto&& it = begin_();
    if (it == cache_.end())
        return nullptr;

    // mismatch
    if (it->second != typeid(aEleType))
        return nullptr;

    // pop
    auto ele = std::move(it->first);
    cache_.pop_front();
    return std::static_pointer_cast<aEleType>(ele);
}

// ***********************************************************************************************
template<class aEleType>
bool MtInQueue::setHdlrOK(const EleHdlr& aHdlr)
{
    if (! aHdlr)
    {
        WRN("(MtQ) why set null hdlr?");
        return false;
    }

    auto&& tid = std::type_index(typeid(aEleType));
    if (tid_hdlr_S_.find(tid) != tid_hdlr_S_.end())
    {
        ERR("(MtQ) failed!!! overwrite hdlr may unsafe existing data");
        return false;
    }

    tid_hdlr_S_.emplace(tid, aHdlr);
    return true;
}



// ***********************************************************************************************
// - REQ: can provide different impl w/o usr code impact
// - REQ: MT safe
MtInQueue& mt_getQ();

}  // namespace
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2022-02-18  CSZ       1)create
// 2022-08-18  CSZ       - replace CppLog by UniLog
// 2022-11-23  CSZ       - rm since no need
// 2023-08-23  CSZ       - re-add since potential need
// 2023-08-24  CSZ       2)MtQueue -> MtInQueue
// 2023-09-08  CSZ       - store Ele with its hash_code
// 2023-09-18  CSZ       - support main thread wait() instead of keeping pop()
// 2023-10-26  CSZ       - replace mt_notifyFn_() by mt_pingMainTH()
// 2023-10-29  CSZ       - integrate handler
// 2024-02-15  CSZ       3)use SafePtr (mem-safe); shared_ptr is not mem-safe
// 2024-03-10  CSZ       - enhance safe of setHdlrOK()
// 2024-10-05  CSZ       - integrate with domino
// ***********************************************************************************************
