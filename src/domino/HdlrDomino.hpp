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
// - core: ev_hdlr_S_
//
// - safe: yes
//   . no duty to hdlr itself's any unsafe behavior
//   . why shared_ptr rather than SafeAdr to store hdlr?
//     . HdlrDomino ensures safely usage of shared_ptr
//     . principle: safe class can base on unsafe materials
// ***********************************************************************************************
#pragma once

#include <functional>
#include <unordered_map>

#include "MsgSelf.hpp"
#include "ObjAnywhere.hpp"
#include "UniLog.hpp"
#include "UniPtr.hpp"

namespace RLib
{
// ***********************************************************************************************
template<class aDominoType>
class HdlrDomino : public aDominoType
{
public:
    explicit HdlrDomino(const LogName& aUniLogName = ULN_DEFAULT) : aDominoType(aUniLogName) {}
    bool setMsgSelfOK(const PTR<MsgSelf>& aMsgSelf);  // replace default

    Domino::Event setHdlr(const Domino::EvName&, const MsgCB& aHdlr);
    bool rmOneHdlrOK(const Domino::EvName&);  // rm by EvName
    void forceAllHdlr(const Domino::EvName& aEN) { effect_(this->getEventBy(aEN)); }
    virtual size_t nHdlr(const Domino::EvName& aEN) const { return ev_hdlr_S_.count(this->getEventBy(aEN)); }

    // -------------------------------------------------------------------------------------------
    // - add a new ev=aAliasEN to store aHdlr (aAliasEN's true prev is aHostEN)
    // . pros: can FreeHdlrDomino::repeatedHdlr() for each hdlr
    // . cons: the state of aHostEN & aAliasEN may not sync
    // -------------------------------------------------------------------------------------------
    Domino::Event multiHdlrByAliasEv(const Domino::EvName& aAliasEN, const MsgCB& aHdlr,
        const Domino::EvName& aHostEN);

    virtual EMsgPriority getPriority(const Domino::Event&) const { return EMsgPri_NORM; }

protected:
    void effect_(const Domino::Event& aEv) override;
    virtual void triggerHdlr_(const SharedMsgCB& aValidHdlr, const Domino::Event& aValidEv);
    virtual bool rmOneHdlrOK_(const Domino::Event& aValidEv, const SharedMsgCB& aValidHdlr);  // rm by aValidHdlr

    void rmEv_(const Domino::Event& aValidEv) override;

