/**
 * Copyright 2020-2022 Nokia
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
// ***********************************************************************************************
#ifndef MULTI_HDLR_DOMINO_HPP_
#define MULTI_HDLR_DOMINO_HPP_

#include <map>
#include <string>
#include <unordered_map>

namespace RLib
{
// ***********************************************************************************************
template<class aDominoType>
class MultiHdlrDomino : public aDominoType
{
public:
    using HdlrName  = std::string;
    using MultiHdlr = std::map<HdlrName, SharedMsgCB>;

    // -------------------------------------------------------------------------------------------
    // - add multi-hdlr on 1 event
    // . cons: can NOT FreeHdlrDomino::flagRepeatedHdlr() for each hdlr
    // . pros: 1 state, always sync
    // -------------------------------------------------------------------------------------------
    Domino::Event multiHdlrOnSameEv(const Domino::EvName& aEvName, const MsgCB& aHdlr, const HdlrName& aHdlrName);

    using aDominoType::rmOneHdlrOK;
    bool rmOneHdlrOK(const Domino::EvName& aEvName, const HdlrName& aHdlrName);

protected:
    void effect(const Domino::Event) override;  // key/min change other Dominos
    bool pureRmHdlrOK(const Domino::Event& aEv, const SharedMsgCB& aHdlr) override;

private:
    // -------------------------------------------------------------------------------------------
    std::unordered_map<Domino::Event, MultiHdlr> multiHdlrs_;
public:
    using aDominoType::log_;
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
        auto&& itNext = std::next(itHdlr);
        DBG("(MultiHdlrDomino) Succeed to trigger 1 hdlr=" << itHdlr->first << " of EvName=" << this->evName(aEv));
        this->triggerHdlr(itHdlr->second, aEv);  // may rm itHdlr in FreeHdlrDomino
        itHdlr = itNext;
    }
}

// ***********************************************************************************************
template<class aDominoType>
Domino::Event MultiHdlrDomino<aDominoType>::multiHdlrOnSameEv(const Domino::EvName& aEvName,
    const MsgCB& aHdlr, const HdlrName& aHdlrName)
{
    auto&& hdlr = std::make_shared<MsgCB>(aHdlr);

    auto&& event = this->newEvent(aEvName);
    auto&& itEv = multiHdlrs_.find(event);
    if (itEv == multiHdlrs_.end())
        multiHdlrs_[event][aHdlrName] = hdlr;
    else
    {
        auto&& itHdlr = itEv->second.find(aHdlrName);
        if (itHdlr != itEv->second.end())
        {
            WRN("(MultiHdlrDomino)!!! Failed since dup EvName=" << aEvName << " + HdlrName=" << aHdlrName);
            return Domino::D_EVENT_FAILED_RET;
        }
        itEv->second[aHdlrName] = hdlr;
    }
    HID("(MultiHdlrDomino) Succeed for EvName=" << aEvName << ", HdlrName=" << aHdlrName);

    if (this->state(event))
    {
        DBG("(MultiHdlrDomino) Trigger the new hdlr=" << aHdlrName << "of EvName=" << aEvName);
        this->triggerHdlr(hdlr, event);
    }

    return event;
}

// ***********************************************************************************************
template<class aDominoType>
bool MultiHdlrDomino<aDominoType>::pureRmHdlrOK(const Domino::Event& aEv, const SharedMsgCB& aHdlr)
{
    const auto isRmOK = aDominoType::pureRmHdlrOK(aEv, aHdlr);
    if (isRmOK || not aHdlr)
        return isRmOK;

    // req: "ret true" means real rm
    auto&& itEv = multiHdlrs_.find(aEv);
    if (itEv == multiHdlrs_.end())
        return false;

    for (auto&& itHdlr = itEv->second.begin(); itHdlr != itEv->second.end(); ++itHdlr)
    {
        if (itHdlr->second != aHdlr)
            continue;

        // alert: mem-leak (fail ut)
        if (itHdlr->second.use_count() > 2)
            INF("(MultiHdlrDomino)!!! nHdlrRef=" << itHdlr->second.use_count());

        HID("(MultiHdlrDomino) Succeed to remove HdlrName=" << itHdlr->first << " of EvName=" << this->evName(aEv));
        itEv->second.erase(itHdlr);
        return true;
    }
    return false;
}

// ***********************************************************************************************
template<class aDominoType>
bool MultiHdlrDomino<aDominoType>::rmOneHdlrOK(const Domino::EvName& aEvName, const HdlrName& aHdlrName)
{
    auto&& itEv = multiHdlrs_.find(this->getEventBy(aEvName));
    if (itEv == multiHdlrs_.end()) return false;

    DBG("(MultiHdlrDomino) Succeed to remove HdlrName=" << aHdlrName << " of EvName=" << aEvName);
    return itEv->second.erase(aHdlrName);
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
// ***********************************************************************************************
