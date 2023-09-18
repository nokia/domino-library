/**
 * Copyright 2016 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - Issue/why: not callback directly but wait current function return to main() then callback
//   . avoid loop callbacks, but exe after all executing/in-stack func returned
//   * avoid logic surprise when immediate callback [MUST-HAVE!]
//   * priority FIFO msg (eg "abort" is higher priority)
//   . single thread (in main thread)
//   * support diff cb mechanism (async, IM, syscom, etc)
//   . can withdraw on-road MsgCB (eg HdlrDomino.rmHdlr())
//   . destruct MsgSelf shall not call MsgCB in msgQueues_, & no mem-leak
// - how:
//   . newMsg(): send msgHdlr into msgQueues_ (all info are in msgHdlr so func<void()> is enough)
//   . handleAllMsg(): call all msgHdlr in msgQueues_, priority then FIFO
// - core: msgQueues_[priority][FIFO]
//   . msgQueues_ is a 2D array, 1st dim is priority, 2nd dim is FIFO
//   . perf better than priority_queue that need search & insert for newMsg()
//
// - which way?    speed                   UT                           code
//   . async task  may slow if async busy  no but direct-CB instead     simple
//   . IM          may ensure speed        no but direct-CB instead     Im, ActiveBtsL, StatusMo
//   . syscom      may ensure speed        ?                            ?
//   . (conclusion: async for product, direct-CB for UT)
// - why MsgCB instead of WeakMsgCB in msgQueues_?
//   . most users want MsgSelf (instead of themselves) to store cb (naturally; so MsgCB is better)
//   . while only a few want to be able to withdraw cb in msgQueues_ (eg HdlrDomino; so WeakMsgCB is better)
// ***********************************************************************************************
#ifndef MSG_SELF_HPP_
#define MSG_SELF_HPP_

#include <functional>
#include <memory>  // shared_ptr<>
#include <deque>

#include "UniLog.hpp"

#define MSG_SELF (ObjAnywhere::get<MsgSelf>(*this))

using namespace std;

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

using MsgCB        = function<void()>;  // !!! MsgCB shall NEVER throw exception!!!
using WeakMsgCB    = weak_ptr<MsgCB>;
using SharedMsgCB  = shared_ptr<MsgCB>;

using PongMainFN   = function<void()>;                   // tap from main()
using PingMainFN   = function<void(const PongMainFN&)>;  // tap to main()

// ***********************************************************************************************
class MsgSelf : public UniLog
{
public:
    MsgSelf(const PingMainFN&, const UniLogName& = ULN_DEFAULT);
    ~MsgSelf();
    const shared_ptr<bool> getValid() const { return isValid_; }

    void   newMsg(const MsgCB&, const EMsgPriority = EMsgPri_NORM);  // can't withdraw CB but easier usage
    bool   hasMsg() const { return nMsg_; }
    size_t nMsg(const EMsgPriority aPriority) const { return msgQueues_[aPriority].size(); }

    static bool isLowPri(const EMsgPriority aPri) { return aPri < EMsgPri_NORM; }

private:
    void handleAllMsg(const shared_ptr<bool> aValidMsgSelf = make_shared<bool>(true));
    bool handleOneMsg();

    // -------------------------------------------------------------------------------------------
    deque<MsgCB> msgQueues_[EMsgPri_MAX];

    shared_ptr<bool> isValid_ = make_shared<bool>(true);  // MsgSelf is still valid?
    PingMainFN       pingMainFN_;
    size_t           nMsg_ = 0;
};
}  // namespace
#endif  // MSG_SELF_HPP_
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2016-12-02  CSZ       1)create & support priority
// 2019-11-06  CSZ       - lower priority by 1msg/handleAllMsg
// 2020-04-21  CSZ       - no crash to callback handleAllMsg() after MsgSelf is deleted
// 2020-08-10  CSZ       2)refactory to MsgSelf + AsyncLooper/ShortLooper/EmptyLooper/etc
// 2020-12-28  CSZ       3)fix invalid cb by weak_ptr<cb>
// 2021-04-05  CSZ       - coding req
// 2021-11-29  CSZ       - lambda instead of bind; fix isValid ut
// 2022-01-01  PJ & CSZ  - formal log & naming
// 2022-03-10  CSZ       - inc code coverage rate
// 2022-08-18  CSZ       - replace CppLog by UniLog
// 2022-11-04  CSZ       4)simplify newMsg(WeakMsgCB -> MsgCB)
//                         . easier to user
//                         . support both MsgCB & WeakMsgCB(by lambda)
// 2022-12-02  CSZ       - simple & natural
// 2022-12-31  CSZ       - not support MsgCB=nullptr
// 2023-07-13  CSZ       - copilot compare
// ***********************************************************************************************
