/**
 * Copyright 2020 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - what issue:
//   * complex to keep hdlr in Domino
//   . state & hdlr are different core
//   . so mv hdlr out Domino
// - why/value:
//   . pure Domino w/o hdlr (SOLID#1: single-responbility)
//   * basic hdlr for common usage
//     . and extendable
//   . support rm hdlr
//     . whenever succ, no cb, even cb already on road
// - core: hdlrs_
// ***********************************************************************************************
#ifndef HDLR_DOMINO_HPP_
#define HDLR_DOMINO_HPP_

#include <functional>
#include <memory>  // for shared_ptr
#include <unordered_map>

#include "MsgSelf.hpp"
#include "ObjAnywhere.hpp"

namespace RLib
{
// ***********************************************************************************************
template<class aDominoType>
class HdlrDomino : public aDominoType
{
public:
    explicit HdlrDomino(const UniLogName& aUniLogName) : aDominoType(aUniLogName) { msgSelf_ = MSG_SELF; }  // default
    void setMsgSelf(shared_ptr<MsgSelf>& aMsgSelf) { msgSelf_ = aMsgSelf; }  // can replace default

    Domino::Event setHdlr(const Domino::EvName&, const MsgCB& aHdlr);
    bool rmOneHdlrOK(const Domino::EvName&);

    // -------------------------------------------------------------------------------------------
    // - add a new ev=aAliasEN to store aHdlr (aAliasEN's true prev is aHostEN)
    // . pros: can FreeHdlrDomino::repeatedHdlr() for each hdlr
    // . cons: the state of aHostEN & aAliasEN may not sync
    // -------------------------------------------------------------------------------------------
    Domino::Event multiHdlrByAliasEv(const Domino::EvName& aAliasEN, const MsgCB& aHdlr,
        const Domino::EvName& aHostEN);

    virtual EMsgPriority getPriority(const Domino::Event) const { return EMsgPri_NORM; }

protected:
    void effect(const Domino::Event) override;
    virtual void triggerHdlr(const SharedMsgCB& aHdlr, const Domino::Event aEv)
    {
        msgSelf_->newMsg(
            [weakMsgCB = WeakMsgCB(aHdlr)]() mutable  // WeakMsgCB is to support rm hdlr
            {
                auto cb = weakMsgCB.lock();
                if (cb) (*cb)();  // doesn't make sense that domino stores *cb==null
            },
            getPriority(aEv)
        );
    }
    virtual bool pureRmHdlrOK(const Domino::Event&, const SharedMsgCB& aHdlr = SharedMsgCB());

    size_t nHdlrRef(const Domino::Event) const;

    // -------------------------------------------------------------------------------------------
private:
    unordered_map<Domino::Event, SharedMsgCB> hdlrs_;
protected:
    shared_ptr<MsgSelf> msgSelf_;
public:
    using aDominoType::oneLog;
};

// ***********************************************************************************************
template<class aDominoType>
void HdlrDomino<aDominoType>::effect(const Domino::Event aEv)
{
    auto&& it = hdlrs_.find(aEv);
    if (it == hdlrs_.end()) return;

    DBG("(HdlrDomino) Succeed to trigger 1 hdlr of EvName=" << this->evName(aEv));
    triggerHdlr(it->second, aEv);
}

// ***********************************************************************************************
template<class aDominoType>
Domino::Event HdlrDomino<aDominoType>::multiHdlrByAliasEv(const Domino::EvName& aAliasEN,
    const MsgCB& aHdlr, const Domino::EvName& aHostEN)
{
    auto&& event = this->setHdlr(aAliasEN, aHdlr);
    if (event == Domino::D_EVENT_FAILED_RET) return Domino::D_EVENT_FAILED_RET;

    return this->setPrev(aAliasEN, {{aHostEN, true}});
}

// ***********************************************************************************************
template<class aDominoType>
size_t HdlrDomino<aDominoType>::nHdlrRef(const Domino::Event aEvent) const
{
    auto&& it = hdlrs_.find(aEvent);
    return it == hdlrs_.end() ? Domino::D_EVENT_FAILED_RET : it->second.use_count();
}

// ***********************************************************************************************
template<class aDominoType>
bool HdlrDomino<aDominoType>::pureRmHdlrOK(const Domino::Event& aEv, const SharedMsgCB& aHdlr)
{
    // req: "ret true" means real rm
    auto&& itHdlr = hdlrs_.find(aEv);
    if (itHdlr == hdlrs_.end())
        return false;

    // req: must match
    if (aHdlr && itHdlr->second != aHdlr)
        return false;

    HID("(HdlrDomino) Succeed to remove hdlr of EvName=" << this->evName(aEv)
        << ", nHdlrRef=" << itHdlr->second.use_count());
    hdlrs_.erase(itHdlr);
    return true;
}

// ***********************************************************************************************
template<class aDominoType>
bool HdlrDomino<aDominoType>::rmOneHdlrOK(const Domino::EvName& aEvName)
{
    HID("(HdlrDomino) EvName=" << aEvName);
    return hdlrs_.erase(this->getEventBy(aEvName)) == 1;  // 0 or >1 are NOK
}

// ***********************************************************************************************
template<class aDominoType>
Domino::Event HdlrDomino<aDominoType>::setHdlr(const Domino::EvName& aEvName, const MsgCB& aHdlr)
{
    if (aHdlr == nullptr)
    {
        WRN("(HdlrDomino) Failed!!! not accept aHdlr=nullptr.");
        return Domino::D_EVENT_FAILED_RET;
    }

    auto&& event = this->newEvent(aEvName);
    if (hdlrs_.find(event) != hdlrs_.end())
    {
        WRN("(HdlrDomino) Failed!!! Not support overwrite hdlr for " << aEvName << ". Use MultiHdlrDomino instead.");
        return Domino::D_EVENT_FAILED_RET;
    }

    auto hdlr = make_shared<MsgCB>(aHdlr);
    hdlrs_[event] = hdlr;
    HID("(HdlrDomino) Succeed for EvName=" << aEvName);

    if (this->state(event) == true)
    {
        DBG("(HdlrDomino) Trigger the new hdlr of EvName=" << aEvName);
        triggerHdlr(hdlr, event);
    }
    return event;
}
}  // namespace
#endif  // HDLR_DOMINO_HPP_
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   ......................................................................
// 2020-12-11  CSZ       1)create
// 2020-12-28  CSZ       2)fix invalid cb by weak_ptr<cb>
// 2021-04-01  CSZ       - coding req
// 2022-01-04  PJ & CSZ  - formal log & naming
// 2022-03-26  CSZ       - ut's PARA_DOM include self class & ALL its base class(es)
// 2022-03-27  CSZ       - if ut case can test base class, never specify derive
// 2022-08-18  CSZ       - replace CppLog by UniLog
// ***********************************************************************************************
