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
//
// - why/value:
//   . pure Domino w/o hdlr (SOLID#1: single-responbility)
//   * basic hdlr for common usage
//     . and extendable
//   . support rm hdlr
//     . whenever succ, no cb, even cb already on road
//
// - core: hdlrs_
//
// - mem safe: yes
//   . no duty to hdlr itself's any unsafe behavior
//   . why shared_ptr rather than SafeAdr to store hdlr?
//     . HdlrDomino ensures safely usage of shared_ptr
//     . principle: safe class can base on unsafe materials
// ***********************************************************************************************
#ifndef HDLR_DOMINO_HPP_
#define HDLR_DOMINO_HPP_

#include <functional>
#include <memory>  // for shared_ptr
#include <unordered_map>

#include "MsgSelf.hpp"
#include "ObjAnywhere.hpp"
#include "UniLog.hpp"

namespace RLib
{
// ***********************************************************************************************
template<class aDominoType>
class HdlrDomino : public aDominoType
{
public:
    explicit HdlrDomino(const LogName& aUniLogName = ULN_DEFAULT) : aDominoType(aUniLogName) {}
    void setMsgSelf(const PTR<MsgSelf>& aMsgSelf) { msgSelf_ = aMsgSelf; }  // can replace default

    Domino::Event setHdlr(const Domino::EvName&, const MsgCB& aHdlr);
    bool rmOneHdlrOK(const Domino::EvName&);  // rm by EvName
    void forceAllHdlr(const Domino::EvName& aEN) { effect_(this->getEventBy(aEN)); }  // can't const as FreeDom rm hdlr
    virtual size_t nHdlr(const Domino::EvName& aEN) const { return hdlrs_.count(this->getEventBy(aEN)); }

    // -------------------------------------------------------------------------------------------
    // - add a new ev=aAliasEN to store aHdlr (aAliasEN's true prev is aHostEN)
    // . pros: can FreeHdlrDomino::repeatedHdlr() for each hdlr
    // . cons: the state of aHostEN & aAliasEN may not sync
    // -------------------------------------------------------------------------------------------
    Domino::Event multiHdlrByAliasEv(const Domino::EvName& aAliasEN, const MsgCB& aHdlr,
        const Domino::EvName& aHostEN);

    virtual EMsgPriority getPriority(const Domino::Event) const { return EMsgPri_NORM; }

protected:
    void effect_(const Domino::Event) override;
    virtual void triggerHdlr_(const SharedMsgCB& aHdlr, const Domino::Event aEv);
    virtual bool rmOneHdlrOK_(const Domino::Event&, const SharedMsgCB& aHdlr);  // rm by aHdlr

    void rmEv_(const Domino::Event) override;

    // -------------------------------------------------------------------------------------------
private:
    unordered_map<Domino::Event, SharedMsgCB> hdlrs_;
protected:
    PTR<MsgSelf> msgSelf_ = ObjAnywhere::get<MsgSelf>(*this);
public:
    using aDominoType::oneLog;
};

// ***********************************************************************************************
template<class aDominoType>
void HdlrDomino<aDominoType>::effect_(const Domino::Event aEv)
{
    auto&& it = hdlrs_.find(aEv);
    if (it == hdlrs_.end())
        return;

    DBG("(HdlrDom) Succeed to trigger 1 hdlr of EvName=" << this->evName_(aEv));
    triggerHdlr_(it->second, aEv);
}

// ***********************************************************************************************
template<class aDominoType>
Domino::Event HdlrDomino<aDominoType>::multiHdlrByAliasEv(const Domino::EvName& aAliasEN,
    const MsgCB& aHdlr, const Domino::EvName& aHostEN)
{
    auto&& event = this->setHdlr(aAliasEN, aHdlr);
    if (event == Domino::D_EVENT_FAILED_RET)
        return Domino::D_EVENT_FAILED_RET;

    return this->setPrev(aAliasEN, {{aHostEN, true}});
}

// ***********************************************************************************************
template<typename aDominoType>
void HdlrDomino<aDominoType>::rmEv_(const Domino::Event aEv)
{
    hdlrs_.erase(aEv);
    aDominoType::rmEv_(aEv);
}

// ***********************************************************************************************
template<class aDominoType>
bool HdlrDomino<aDominoType>::rmOneHdlrOK(const Domino::EvName& aEvName)
{
    HID("(HdlrDom) EvName=" << aEvName);
    return hdlrs_.erase(this->getEventBy(aEvName));
}

// ***********************************************************************************************
template<class aDominoType>
bool HdlrDomino<aDominoType>::rmOneHdlrOK_(const Domino::Event& aValidEv, const SharedMsgCB& aHdlr)
{
    auto&& itHdlr = hdlrs_.find(aValidEv);
    if (itHdlr == hdlrs_.end())
        return false;

    // req: must match
    if (itHdlr->second != aHdlr)
        return false;

    HID("(HdlrDom) Will remove hdlr of EvName=" << this->evName_(aValidEv)
        << ", nHdlrRef=" << itHdlr->second.use_count());
    hdlrs_.erase(itHdlr);
    return true;
}

// ***********************************************************************************************
template<class aDominoType>
Domino::Event HdlrDomino<aDominoType>::setHdlr(const Domino::EvName& aEvName, const MsgCB& aHdlr)
{
    if (! aHdlr)
    {
        WRN("(HdlrDom) Failed!!! not accept aHdlr=nullptr.");
        return Domino::D_EVENT_FAILED_RET;
    }

    auto&& event = this->newEvent(aEvName);
    if (hdlrs_.find(event) != hdlrs_.end())
    {
        WRN("(HdlrDom) Failed!!! Not support overwrite hdlr for " << aEvName << ". Use MultiHdlrDomino instead.");
        return Domino::D_EVENT_FAILED_RET;
    }

    auto hdlr = make_shared<MsgCB>(aHdlr);
    hdlrs_[event] = hdlr;
    HID("(HdlrDom) Succeed for EvName=" << aEvName);

    if (this->state_(event) == true)
    {
        DBG("(HdlrDom) Trigger the new hdlr of EvName=" << aEvName);
        triggerHdlr_(hdlr, event);
    }
    return event;
}

// ***********************************************************************************************
template<class aDominoType>
void HdlrDomino<aDominoType>::triggerHdlr_(const SharedMsgCB& aHdlr, const Domino::Event aEv)
{
    HID("(HdlrDom) trigger a new msg.");
    msgSelf_->newMsg(
        [weakMsgCB = WeakMsgCB(aHdlr)]() mutable  // WeakMsgCB is to support rm hdlr
        {
            auto cb = weakMsgCB.lock();
            if (cb)
                (*cb)();  // doesn't make sense that domino stores *cb==null
        },
        getPriority(aEv)
    );
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
// 2023-01-23  CSZ       - rename pureRmHdlrOK to rmOneHdlrOK
// 2023-05-24  CSZ       - support force call hdlr
// ***********************************************************************************************
