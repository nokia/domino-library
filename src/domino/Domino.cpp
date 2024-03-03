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
void Domino::deduceState(const Event aEv)
{
    HID("(Domino) en=" << evName(aEv));

    auto&& ev_prevEVs = prev_[true].find(aEv);
    if (ev_prevEVs != prev_[true].end())  // true br
        for (auto&& prevEV : ev_prevEVs->second)
            if (states_[prevEV] != true)  // 1 prev not satisfied
                return;
    ev_prevEVs = prev_[false].find(aEv);
    if (ev_prevEVs != prev_[false].end())  // false br
        for (auto&& prevEV : ev_prevEVs->second)
            if (states_[prevEV] != false)  // 1 prev not satisfied
                return;

    pureSetState(aEv, true);
    auto&& ev_nextEVs = next_[true].find(aEv);
    if (ev_nextEVs == next_[true].end())  // no more next
        return;
    for (auto&& nextEV : ev_nextEVs->second)
        deduceState(nextEV);
}

// ***********************************************************************************************
bool Domino::isLinkedFromTo(const Event aFromEv, const Event aToEv) const
{
    if (aFromEv == aToEv)
        return true;
    if (isLinkedFromToVia(aFromEv, aToEv, next_[true]))
        return true;
    return isLinkedFromToVia(aFromEv, aToEv, next_[false]);
}
bool Domino::isLinkedFromToVia(const Event aFromEv, const Event aToEv, const EvLinks& aViaEvLinks) const
{
    auto&& ev_nextEVs = aViaEvLinks.find(aFromEv);
    if (ev_nextEVs != aViaEvLinks.end())
    {
        for (auto&& nextEV : ev_nextEVs->second)
        {
            if (nextEV == aToEv)
                return true;
            else if (isLinkedFromTo(nextEV, aToEv))
                return true;
        }  // for
    }  // if
    return false;
}

// ***********************************************************************************************
Domino::Event Domino::getEventBy(const EvName& aEvName) const
{
    auto&& en_ev = events_.find(aEvName);
    return en_ev == events_.end()
        ? D_EVENT_FAILED_RET
        : en_ev->second;
}

// ***********************************************************************************************
void Domino::innerRmEv(const Event aEv)
{
    // for later deduceState()
    auto&& ev_nextEVs = next_[true].find(aEv);
    auto&& trueNextEVs = (ev_nextEVs == next_[true].end()) ? Events() : ev_nextEVs->second;
    ev_nextEVs = next_[false].find(aEv);
    auto&& falseNextEVs = (ev_nextEVs == next_[false].end()) ? Events() : ev_nextEVs->second;
    HID("(Domino) en=" << evName(aEv) << ", nT=" << trueNextEVs.size() << ", nF=" << falseNextEVs.size());

    pureRmLink(aEv, prev_[true],  next_[true]);
    pureRmLink(aEv, prev_[false], next_[false]);
    pureRmLink(aEv, next_[true],  prev_[true]);
    pureRmLink(aEv, next_[false], prev_[false]);

    pureSetState(aEv, false);  // must before clean evNames_

    events_.erase(evNames_[aEv]);
    auto ret = evNames_.erase(aEv);
    HID("[Domino] aEv=" << aEv << ", ret=" << ret);

    for (auto&& ev : trueNextEVs)
        deduceState(ev);
    for (auto&& ev : falseNextEVs)
        deduceState(ev);
}

// ***********************************************************************************************
Domino::Event Domino::newEvent(const EvName& aEvName)
{
    auto&& event = getEventBy(aEvName);
    if (event != D_EVENT_FAILED_RET)
        return event;

    event = recycleEv();
    if (event == D_EVENT_FAILED_RET)
        event = states_.size();

    HID("(Domino) Succeed, EvName=" << aEvName << ", event=" << event);
    events_.emplace(aEvName, event);
    evNames_[event] = aEvName;
    states_.push_back(false);
    return event;
}

