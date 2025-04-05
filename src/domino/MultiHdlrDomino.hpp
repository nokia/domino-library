/**
 * Copyright 2020 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - what:
//   . Domino can support only 1 hdlr
//   . MultiHdlrDomino can support more hdlrs
//
// - why:
//   . simplify/min Domino to most usecases
//   . extend multi-hdlr if needed
//   . legacy Domino uses multi-event to support multi-hdlr, but these events' state not auto-sync always
//
// - core:
//
// - class safe: yes
// ***********************************************************************************************
#pragma once

#include <map>
#include <memory>
#include <string>
#include <unordered_map>

#include "UniLog.hpp"

namespace rlib
{
// ***********************************************************************************************
template<class aDominoType>
class MultiHdlrDomino : public aDominoType
{
public:
    using HdlrName  = std::string;
    using HName_Hdlr_S = std::map<HdlrName, SharedMsgCB>;

    explicit MultiHdlrDomino(const LogName& aUniLogName = ULN_DEFAULT) noexcept : aDominoType(aUniLogName) {}

    // -------------------------------------------------------------------------------------------
    // - add multi-hdlr on 1 event
    // . cons: can NOT FreeHdlrDomino::repeatedHdlr() for each hdlr
    // . pros: 1 state, always sync
    // -------------------------------------------------------------------------------------------
    Domino::Event multiHdlrOnSameEv(const Domino::EvName&, const MsgCB& aHdlr, const HdlrName&) noexcept;

    using aDominoType::rmOneHdlrOK;  // rm HdlrDom's by EvName
    bool rmOneHdlrOK(const Domino::EvName&, const HdlrName&) noexcept;  // rm MultiDom's by HdlrName
    void rmAllHdlr(const Domino::EvName&) noexcept;
    size_t nHdlr(const Domino::EvName& aEN) const noexcept override;

protected:
    void effect_(const Domino::Event& aEv) noexcept override;  // key/min change other Dominos
    bool rmOneHdlrOK_(const Domino::Event& aValidEv, const SharedMsgCB& aValidHdlr) noexcept override; // by aValidHdlr
    void rmEv_(const Domino::Event& aValidEv) override;

private:
    // -------------------------------------------------------------------------------------------
    std::unordered_map<Domino::Event, HName_Hdlr_S> ev_hdlrs_S_;
public:
    using aDominoType::oneLog;
};

// ***********************************************************************************************
template<class aDominoType>
void MultiHdlrDomino<aDominoType>::effect_(const Domino::Event& aEv) noexcept
{
    // call parent's hdlr
    aDominoType::effect_(aEv);

    // validate
    auto&& ev_hdlrs = ev_hdlrs_S_.find(aEv);
    if (ev_hdlrs == ev_hdlrs_S_.end())
        return;

    // call my hdlr(s)
    for (auto&& name_hdlr = ev_hdlrs->second.begin(); name_hdlr != ev_hdlrs->second.end();)
    {
        auto&& itNext = next(name_hdlr);
        HID("(MultiHdlrDom) trigger 1 hdlr=" << name_hdlr->first << " of EvName=" << this->evName_(aEv));
        this->triggerHdlr_(name_hdlr->second, aEv);  // may rm name_hdlr in FreeHdlrDomino
        name_hdlr = itNext;
    }
}

// ***********************************************************************************************
template<class aDominoType>
Domino::Event MultiHdlrDomino<aDominoType>::multiHdlrOnSameEv(const Domino::EvName& aEvName,
    const MsgCB& aHdlr, const HdlrName& aHdlrName) noexcept
{
    // validate
    if (aHdlr == nullptr)
    {
        WRN("(MultiHdlrDom) Failed!!! not accept aHdlr=nullptr.");
        return Domino::D_EVENT_FAILED_RET;
    }

    // set hdlr
    auto&& newHdlr = MAKE_PTR<MsgCB>(aHdlr);
    auto&& ev = this->getEventBy(aEvName);
    auto&& ev_hdlrs = ev_hdlrs_S_.find(ev);
    if (ev_hdlrs == ev_hdlrs_S_.end())
    {
        ev = this->newEvent(aEvName);
        ev_hdlrs_S_[ev].emplace(aHdlrName, newHdlr);
    }
    else
    {
        auto&& name_hdlr = ev_hdlrs->second.find(aHdlrName);
        if (name_hdlr != ev_hdlrs->second.end())
        {
            WRN("(MultiHdlrDom)!!! Failed since dup EvName=" << aEvName << " + HdlrName=" << aHdlrName);
            return Domino::D_EVENT_FAILED_RET;
        }
        ev_hdlrs->second.emplace(aHdlrName, newHdlr);
    }
    HID("(MultiHdlrDom) Succeed for EvName=" << aEvName << ", HdlrName=" << aHdlrName);

    // call hdlr
    if (this->state(ev))
    {
        HID("(MultiHdlrDom) Trigger the new hdlr=" << aHdlrName << "of EvName=" << aEvName);
        this->triggerHdlr_(newHdlr, ev);
    }

    return ev;
}

// ***********************************************************************************************
template<class aDominoType>
size_t MultiHdlrDomino<aDominoType>::nHdlr(const Domino::EvName& aEN) const noexcept
{
    auto&& ev_hdlrs = ev_hdlrs_S_.find(this->getEventBy(aEN));
    return (ev_hdlrs == ev_hdlrs_S_.end() ? 0 : ev_hdlrs->second.size()) + aDominoType::nHdlr(aEN);
}

// ***********************************************************************************************
template<class aDominoType>
void MultiHdlrDomino<aDominoType>::rmAllHdlr(const Domino::EvName& aEN) noexcept
{
    aDominoType::rmOneHdlrOK(aEN);

    ev_hdlrs_S_.erase(this->getEventBy(aEN));
}

// ***********************************************************************************************
template<typename aDominoType>
void MultiHdlrDomino<aDominoType>::rmEv_(const Domino::Event& aValidEv)
{
    ev_hdlrs_S_.erase(aValidEv);
    aDominoType::rmEv_(aValidEv);
}

// ***********************************************************************************************
template<class aDominoType>
bool MultiHdlrDomino<aDominoType>::rmOneHdlrOK(const Domino::EvName& aEvName, const HdlrName& aHdlrName) noexcept
{
    // find
    auto&& ev_hdlrs = ev_hdlrs_S_.find(this->getEventBy(aEvName));
    if (ev_hdlrs == ev_hdlrs_S_.end())
        return false;

    // rm
    INF("(MultiHdlrDom) Succeed to remove HdlrName=" << aHdlrName << " of EvName=" << aEvName);
    return ev_hdlrs->second.erase(aHdlrName);
}

// ***********************************************************************************************
template<class aDominoType>
bool MultiHdlrDomino<aDominoType>::rmOneHdlrOK_(const Domino::Event& aValidEv, const SharedMsgCB& aValidHdlr) noexcept
{
    // parent's hdlr?
    if (aDominoType::rmOneHdlrOK_(aValidEv, aValidHdlr))
        return true;

    // rm
    auto&& ev_hdlrs = ev_hdlrs_S_.find(aValidEv);
    // if (ev_hdlrs == ev_hdlrs_S_.end()) return false;  // impossible if valid aValidHdlr
    for (auto&& name_hdlr = ev_hdlrs->second.begin(); name_hdlr != ev_hdlrs->second.end(); ++name_hdlr)
    {
        if (name_hdlr->second != aValidHdlr)
            continue;

        HID("(MultiHdlrDom) Will remove HdlrName=" << name_hdlr->first << " of EvName=" << this->evName_(aValidEv)
            << ", nHdlrRef=" << name_hdlr->second.use_count());
        if (ev_hdlrs->second.size() > 1)
            ev_hdlrs->second.erase(name_hdlr);
        else
            ev_hdlrs_S_.erase(ev_hdlrs);  // min mem (mem safe)
        return true;
    }
    return false;  // impossible if valid aValidHdlr
}

}  // namespace
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2020-11-17  CSZ       1)create
// 2020-12-28  CSZ       2)fix invalid cb by weak_ptr<cb>
// 2020-12-29  CSZ       . clean ut cases
// 2021-04-01  CSZ       - coding req
// 2022-01-04  PJ & CSZ  - formal log & naming
// 2022-03-26  CSZ       - ut's PARA_DOM include self class & ALL its base class(es)
// 2022-03-27  CSZ       - if ut case can test base class, never specify derive
// 2022-08-18  CSZ       - replace CppLog by UniLog
// 2023-05-25  CSZ       - support force call hdlr
// 2023-05-29  CSZ       - rmAllHdlr
// 2025-02-13  CSZ       - support both SafePtr & shared_ptr
// 2025-04-05  CSZ       3)tolerate exception
// ***********************************************************************************************
