/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - what:
//   . multi-thread-safe queue - FIFO
//   . can store any type data
//   . can find & fetch 1 element in the (middle of) the queue
// - why:
//   . eg netconf client,
//     . rpc reply & notification are all in the queue (multi-thread in)
//     . main thread can get & handle from the queue (multi-thread out)
// - core:
//   . queue_
// ***********************************************************************************************
#pragma once

#include <functional>
#include <list>
#include <memory>
#include <mutex>

namespace RLib {
// ***********************************************************************************************
class MtQueue
{
public:
    void push(std::shared_ptr<void> aEle);
    std::shared_ptr<void> pop();
    std::shared_ptr<void> fetch(std::function<bool(std::shared_ptr<void>)>);

    size_t size();

private:
    std::list<std::shared_ptr<void> > queue_;

    std::mutex mutex_;
};
}  // namespace
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2022-02-18  CSZ       - create  
// 2022-08-18  CSZ       - replace CppLog by UniLog
// ***********************************************************************************************
