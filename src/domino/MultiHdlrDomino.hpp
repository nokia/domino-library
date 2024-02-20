/**
 * Copyright 2020 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - what:
//   . Domino can support only 1 hdlr
//   . MultiHdlrDomino can support more hdlrs
// - why:
//   . simplify/min Domino to most usecases
//   . extend multi-hdlr if needed
//   . legacy Domino uses multi-event to support multi-hdlr, but these events' state not auto-sync always
// - core:
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

    explicit MultiHdlrDomino(const UniLogName& aUniLogName) : aDominoType(aUniLogName) {}

    // -------------------------------------------------------------------------------------------
    // - add multi-hdlr on 1 event
    // . cons: can NOT FreeHdlrDomino::repeatedHdlr() for each hdlr
    // . pros: 1 state, always sync
    // -------------------------------------------------------------------------------------------
    Domino::Event multiHdlrOnSameEv(const Domino::EvName&, const MsgCB& aHdlr, const HdlrName&);

    using aDominoType::rmOneHdlrOK;  // rm HdlrDom's by EvName
    bool rmOneHdlrOK(const Domino::EvName&, const HdlrName&);  // rm MultiDom's by HdlrName
    bool rmOneHdlrOK(const Domino::Event&, const SharedMsgCB& aHdlr) override;  // rm by aHdlr
    void rmAllHdlr(const Domino::EvName&);
    size_t nHdlr(const Domino::EvName& aEN) const override;

protected:
    void effect(const Domino::Event) override;  // key/min change other Dominos
    void innerRmEv(const Domino::Event) override;

private:
    // -------------------------------------------------------------------------------------------
    unordered_map<Domino::Event, MultiHdlr> multiHdlrs_;
public:
    using aDominoType::oneLog;
};

// ***********************************************************************************************
template<class aDominoType>
void MultiHdlrDomino<aDominoType>::effect(const Domino::Event aEv)
{
    aDominoType::effect(aEv);

    auto&& itEv = multiHdlrs_.find(aEv);
    if (itEv == multiHdlrs_.end())
        return;
    for (auto&& itHdlr = itEv->second.begin(); itHdlr != itEv->second.end();)
    {
        auto&& itNext = next(itHdlr);
        DBG("(MultiHdlrDom) Succeed to trigger 1 hdlr=" << itHdlr->first << " of EvName=" << this->evName(aEv));
        this->triggerHdlr(itHdlr->second, aEv);  // may rm itHdlr in FreeHdlrDomino
        itHdlr = itNext;
    }
}

// ***********************************************************************************************
template<typename aDominoType>
void MultiHdlrDomino<aDominoType>::innerRmEv(const Domino::Event aEv)
{
    multiHdlrs_.erase(aEv);
    aDominoType::innerRmEv(aEv);
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

    auto&& hdlr = make_shared<MsgCB>(aHdlr);
    auto&& event = this->newEvent(aEvName);
    auto&& itEv = multiHdlrs_.find(event);
    if (itEv == multiHdlrs_.end())
        multiHdlrs_[event][aHdlrName] = hdlr;
    else
    {
        auto&& itHdlr = itEv->second.find(aHdlrName);
        if (itHdlr != itEv->second.end())
        {
            WRN("(MultiHdlrDom)!!! Failed since dup EvName=" << aEvName << " + HdlrName=" << aHdlrName);
            return Domino::D_EVENT_FAILED_RET;
        }
        itEv->second[aHdlrName] = hdlr;
    }
    HID("(MultiHdlrDom) Succeed for EvName=" << aEvName << ", HdlrName=" << aHdlrName);

    if (this->state(event))
    {
        DBG("(MultiHdlrDom) Trigger the new hdlr=" << aHdlrName << "of EvName=" << aEvName);
        this->triggerHdlr(hdlr, event);
    }

    return event;
}

// ***********************************************************************************************
template<class aDominoType>
size_t MultiHdlrDomino<aDominoType>::nHdlr(const Domino::EvName& aEN) const
{
    HID("(MultiHdlrDom) aEN=" << aEN);
    auto&& it = multiHdlrs_.find(this->getEventBy(aEN));
    return (it == multiHdlrs_.end() ? 0 : it->second.size()) + aDominoType::nHdlr(aEN);
}

// ***********************************************************************************************
template<class aDominoType>
void MultiHdlrDomino<aDominoType>::rmAllHdlr(const Domino::EvName& aEN)
{
    aDominoType::rmOneHdlrOK(aEN);

    multiHdlrs_.erase(this->getEventBy(aEN));
}

// ***********************************************************************************************
template<class aDominoType>
bool MultiHdlrDomino<aDominoType>::rmOneHdlrOK(const Domino::EvName& aEvName, const HdlrName& aHdlrName)
{
    auto&& itEv = multiHdlrs_.find(this->getEventBy(aEvName));
    if (itEv == multiHdlrs_.end())
        return false;

    DBG("(MultiHdlrDom) Succeed to remove HdlrName=" << aHdlrName << " of EvName=" << aEvName);
    return itEv->second.erase(aHdlrName);
}

// ***********************************************************************************************
template<class aDominoType>
bool MultiHdlrDomino<aDominoType>::rmOneHdlrOK(const Domino::Event& aEv, const SharedMsgCB& aHdlr)
{
    if (aDominoType::rmOneHdlrOK(aEv, aHdlr))
        return true;

    // req: "ret true" means real rm
    auto&& itEv = multiHdlrs_.find(aEv);
    if (itEv == multiHdlrs_.end())
        return false;

    for (auto&& itHdlr = itEv->second.begin(); itHdlr != itEv->second.end(); ++itHdlr)
    {
        if (itHdlr->second != aHdlr)
            continue;

        HID("(MultiHdlrDom) Will remove HdlrName=" << itHdlr->first << " of EvName=" << this->evName(aEv)
            << ", nHdlrRef=" << itHdlr->second.use_count());
        itEv->second.erase(itHdlr);
        return true;
    }
    return false;
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
