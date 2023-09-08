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
using MatcherFN = function<bool(shared_ptr<void>)>;
using ElePair   = pair<shared_ptr<void>, size_t>;  // ele & its typeid.hash_code

// ***********************************************************************************************
class MtInQueue
{
public:
    size_t mt_size();
    size_t mt_clear();

    template<class aEleType> void mt_push(shared_ptr<aEleType> aEle);

    // shall access in main thread ONLY!!! high performance
    ElePair pop();
    template<class aEleType> shared_ptr<aEleType> pop() { return static_pointer_cast<aEleType>(pop().first); }

    // -------------------------------------------------------------------------------------------
private:
    deque<ElePair> queue_;  // unlimited ele; most suitable container
    mutex mutex_;
    deque<ElePair> cache_;

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
    lock_guard<mutex> guard(mutex_);
    queue_.push_back(ElePair(aEle, typeid(aEle).hash_code()));
}
}  // namespace
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2022-02-18  CSZ       1)create
// 2022-08-18  CSZ       - replace CppLog by UniLog
// 2022-11-23  CSZ       - rm since no need
// 2023-08-23  CSZ       - re-add since potential need
// 2023-08024  CSZ       2)MtQueue -> MtInQueue
// ***********************************************************************************************
