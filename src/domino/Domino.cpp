/**
 * Copyright 2018 Nokia. All rights reserved.
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <string>

#include "Domino.hpp"

using namespace std;

namespace RLib
{
static const Domino::Events defaultEVs;  // internal use only

// ***********************************************************************************************
void Domino::deduceStateFrom_(const Event& aValidEv)
{
    HID("(Domino) en=" << evName_(aValidEv));

    // recalc self state
    auto newState = deduceStateSelf_(aValidEv, true) && deduceStateSelf_(aValidEv, false);
    if (pureSetStateOK_(aValidEv, newState))
    {
        // deduce impacted (true branch)
        for (auto&& nextEV : findPeerEVs(aValidEv, next_[true]))
            deduceStateFrom_(nextEV);

        // deduce impacted (false branch)
        for (auto&& nextEV : findPeerEVs(aValidEv, next_[false]))
            deduceStateFrom_(nextEV);
    }
}

// ***********************************************************************************************
bool Domino::deduceStateSelf_(const Event& aValidEv, bool aPrevType) const
{
    for (auto&& prevEV : findPeerEVs(aValidEv, prev_[aPrevType]))
        if (states_[prevEV] != aPrevType)  // 1 prev not satisfied
            return false;
    return true;
}

// ***********************************************************************************************
void Domino::effect_()
{
    for (auto&& ev : effectEVs_)
        if (state(ev) == true)  // avoid multi-change
            effect_(ev);
    effectEVs_.clear();
}

// ***********************************************************************************************
const Domino::Events& Domino::findPeerEVs(const Event& aEv, const EvLinks& aLinks)
{
    auto ev_peerEVs = aLinks.find(aEv);
    return ev_peerEVs == aLinks.end()
        ? defaultEVs
        : ev_peerEVs->second;
}

// ***********************************************************************************************
bool Domino::isNextFromTo_(const Event& aFromValidEv, const Event& aToValidEv) const
{
    if (aFromValidEv == aToValidEv)  // self loop
        return true;
    if (isNextFromToVia_(aFromValidEv, aToValidEv, next_[true]))  // loop via T-br
        return true;
    return isNextFromToVia_(aFromValidEv, aToValidEv, next_[false]);  // may loop via F-br
}
bool Domino::isNextFromToVia_(const Event& aFromValidEv, const Event& aToValidEv, const EvLinks& aViaEvLinks) const
{
    for (auto&& nextEV : findPeerEVs(aFromValidEv, aViaEvLinks))
    {
        if (nextEV == aToValidEv)
            return true;
        else if (isNextFromTo_(nextEV, aToValidEv))
            return true;
    }
    return false;
}

// ***********************************************************************************************
Domino::Event Domino::getEventBy(const EvName& aEvName) const
{
    auto&& en_ev = en_ev_.find(aEvName);
    return en_ev == en_ev_.end()
        ? D_EVENT_FAILED_RET
        : en_ev->second;
}

// ***********************************************************************************************
Domino::Event Domino::newEvent(const EvName& aEvName)
{
    // exist?
    auto&& newEv = getEventBy(aEvName);
    if (newEv != D_EVENT_FAILED_RET)
        return newEv;

    // new from rm-ed
    newEv = recycleEv_();
    if (newEv == D_EVENT_FAILED_RET)
        newEv = states_.size();

    HID("(Domino) init new EvName=" << aEvName << ", event=" << newEv);
    en_ev_[aEvName] = newEv;
    ev_en_[newEv] = aEvName;
    states_.push_back(false);
    return newEv;
}

// ***********************************************************************************************
void Domino::pureRmLink_(const Event& aValidEv, EvLinks& aMyLinks, EvLinks& aNeighborLinks)
{
    // rm neighbor's link
    for (auto&& peerEv : findPeerEVs(aValidEv, aMyLinks))
    {
        auto&& neighborLinks = aNeighborLinks[peerEv];
        if (neighborLinks.size() <= 1)
            aNeighborLinks.erase(peerEv);  // erase entire
        else
            neighborLinks.erase(aValidEv);  // erase 1
    }

    // rm my link
    aMyLinks.erase(aValidEv);
}

// ***********************************************************************************************
void Domino::pureSetPrev_(const Event& aValidEv, const SimuEvents& aSimuPrevEvents)
{
    HID("(Domino) before: nPrev[true]=" << prev_[true].size() << ", nNext[true]=" << next_[true].size()
        << ", nPrev[false]=" << prev_[false].size() << ", nNext[false]=" << next_[false].size());
    for (auto&& prevEn_state : aSimuPrevEvents)
    {
        auto&& prevEv = newEvent(prevEn_state.first);
        prev_[prevEn_state.second][aValidEv].insert(prevEv);
        next_[prevEn_state.second][prevEv].insert(aValidEv);
    }
    HID("(Domino) after: nPrev[true]=" << prev_[true].size() << ", nNext[true]=" << next_[true].size()
        << ", nPrev[false]=" << prev_[false].size() << ", nNext[false]=" << next_[false].size());
}

// ***********************************************************************************************
bool Domino::pureSetStateOK_(const Event& aValidEv, const bool aNewState)
{
    if (states_[aValidEv] != aNewState) // do need change
    {
        states_[aValidEv] = aNewState;
        HID("(Domino) Succeed, EvName=" << ev_en_[aValidEv] << " newState=" << aNewState);
        if (aNewState == true)
            effectEVs_.insert(aValidEv);
        return true;
    }
    return false;
}

// ***********************************************************************************************
void Domino::rmEv_(const Event& aValidEv)
{
    // cp for later deduceStateFrom_(next)
    auto trueNextEVs  = findPeerEVs(aValidEv, next_[true]);
    auto falseNextEVs = findPeerEVs(aValidEv, next_[false]);
    HID("(Domino) en=" << evName_(aValidEv) << ", nNextT=" << trueNextEVs.size() << ", nNextF=" << falseNextEVs.size());

    // rm link
    pureRmLink_(aValidEv, prev_[true],  next_[true]);
    pureRmLink_(aValidEv, prev_[false], next_[false]);
    pureRmLink_(aValidEv, next_[true],  prev_[true]);
    pureRmLink_(aValidEv, next_[false], prev_[false]);

    // rm self resrc
    pureSetStateOK_(aValidEv, false);  // must before clean ev_en_
    en_ev_.erase(ev_en_[aValidEv]);
    auto ret = ev_en_.erase(aValidEv);
    HID("[Domino] ev=" << aValidEv << ", ret=" << ret);

    // deduce impacted
    for (auto&& nextEV : trueNextEVs)
        deduceStateFrom_(nextEV);
    for (auto&& nextEV : falseNextEVs)
        deduceStateFrom_(nextEV);

    // call hdlr
    effect_();
}

// ***********************************************************************************************
Domino::Event Domino::setPrev(const EvName& aEvName, const SimuEvents& aSimuPrevEvents)
{
    // validate
    auto fromEv = newEvent(aEvName);  // complex by getEventBy(), not worth
    for (auto&& prevEn_state : aSimuPrevEvents)
    {
        if (isNextFromTo_(fromEv, newEvent(prevEn_state.first)))
        {
            WRN("(Domino) !!!Failed, avoid loop between " << aEvName << " & " << prevEn_state.first);
            return D_EVENT_FAILED_RET;
        }
    }

    // set prev
    pureSetPrev_(fromEv, aSimuPrevEvents);

    // deduce all impacted
    deduceStateFrom_(fromEv);

    // call hdlr
    effect_();
    return fromEv;
}

// ***********************************************************************************************
size_t Domino::setState(const SimuEvents& aSimuEvents)
{
    // validate
    for (auto&& en_state : aSimuEvents)
    {
        const auto ev = getEventBy(en_state.first);  // not create new ev if validation fail
        if (ev == D_EVENT_FAILED_RET)
            continue;
        if (prev_[true].find(ev) != prev_[true].end() || prev_[false].find(ev) != prev_[false].end())
        {
            ERR("(Domino) refuse since en=" << en_state.first << " has prev (avoid break its prev logic)");
            return 0;
        }
    }

    // set state(s)
    set<Event> evs;
    for (auto&& en_state : aSimuEvents)
    {
        const auto ev = newEvent(en_state.first);
        if (pureSetStateOK_(ev, en_state.second))  // real changed
            evs.insert(ev);
    }

    // decude state(s)
    for (auto&& ev : evs)
    {
        for (auto&& nextEV : findPeerEVs(ev, next_[true]))
            deduceStateFrom_(nextEV);
        for (auto&& nextEV : findPeerEVs(ev, next_[false]))
            deduceStateFrom_(nextEV);
    }

    // call hdlr(s)
    effect_();
    return evs.size();
}

// ***********************************************************************************************
Domino::EvName Domino::whyFalse(const Event& aEv) const
{
    auto&& ev_en = ev_en_.find(aEv);
    if (ev_en == ev_en_.end())
    {
        WRN("(Domino) invalid event=" << aEv);
        return EvName(DOM_RESERVED_EVNAME) + " whyFalse() found nothing";
    }
    if (state(aEv) == true)
    {
        WRN("(Domino) en=" << ev_en->second << ", state=true");
        return EvName(DOM_RESERVED_EVNAME) + " whyFalse() found nothing";
    }
    HID("(Domino) en=" << ev_en->second);

    // root T-br
    for (auto&& prevEV : findPeerEVs(aEv, prev_[true]))
    {
        if (states_[prevEV] == true)
            continue;
        return whyFalse(prevEV);
    }

    // root F-br
    for (auto&& prevEV : findPeerEVs(aEv, prev_[false]))
    {
        if (states_[prevEV] == false)
            continue;
        return whyTrue_(prevEV);
    }

    return ev_en->second + "==false";
    // - doesn't make sense that user define EvName like this
    // - but newEvent() doesn't forbid this kind of EvName to ensure safe of other Domino func
    //   that assume newEvent() always succ
}
Domino::EvName Domino::whyTrue_(const Event& aValidEv) const
{
    auto&& truePrevEVs = findPeerEVs(aValidEv, prev_[true]);
    const size_t nTruePrev = truePrevEVs.size();

    auto&& falsePrevEVs = findPeerEVs(aValidEv, prev_[false]);
    const size_t nFalsePrev = falsePrevEVs.size();

    HID("(Domino en=" << evName_(aValidEv) << ", nTruePrev=" << nTruePrev << ", nFalsePrev=" << nFalsePrev);
    if (nTruePrev == 1 && nFalsePrev == 0)
        return whyTrue_(*(truePrevEVs.begin()));
    if (nTruePrev == 0 && nFalsePrev == 1)
        return whyFalse(*(falsePrevEVs.begin()));
    return ev_en_.at(aValidEv) + "==true";  // here is the futhest unique prev
}

}  // namespace
