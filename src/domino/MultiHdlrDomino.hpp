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
// - mem safe: yes
// ***********************************************************************************************
#ifndef MULTI_HDLR_DOMINO_HPP_
#define MULTI_HDLR_DOMINO_HPP_

#include <map>
#include <string>
#include <unordered_map>

#include "UniLog.hpp"

namespace RLib
{
// ***********************************************************************************************
template<class aDominoType>
class MultiHdlrDomino : public aDominoType
{
public:
    using HdlrName  = string;
    using MultiHdlr = map<HdlrName, SharedMsgCB>;

    explicit MultiHdlrDomino(const LogName& aUniLogName = ULN_DEFAULT) : aDominoType(aUniLogName) {}

    // -------------------------------------------------------------------------------------------
    // - add multi-hdlr on 1 event
    // . cons: can NOT FreeHdlrDomino::repeatedHdlr() for each hdlr
    // . pros: 1 state, always sync
    // -------------------------------------------------------------------------------------------
    Domino::Event multiHdlrOnSameEv(const Domino::EvName&, const MsgCB& aHdlr, const HdlrName&);

    using aDominoType::rmOneHdlrOK;  // rm HdlrDom's by EvName
    bool rmOneHdlrOK(const Domino::EvName&, const HdlrName&);  // rm MultiDom's by HdlrName
    void rmAllHdlr(const Domino::EvName&);
    size_t nHdlr(const Domino::EvName& aEN) const override;

protected:
    void effect_(const Domino::Event& aValidEv) override;  // key/min change other Dominos
    bool rmOneHdlrOK_(const Domino::Event& aValidEv, const SharedMsgCB& aValidHdlr) override;  // rm by aValidHdlr
    void rmEv_(const Domino::Event& aValidEv) override;

private:
    // -------------------------------------------------------------------------------------------
    unordered_map<Domino::Event, MultiHdlr> multiHdlrs_;
public:
    using aDominoType::oneLog;
};

// ***********************************************************************************************
template<class aDominoType>
void MultiHdlrDomino<aDominoType>::effect_(const Domino::Event& aValidEv)
{
    aDominoType::effect_(aValidEv);

    auto&& ev_hdlrs = multiHdlrs_.find(aValidEv);
    if (ev_hdlrs == multiHdlrs_.end())
        return;
    for (auto&& name_hdlr = ev_hdlrs->second.begin(); name_hdlr != ev_hdlrs->second.end();)
    {
        auto&& itNext = next(name_hdlr);
        DBG("(MultiHdlrDom) trigger 1 hdlr=" << name_hdlr->first << " of EvName=" << this->evName_(aValidEv));
        this->triggerHdlr_(name_hdlr->second, aValidEv);  // may rm name_hdlr in FreeHdlrDomino
        name_hdlr = itNext;
    }
}

// ***********************************************************************************************
template<class aDominoType>
Domino::Event MultiHdlrDomino<aDominoType>::multiHdlrOnSameEv(const Domino::EvName& aEvName,
    const MsgCB& aHdlr, const HdlrName& aHdlrName)
{
    if (aHdlr == nullptr)
    {
        WRN("(MultiHdlrDom) Failed!!! not accept aHdlr=nullptr.");
        return Domino::D_EVENT_FAILED_RET;
    }

    auto&& newHdlr = make_shared<MsgCB>(aHdlr);
    auto&& newEv = this->newEvent(aEvName);
    auto&& ev_hdlrs = multiHdlrs_.find(newEv);
    if (ev_hdlrs == multiHdlrs_.end())
        multiHdlrs_[newEv][aHdlrName] = newHdlr;
    else
    {
        auto&& name_hdlr = ev_hdlrs->second.find(aHdlrName);
        if (name_hdlr != ev_hdlrs->second.end())
        {
            WRN("(MultiHdlrDom)!!! Failed since dup EvName=" << aEvName << " + HdlrName=" << aHdlrName);
            return Domino::D_EVENT_FAILED_RET;
        }
        ev_hdlrs->second[aHdlrName] = newHdlr;
    }
    HID("(MultiHdlrDom) Succeed for EvName=" << aEvName << ", HdlrName=" << aHdlrName);

    if (this->state(newEv))
    {
        DBG("(MultiHdlrDom) Trigger the new hdlr=" << aHdlrName << "of EvName=" << aEvName);
        this->triggerHdlr_(newHdlr, newEv);
    }

    return newEv;
}

// ***********************************************************************************************
template<class aDominoType>
size_t MultiHdlrDomino<aDominoType>::nHdlr(const Domino::EvName& aEN) const
{
    HID("(MultiHdlrDom) aEN=" << aEN);
    auto&& ev_hdlrs = multiHdlrs_.find(this->getEventBy(aEN));
    return (ev_hdlrs == multiHdlrs_.end() ? 0 : ev_hdlrs->second.size()) + aDominoType::nHdlr(aEN);
}

// ***********************************************************************************************
template<class aDominoType>
void MultiHdlrDomino<aDominoType>::rmAllHdlr(const Domino::EvName& aEN)
{
    aDominoType::rmOneHdlrOK(aEN);

    multiHdlrs_.erase(this->getEventBy(aEN));
}

// ***********************************************************************************************
template<typename aDominoType>
void MultiHdlrDomino<aDominoType>::rmEv_(const Domino::Event& aValidEv)
{
    multiHdlrs_.erase(aValidEv);
    aDominoType::rmEv_(aValidEv);
}

// ***********************************************************************************************
template<class aDominoType>
bool MultiHdlrDomino<aDominoType>::rmOneHdlrOK(const Domino::EvName& aEvName, const HdlrName& aHdlrName)
{
    auto&& ev_hdlrs = multiHdlrs_.find(this->getEventBy(aEvName));
    if (ev_hdlrs == multiHdlrs_.end())
        return false;

    DBG("(MultiHdlrDom) Succeed to remove HdlrName=" << aHdlrName << " of EvName=" << aEvName);
    return ev_hdlrs->second.erase(aHdlrName);
}

// ***********************************************************************************************
template<class aDominoType>
bool MultiHdlrDomino<aDominoType>::rmOneHdlrOK_(const Domino::Event& aValidEv, const SharedMsgCB& aValidHdlr)
{
    if (aDominoType::rmOneHdlrOK_(aValidEv, aValidHdlr))
        return true;

    auto&& ev_hdlrs = multiHdlrs_.find(aValidEv);
    //if (ev_hdlrs == multiHdlrs_.end()) return false;  // impossible if valid aValidHdlr


    for (auto&& name_hdlr = ev_hdlrs->second.begin(); name_hdlr != ev_hdlrs->second.end(); ++name_hdlr)
    {
        if (name_hdlr->second != aValidHdlr)
            continue;

        HID("(MultiHdlrDom) Will remove HdlrName=" << name_hdlr->first << " of EvName=" << this->evName_(aValidEv)
            << ", nHdlrRef=" << name_hdlr->second.use_count());
        if (ev_hdlrs->second.size() > 1)
            ev_hdlrs->second.erase(name_hdlr);
        else
            multiHdlrs_.erase(ev_hdlrs);  // min mem (mem safe)
        return true;
    }
    return false;  // impossible if valid aValidHdlr
}

}  // namespace
#endif  // MULTI_HDLR_DOMINO_HPP_
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
// ***********************************************************************************************
