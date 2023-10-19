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
// ***********************************************************************************************
#pragma once

#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>

using namespace std;

namespace RLib
{
// ele & its typeid.hash_code
// - hash_code is size_t (simplest) vs type_info (unworth to cost storage mem, & complex MtInQueue)
using ElePair = pair<shared_ptr<void>, size_t>;

// ***********************************************************************************************
class MtInQueue
{
public:
    // can't change mt_notifyFn_ -> MT safe
    MtInQueue(const function<void(void)>& aNotifyFn = nullptr) : mt_notifyFn_(aNotifyFn) {}

    template<class aEleType> void mt_push(shared_ptr<aEleType> aEle);

    // shall be called in main thread ONLY!!!
    ElePair pop();  // high performance
    template<class aEleType> shared_ptr<aEleType> pop() { return static_pointer_cast<aEleType>(pop().first); }

    size_t mt_size();
    size_t mt_clear();

private:
    void mt_notifyFn() { if (mt_notifyFn_) mt_notifyFn_(); }

    // -------------------------------------------------------------------------------------------
    deque<ElePair> queue_;  // unlimited ele; most suitable container
    deque<ElePair> cache_;
    mutex mutex_;
    function<void(void)> mt_notifyFn_;  // must be MT safety

    // -------------------------------------------------------------------------------------------
#ifdef MT_IN_Q_TEST  // UT only
public:
    mutex& backdoor() { return mutex_; }
#endif
};

// ***********************************************************************************************
template<class aEleType>
void MtInQueue::mt_push(shared_ptr<aEleType> aEle)
{
    {
        lock_guard<mutex> guard(mutex_);
        queue_.push_back(ElePair(aEle, typeid(aEleType).hash_code()));
    }  // unlock then mt_notifyFn()
    mt_notifyFn();
}

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
// ***********************************************************************************************