// ***********************************************************************************************
void Domino::pureRmLink(const Event aEv, EvLinks& aMyLinks, EvLinks& aNeighborLinks)
{
    auto itMyLinks = aMyLinks.find(aEv);
    if (itMyLinks == aMyLinks.end())  // not found
        return;

    for (auto&& peerEv : itMyLinks->second)
    {
        auto&& neighborLinks = aNeighborLinks[peerEv];
        if (neighborLinks.size() <= 1)
            aNeighborLinks.erase(peerEv);  // erase entire
        else
            neighborLinks.erase(aEv);  // erase 1
    }
    aMyLinks.erase(itMyLinks);
}

// ***********************************************************************************************
void Domino::pureSetState(const Event aEv, const bool aNewState)
{
    if (states_[aEv] != aNewState)
    {
        states_[aEv] = aNewState;
        HID("(Domino) Succeed, EvName=" << evNames_[aEv] << " newState=" << aNewState);
        if (aNewState == true)
            effect(aEv);

        sthChanged_ = true;
    }
}

// ***********************************************************************************************
Domino::Event Domino::setPrev(const EvName& aEvName, const SimuEvents& aSimuPrevEvents)
{
    auto&& event = newEvent(aEvName);
    for (auto&& prevEn_state : aSimuPrevEvents)
    {
        if (isLinkedFromTo(event, newEvent(prevEn_state.first)))
        {
            WRN("(Domino) !!!Failed, avoid loop between " << aEvName << " & " << prevEn_state.first);
            return D_EVENT_FAILED_RET;
        }
    }

    for (auto&& prevEn_state : aSimuPrevEvents)
    {
        auto&& prevEv = newEvent(prevEn_state.first);
        prev_[prevEn_state.second][event].insert(prevEv);
        next_[prevEn_state.second][prevEv].insert(event);
        DBG("(Domino) Succeed, EvName=" << aEvName << ", prevEn=" << prevEn_state.first
            << ", prerequisiteState=" << prevEn_state.second
            << ", nPrev[true]=" << prev_[true].size() << ", nNext[true]=" << next_[true].size()
            << ", nPrev[false]=" << prev_[false].size() << ", nNext[false]=" << next_[false].size());
    }
    deduceState(event);
    return event;
}

// ***********************************************************************************************
void Domino::setState(const SimuEvents& aSimuEvents)
{
    sthChanged_ = false;

    for (auto&& en_state : aSimuEvents)
        pureSetState(newEvent(en_state.first), en_state.second);
    for (auto&& en_state : aSimuEvents)
    {
        auto&& ev_nextEVs = next_[en_state.second].find(events_[en_state.first]);
        if (ev_nextEVs == next_[en_state.second].end())
            continue;
        HID("(Domino) en=" << en_state.first << ", nNext=" << ev_nextEVs->second.size());
        for (auto&& nextEV : ev_nextEVs->second)
            deduceState(nextEV);
    }

    if (!sthChanged_)
        DBG("(Domino) nothing changed for all nEvent=" << aSimuEvents.size());
}

// ***********************************************************************************************
Domino::EvName Domino::whyFalse(const EvName& aEvName) const
{
    auto&& ev = getEventBy(aEvName);

    auto&& ev_prevEVs = prev_[true].find(ev);
    if (ev_prevEVs != prev_[true].end())
    {
        for (auto&& prevEV : ev_prevEVs->second)
        {
            if (states_[prevEV] == false)
                return evNames_.at(prevEV) + "==false";
        }
    }

    ev_prevEVs = prev_[false].find(ev);
    if (ev_prevEVs != prev_[false].end())
    {
        for (auto&& prevEV : ev_prevEVs->second)
        {
            if (states_[prevEV] == true)
                return evNames_.at(prevEV) + "==true";
        }
    }

    return EvName(DOM_RESERVED_EVNAME) + " whyFalse() found nothing";
    // - doesn't make sense that user define EvName like this
    // - but newEvent() doesn't forbid this kind of EvName to ensure safe of other Domino func
    //   that assume newEvent() always succ
}
}  // namespace
