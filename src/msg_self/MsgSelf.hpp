/**
 * Copyright 2016-2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - why: not callback directly but wait current function return to main() then callback
//   * avoid logic surprise when immediate callback [MUST-HAVE!]
//   . avoid deep/loop callbacks, exe after all executing/in-stack func returned
//   . priority FIFO msg (eg abort is higher priority)
//   . single thread (in main thread)
//   . encapsulate diff solution (async, IM, syscom, etc)
// - how:
//   . newMsg(): send msgHdlr into msgQueues_ (all info are in msgHdlr so func<void()> is enough)
//   . loopBack(): call all msgHdlr in msgQueues_, priority then FIFO
// - core: msgQueues_
// - which way?    speed                   UT                           code
//   . async task  may slow if async busy  no but direct-CB instead     simple
//   . IM          may ensure speed        no but direct-CB instead     Im, ActiveBtsL, StatusMo
//   . syscom      may ensure speed        ?                            ?
//   . (conclusion: async for product, direct-CB for UT)
// - multi-thread safe?
//   . no since msgQueues_
//   . FIFO is common need for many classes besides Domino
//   . SimpleAsyncTask is multi-thread safe
// ***********************************************************************************************
#ifndef MSG_SELF_HPP_
#define MSG_SELF_HPP_

#include <functional>
#include <memory>  // shared_ptr<>
#include <queue>

#include "CellLog.hpp"
#include "ObjAnywhere.hpp"

#define MSG_SELF (ObjAnywhere::get<MsgSelf>(*this))

namespace RLib
{
// ***********************************************************************************************
enum EMsgPriority : unsigned char
{
    EMsgPri_MIN,

    EMsgPri_LOW = EMsgPri_MIN,
    EMsgPri_NORM,
    EMsgPri_HIGH,

    EMsgPri_MAX
};

using MsgCB        = std::function<void()>;
using WeakMsgCB    = std::weak_ptr<MsgCB>;
using SharedMsgCB  = std::shared_ptr<MsgCB>;

using LoopBackFUNC = std::function<void()>;
using LoopReqFUNC  = std::function<void(LoopBackFUNC)>;

// ***********************************************************************************************
class MsgSelf : public CellLog
{
public:
    MsgSelf(LoopReqFUNC aFunc, const CellName&);
    ~MsgSelf();

    void newMsg(const WeakMsgCB& aMsgCB, const EMsgPriority = EMsgPri_NORM);
    const std::shared_ptr<bool> getValid() const { return isValid_; }

    bool hasMsg() const { return nMsg_; }
    static bool isLowPri(const EMsgPriority aPri) { return aPri < EMsgPri_NORM; }
    size_t nMsg(const EMsgPriority aPriority) const { return msgQueues_[aPriority].size(); }

private:
    bool handleOneMsg();
    void loopBack(const std::shared_ptr<bool> aValidMsgSelf = std::make_shared<bool>(true));

    // -------------------------------------------------------------------------------------------
    std::queue<WeakMsgCB> msgQueues_[EMsgPri_MAX];

    std::shared_ptr<bool> isValid_ = std::make_shared<bool>(true);  // MsgSelf is still valid?
    LoopReqFUNC loopReq_;
    size_t nMsg_ = 0;
};
}  // namespace
#endif  // MSG_SELF_HPP_
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2016-12-02  CSZ       1)create & support priority
// 2019-11-06  CSZ       - lower priority by 1msg/loopBack
// 2020-04-21  CSZ       - can loopBack() after MsgSelf is deleted
// 2020-08-10  CSZ       2)refactory to MsgSelf + AsyncLooper/ShortLooper/EmptyLooper/etc
// 2020-12-28  CSZ       3)fix invalid cb by weak_ptr<cb>
// 2021-04-05  CSZ       - coding req
// 2021-11-29  CSZ       - lambda instead of bind; fix isValid ut
// 2022-01-01  PJ & CSZ  - formal log & naming
// 2022-03-10  CSZ       - inc code coverage rate
// 2022-08-18  CSZ       - replace CppLog by CellLog
// ***********************************************************************************************// ***********************************************************************************************
