/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - what:
//   * multi-thread-safe queue - FIFO
//   . can store any type data
//   . high performance by pop-cache
// - why:
//   . multi-thread msg into MtInQueue, main thread read & handle (so call "In"Queue)
//   . req to "Out"Queue is not clear: seems main thread can send directly w/o block or via ThreadBack
//   . fetch() is not common, let class simple (eg std::queue than list)
//   . cache_: if mt_push() heavily, cache_ avoid ~all mutex from pop()
// - core:
//   . queue_
// - mem-safe: true (when use SafeAdr instead of shared_ptr)
// ***********************************************************************************************
#pragma once

#include <deque>
#include <functional>
#include <mutex>
#include <unordered_map>

#include "MT_PingMainTH.hpp"
#include "UniLog.hpp"
#include "UniPtr.hpp"

using namespace std;

namespace RLib
{
// ele & its typeid.hash_code
using ElePair = pair<UniPtr, size_t>;  // <ele, ID>
using EleHdlr = function<void(UniPtr)>;

// ***********************************************************************************************
class MtInQueue : public UniLog  // but mt_ interface can't UniLog that's not MT safe!!!
{
public:
    ~MtInQueue();

    template<class aEleType> void mt_push(PTR<aEleType> aEle);

    // shall be called in main thread ONLY!!!
    ElePair pop();  // high performance
    template<class aEleType> PTR<aEleType> pop() { return static_pointer_cast<aEleType>(pop().first); }

    size_t mt_sizeQ();
    void   mt_clear();

    // shall be called in main thread ONLY!!!
    template<class aEleType> void hdlr(const EleHdlr&);
    size_t handleAllEle();
    size_t nHdlr() const { return eleHdlrs_.size(); }

private:
    size_t handleCacheEle();

    // -------------------------------------------------------------------------------------------
    deque<ElePair> queue_;  // unlimited ele; most suitable container
    deque<ElePair> cache_;
    mutex mutex_;

    unordered_map<size_t, EleHdlr> eleHdlrs_;  // <ID, hdlr>

    // -------------------------------------------------------------------------------------------
#ifdef MT_IN_Q_TEST  // UT only
public:
    mutex& backdoor() { return mutex_; }
#endif
};

// ***********************************************************************************************
template<class aEleType>
void MtInQueue::hdlr(const EleHdlr& aHdlr)
{
    if (aHdlr)
      eleHdlrs_[typeid(aEleType).hash_code()] = aHdlr;
}

// ***********************************************************************************************
template<class aEleType>
void MtInQueue::mt_push(PTR<aEleType> aEle)
{
    {
        lock_guard<mutex> guard(mutex_);
        queue_.push_back(ElePair(aEle, typeid(aEleType).hash_code()));
        HID("(MtQ) ptr=" << aEle.get() << ", nRef=" << aEle.use_count());
    }   // unlock then mt_notifyFn()
    mt_pingMainTH();
}



// ***********************************************************************************************
// - REQ: can provide different impl w/o usr code impact
// - REQ: MT safe
//   . can't use ObjAnywhere that is not MT safe
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
// 2024-02-15  CSZ       3)use SafeAdr (mem-safe); shared_ptr is not mem-safe
// ***********************************************************************************************
