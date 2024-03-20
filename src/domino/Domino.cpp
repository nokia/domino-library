/**
 * Copyright 2018 Nokia. All rights reserved.
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <string>

#include "Domino.hpp"

namespace RLib
{
// ***********************************************************************************************
void Domino::deduceState_(const Event& aValidEv)
{
    HID("(Domino) en=" << evName_(aValidEv));

    // recalc self state
    auto newState = deduceState_(aValidEv, true)
        ? deduceState_(aValidEv, false)
        : false;
    if (pureSetStateOK_(aValidEv, newState))
    {
        // deduce impacted (true branch)
        auto&& ev_nextEVs = next_[true].find(aValidEv);
        if (ev_nextEVs != next_[true].end())
            for (auto&& nextEV : ev_nextEVs->second)
                deduceState_(nextEV);

        // deduce impacted (false branch)
        ev_nextEVs = next_[false].find(aValidEv);
        if (ev_nextEVs != next_[false].end())
            for (auto&& nextEV : ev_nextEVs->second)
                deduceState_(nextEV);
    }
}

// ***********************************************************************************************
bool Domino::deduceState_(const Event& aValidEv, bool aPrevType) const
{
    auto&& ev_prevEVs = prev_[aPrevType].find(aValidEv);
    if (ev_prevEVs != prev_[aPrevType].end())
        for (auto&& prevEV : ev_prevEVs->second)
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
bool Domino::isNextFromTo_(const Event& aFromValidEv, const Event& aToValidEv) const
{
    if (aFromValidEv == aToValidEv)
        return true;
    if (isNextFromToVia_(aFromValidEv, aToValidEv, next_[true]))
        return true;
    return isNextFromToVia_(aFromValidEv, aToValidEv, next_[false]);
}
bool Domino::isNextFromToVia_(const Event& aFromValidEv, const Event& aToValidEv, const EvLinks& aViaEvLinks) const
{
    auto&& ev_nextEVs = aViaEvLinks.find(aFromValidEv);
    if (ev_nextEVs != aViaEvLinks.end())
    {
        for (auto&& nextEV : ev_nextEVs->second)
        {
            if (nextEV == aToValidEv)
                return true;
            else if (isNextFromTo_(nextEV, aToValidEv))
                return true;
        }  // for
    }  // if
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
    auto&& newEv = getEventBy(aEvName);
    if (newEv != D_EVENT_FAILED_RET)
        return newEv;

    newEv = recycleEv_();
    if (newEv == D_EVENT_FAILED_RET)
        newEv = states_.size();

    HID("(Domino) Succeed, EvName=" << aEvName << ", new event=" << newEv);
    en_ev_.emplace(aEvName, newEv);
    ev_en_[newEv] = aEvName;
    states_.push_back(false);
    return newEv;
}

// ***********************************************************************************************
void Domino::pureRmLink_(const Event& aValidEv, EvLinks& aMyLinks, EvLinks& aNeighborLinks)
{
    auto myEv_peerEVs = aMyLinks.find(aValidEv);
    if (myEv_peerEVs == aMyLinks.end())  // not found
        return;

    for (auto&& peerEv : myEv_peerEVs->second)
    {
        auto&& neighborLinks = aNeighborLinks[peerEv];
        if (neighborLinks.size() <= 1)
            aNeighborLinks.erase(peerEv);  // erase entire
        else
            neighborLinks.erase(aValidEv);  // erase 1
    }
    aMyLinks.erase(myEv_peerEVs);
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
    if (states_[aValidEv] != aNewState)
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
    // for later deduceState_()
    auto&& ev_nextEVs = next_[true].find(aValidEv);
    auto&& trueNextEVs = (ev_nextEVs == next_[true].end()) ? Events() : ev_nextEVs->second;
    ev_nextEVs = next_[false].find(aValidEv);
    auto&& falseNextEVs = (ev_nextEVs == next_[false].end()) ? Events() : ev_nextEVs->second;
    HID("(Domino) en=" << evName_(aValidEv) << ", nT=" << trueNextEVs.size() << ", nF=" << falseNextEVs.size());

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
    for (auto&& ev : trueNextEVs)
        deduceState_(ev);
    for (auto&& ev : falseNextEVs)
        deduceState_(ev);

    // call hdlr
    effect_();
}

// ***********************************************************************************************
Domino::Event Domino::setPrev(const EvName& aEvName, const SimuEvents& aSimuPrevEvents)
{
    // validate
    auto&& newEv = newEvent(aEvName);
    for (auto&& prevEn_state : aSimuPrevEvents)
    {
        if (isNextFromTo_(newEv, newEvent(prevEn_state.first)))
        {
            WRN("(Domino) !!!Failed, avoid loop between " << aEvName << " & " << prevEn_state.first);
            return D_EVENT_FAILED_RET;
        }
    }

    // set prev
    pureSetPrev_(newEv, aSimuPrevEvents);

    // deduce all impacted
    deduceState_(newEv);

    // call hdlr
    effect_();
    return newEv;
}

// ***********************************************************************************************
bool Domino::setStateOK(const SimuEvents& aSimuEvents)
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
            return false;
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
        auto&& ev_nextEVs = next_[true].find(ev);
        if (ev_nextEVs != next_[true].end())
            for (auto&& nextEV : ev_nextEVs->second)
                deduceState_(nextEV);
        ev_nextEVs = next_[false].find(ev);
        if (ev_nextEVs != next_[false].end())
            for (auto&& nextEV : ev_nextEVs->second)
                deduceState_(nextEV);
    }

    // call hdlr(s)
    effect_();
    return true;
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

    auto&& ev_prevEVs = prev_[true].find(aEv);
    if (ev_prevEVs != prev_[true].end())
    {
        HID("(Domino) nTruePrev=" << ev_prevEVs->second.size());
        for (auto&& prevEV : ev_prevEVs->second)
        {
            if (states_[prevEV] == true)
                continue;
            return whyFalse(prevEV);
        }
    }

    ev_prevEVs = prev_[false].find(aEv);
    if (ev_prevEVs != prev_[false].end())
    {
        HID("(Domino) nFalsePrev=" << ev_prevEVs->second.size());
        for (auto&& prevEV : ev_prevEVs->second)
        {
            if (states_[prevEV] == false)
                continue;
            return whyTrue_(prevEV);
        }
    }

    return ev_en->second + "==false";
    // - doesn't make sense that user define EvName like this
    // - but newEvent() doesn't forbid this kind of EvName to ensure safe of other Domino func
    //   that assume newEvent() always succ
}
Domino::EvName Domino::whyTrue_(const Event& aValidEv) const
{
    auto&& ev_truePrevEVs = prev_[true].find(aValidEv);
    const size_t nTruePrev = ev_truePrevEVs == prev_[true].end() ? 0 : ev_truePrevEVs->second.size();

    auto&& ev_falsePrevEVs = prev_[false].find(aValidEv);
    const size_t nFalsePrev = ev_falsePrevEVs == prev_[false].end() ? 0 : ev_falsePrevEVs->second.size();

    HID("(Domino en=" << evName_(aValidEv) << ", nTruePrev=" << nTruePrev << ", nFalsePrev=" << nFalsePrev);
    if (nTruePrev == 1 && nFalsePrev == 0)
        return whyTrue_(*(ev_truePrevEVs->second.begin()));
    if (nTruePrev == 0 && nFalsePrev == 1)
        return whyFalse(*(ev_falsePrevEVs->second.begin()));
    return ev_en_.at(aValidEv) + "==true";  // futhest unique prev
}

}  // namespace