    // -------------------------------------------------------------------------------------------
private:
    unordered_map<Domino::Event, SharedMsgCB> ev_hdlr_S_;
protected:
    PTR<MsgSelf> msgSelf_ = ObjAnywhere::get<MsgSelf>(*this);
public:
    using aDominoType::oneLog;
};

// ***********************************************************************************************
template<class aDominoType>
void HdlrDomino<aDominoType>::effect_(const Domino::Event& aEv)
{
    // validate
    auto&& ev_hdlr = ev_hdlr_S_.find(aEv);
    if (ev_hdlr == ev_hdlr_S_.end())
        return;

    HID("(HdlrDom) Succeed to trigger 1 hdlr of EvName=" << this->evName_(aEv));
    triggerHdlr_(ev_hdlr->second, aEv);
}

// ***********************************************************************************************
template<class aDominoType>
Domino::Event HdlrDomino<aDominoType>::multiHdlrByAliasEv(const Domino::EvName& aAliasEN,
    const MsgCB& aHdlr, const Domino::EvName& aHostEN)
{
    if (this->getEventBy(aAliasEN) != Domino::D_EVENT_FAILED_RET)
    {
        ERR("(HdlrDom) fail since already exist en=" << aAliasEN << " to avoid complex scenario"
            " eg setHdlr() ok but setPrev() failed, or setHdlr() cb but setPrev() unsatisfied");
        return Domino::D_EVENT_FAILED_RET;
    }

    // set hdlr
    auto&& newEv = this->setHdlr(aAliasEN, aHdlr);
    if (newEv == Domino::D_EVENT_FAILED_RET)  // setHdlr failed
        return Domino::D_EVENT_FAILED_RET;

    // auto set prev
    return this->setPrev(aAliasEN, {{aHostEN, true}});
}

// ***********************************************************************************************
template<typename aDominoType>
void HdlrDomino<aDominoType>::rmEv_(const Domino::Event& aValidEv)
{
    ev_hdlr_S_.erase(aValidEv);
    aDominoType::rmEv_(aValidEv);
}

// ***********************************************************************************************
template<class aDominoType>
bool HdlrDomino<aDominoType>::rmOneHdlrOK(const Domino::EvName& aEvName)
{
    HID("(HdlrDom) EvName=" << aEvName);
    return ev_hdlr_S_.erase(this->getEventBy(aEvName));
}

// ***********************************************************************************************
template<class aDominoType>
bool HdlrDomino<aDominoType>::rmOneHdlrOK_(const Domino::Event& aValidEv, const SharedMsgCB& aValidHdlr)
{
    auto&& ev_hdlr = ev_hdlr_S_.find(aValidEv);
    if (ev_hdlr == ev_hdlr_S_.end())
        return false;

    // req: must match
    if (ev_hdlr->second != aValidHdlr)
        return false;

    HID("(HdlrDom) Will remove hdlr of EvName=" << this->evName_(aValidEv)
        << ", nHdlrRef=" << ev_hdlr->second.use_count());
    ev_hdlr_S_.erase(ev_hdlr);
    return true;
}

// ***********************************************************************************************
template<class aDominoType>
Domino::Event HdlrDomino<aDominoType>::setHdlr(const Domino::EvName& aEvName, const MsgCB& aHdlr)
{
    // validate
    if (! aHdlr)
    {
        WRN("(HdlrDom) Failed!!! not accept aHdlr=nullptr.");
        return Domino::D_EVENT_FAILED_RET;
    }
    auto&& newEv = this->newEvent(aEvName);
    if (ev_hdlr_S_.find(newEv) != ev_hdlr_S_.end())
    {
        WRN("(HdlrDom) Failed!!! Can't overwrite hdlr for " << aEvName << ". Rm old or Use MultiHdlrDomino instead.");
        return Domino::D_EVENT_FAILED_RET;
    }

    // set
    auto newHdlr = make_shared<MsgCB>(aHdlr);
    ev_hdlr_S_[newEv] = newHdlr;
    HID("(HdlrDom) Succeed for EvName=" << aEvName);

    // call
    if (this->state(newEv) == true)
    {
        HID("(HdlrDom) Trigger the new hdlr of EvName=" << aEvName);
        triggerHdlr_(newHdlr, newEv);
    }
    return newEv;
}

// ***********************************************************************************************
template<class aDominoType>
bool HdlrDomino<aDominoType>::setMsgSelfOK(const PTR<MsgSelf>& aMsgSelf)
{
    // validate
    const auto nMsgUnhandled = msgSelf_->nMsg();  // HdlrDomino ensure msgSelf_ always NOT null
    if (nMsgUnhandled > 0)
    {
        ERR("(MsgSelf) failed!!! since old msgSelf is not empty, nMsgUnhandled=" << nMsgUnhandled);
        return false;
    }
    if (aMsgSelf.get() == nullptr)
    {
        ERR("Failed!!! since HdlrDomino unsafe when msgSelf==nullptr");
        return false;
    }

    // set
    msgSelf_ = aMsgSelf;
    return true;
}

// ***********************************************************************************************
template<class aDominoType>
void HdlrDomino<aDominoType>::triggerHdlr_(const SharedMsgCB& aValidHdlr, const Domino::Event& aValidEv)
{
    HID("(HdlrDom) trigger a new msg.");
    msgSelf_->newMsg(
        [weakMsgCB = WeakMsgCB(aValidHdlr)]() mutable  // WeakMsgCB is to support rm hdlr
        {
            auto cb = weakMsgCB.lock();
            if (cb)
                (*cb)();  // setHdlr() forbid cb==null
        },
        getPriority(aValidEv)
    );
}

}  // namespace
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
// 2024-03-10  CSZ       - enhance safe eg setMsgSelf()
// ***********************************************************************************************
