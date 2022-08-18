/**
 * Copyright 2018 Nokia. All rights reserved.
 */
#include <string>

#include "Domino.hpp"

namespace RLib
{
const Domino::EvName Domino::invalidEvName("Invalid Ev/EvName");

// ***********************************************************************************************
void Domino::deduceState(const Event aEv)
{
    for (auto&& prevEvent : prev_[true][aEv])
    {
        if (states_.at(prevEvent) != true) return;
    }
    for (auto&& prevEvent : prev_[false][aEv])
    {
        if (states_.at(prevEvent) != false) return;
    }

    pureSetState(aEv, true);
    for (auto&& nextEvent : next_[true][aEv]) deduceState(nextEvent);
}

// ***********************************************************************************************
Domino::Event Domino::getEventBy(const EvName& aEvName) const
{
    auto&& it = events_.find(aEvName);
    if (it == events_.end()) return D_EVENT_FAILED_RET;
    return it->second;
}

// ***********************************************************************************************
Domino::Event Domino::newEvent(const EvName& aEvName)
{
    auto&& event = getEventBy(aEvName);
    if (event != D_EVENT_FAILED_RET) return event;

    event = states_.size();
    HID("Succeed, EvName=" << aEvName << ", event id=" << event);
    events_.emplace(aEvName, event);
    evNames_.push_back(aEvName);
    states_.push_back(false);
    return event;
}

// ***********************************************************************************************
void Domino::pureSetState(const Event aEv, const bool aNewState)
{
    if (states_[aEv] != aNewState)
    {
        states_[aEv] = aNewState;
        DBG("Succeed, EvName=" << evNames_[aEv] << " newState=" << aNewState);
        if (aNewState == true) effect(aEv);

        sthChanged_ = true;
    }
}

// ***********************************************************************************************
Domino::Event Domino::setPrev(const EvName& aEvName, const SimuEvents& aSimuPrevEvents)
{
    auto&& event = newEvent(aEvName);
    for (auto&& itSim : aSimuPrevEvents)
    {
        if (event == getEventBy(itSim.first))
        {
            WRN("!!!Failed, can't set self as previous event (=loop self), EvName=" << aEvName);
            return D_EVENT_FAILED_RET;
        }
    }

    for (auto&& itSim : aSimuPrevEvents)
    {
        auto&& prevEv = newEvent(itSim.first);
        prev_[itSim.second][event].insert(prevEv);
        next_[itSim.second][prevEv].insert(event);
        DBG("Succeed, EvName=" << aEvName << ", preEvent=" << itSim.first << ", preEventState=" << itSim.second);
    }
    deduceState(event);
    return event;
}

// ***********************************************************************************************
void Domino::setState(const SimuEvents& aSimuEvents)
{
    sthChanged_ = false;

    for (auto&& itSim : aSimuEvents) pureSetState(newEvent(itSim.first), itSim.second);
    for (auto&& itSim : aSimuEvents)
    {
        for (auto&& nextEvent : next_[itSim.second][events_[itSim.first]]) deduceState(nextEvent);
    }

    if (!sthChanged_) DBG("nothing changed for all nEvent=" << aSimuEvents.size());
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
            if (states_[candEv] == false) return evNames_[candEv] + "==false";
        }
    }

    auto&& itPairFalse = prev_[false].find(ev);
    if (itPairFalse != prev_[false].end())
    {
        for (auto&& candEv : itPairFalse->second)
        {
            if (states_[candEv] == true) return evNames_[candEv] + "==true";
        }
    }

    return EvName();
}
}  // namespace
