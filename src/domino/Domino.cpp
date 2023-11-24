/**
 * Copyright 2018 Nokia. All rights reserved.
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <string>

#include "Domino.hpp"

namespace RLib
{
const Domino::EvName Domino::invalidEvName("Invalid Ev/EvName");

// ***********************************************************************************************
void Domino::deduceState(const Event aEv)
{
    auto&& itEvLinks = prev_[true].find(aEv);
    if (itEvLinks != prev_[true].end())  // true br
        for (auto&& tPrev : itEvLinks->second)
            if (states_[tPrev] != true)  // 1 prev not satisfied
                return;
    itEvLinks = prev_[false].find(aEv);
    if (itEvLinks != prev_[false].end())  // false br
        for (auto&& fPrev : itEvLinks->second)
            if (states_[fPrev] != false)  // 1 prev not satisfied
                return;

    pureSetState(aEv, true);
    itEvLinks = next_[true].find(aEv);
    if (itEvLinks == next_[true].end())  // no more next
        return;
    for (auto&& nextEvent : itEvLinks->second)
        deduceState(nextEvent);
}

// ***********************************************************************************************
Domino::Event Domino::getEventBy(const EvName& aEvName) const
{
    auto&& it = events_.find(aEvName);
    return it == events_.end()
        ? D_EVENT_FAILED_RET
        : it->second;
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

    HID("(Domino) Succeed, EvName=" << aEvName << ", event id=" << event);
    events_.emplace(aEvName, event);
    evNames_.push_back(aEvName);
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
            neighborLinks.erase(aEv);      // erase 1
    }
    aMyLinks.erase(itMyLinks);
}

// ***********************************************************************************************
void Domino::pureSetState(const Event aEv, const bool aNewState)
{
    if (states_[aEv] != aNewState)
    {
        states_[aEv] = aNewState;
        DBG("(Domino) Succeed, EvName=" << evNames_[aEv] << " newState=" << aNewState);
        if (aNewState == true)
            effect(aEv);

        sthChanged_ = true;
    }
}

// ***********************************************************************************************
void Domino::innerRmEvOK(const Event aEv)
{
    pureRmLink(aEv, prev_[true],  next_[true]);
    pureRmLink(aEv, prev_[false], next_[false]);
    pureRmLink(aEv, next_[true],  prev_[true]);
    pureRmLink(aEv, next_[false], prev_[false]);

    auto&& en = evNames_[aEv];
    events_.erase(en);
    en = invalidEvName;

    pureSetState(aEv, false);
}

// ***********************************************************************************************
Domino::Event Domino::setPrev(const EvName& aEvName, const SimuEvents& aSimuPrevEvents)
{
    auto&& event = newEvent(aEvName);
    for (auto&& itSim : aSimuPrevEvents)
    {
        if (event == getEventBy(itSim.first))
        {
            WRN("(Domino) !!!Failed, can't set self as previous event (=loop self), EvName=" << aEvName);
            return D_EVENT_FAILED_RET;
        }
    }

    for (auto&& itSim : aSimuPrevEvents)
    {
        auto&& prevEv = newEvent(itSim.first);
        prev_[itSim.second][event].insert(prevEv);
        next_[itSim.second][prevEv].insert(event);
        DBG("(Domino) Succeed, EvName=" << aEvName << ", preEvent=" << itSim.first
            << ", preEventState=" << itSim.second);
    }
    deduceState(event);
    return event;
}

// ***********************************************************************************************
void Domino::setState(const SimuEvents& aSimuEvents)
{
    sthChanged_ = false;

    for (auto&& itSim : aSimuEvents)
        pureSetState(newEvent(itSim.first), itSim.second);
    for (auto&& itSim : aSimuEvents)
    {
        auto&& itEvLinks = next_[itSim.second].find(events_[itSim.first]);
        if (itEvLinks == next_[itSim.second].end())
            break;
        for (auto&& nextEvent : itEvLinks->second)
            deduceState(nextEvent);
    }

    if (!sthChanged_)
        DBG("(Domino) nothing changed for all nEvent=" << aSimuEvents.size());
}

// ***********************************************************************************************
Domino::EvName Domino::whyFalse(const EvName& aEvName) const
{
    auto&& ev = getEventBy(aEvName);

    auto&& itPairTrue = prev_[true].find(ev);
    if (itPairTrue != prev_[true].end())
    {
        for (auto&& candEv : itPairTrue->second)
        {
            if (states_[candEv] == false)
                return evNames_[candEv] + "==false";
        }
    }

    auto&& itPairFalse = prev_[false].find(ev);
    if (itPairFalse != prev_[false].end())
    {
        for (auto&& candEv : itPairFalse->second)
        {
            if (states_[candEv] == true)
                return evNames_[candEv] + "==true";
        }
    }

    return EvName();
}
}  // namespace
