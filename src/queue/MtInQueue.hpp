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
// - core:
//   . queue_
// ***********************************************************************************************
#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <queue>

using namespace std;

namespace RLib
{
using MatcherFN = function<bool(shared_ptr<void>)>;

// ***********************************************************************************************
class MtInQueue
{
public:
    void mt_push(shared_ptr<void> aEle);
    shared_ptr<void> mt_pop();

    size_t mt_size();

private:
    queue<shared_ptr<void> > queue_;
    mutex mutex_;
};
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
