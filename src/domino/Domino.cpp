/**
 * Copyright 2018 Nokia. All rights reserved.
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <algorithm>
#include <stack>
#include <string>
#include <unordered_set>

#include "Domino.hpp"

using namespace std;

namespace rlib
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
void Domino::effect_() noexcept
{
    for (auto&& ev : effectEVs_)
        if (state(ev) == true)  // avoid multi-change
            effect_(ev);
    effectEVs_.clear();
}

// ***********************************************************************************************
const Domino::Events& Domino::findPeerEVs(const Event& aEv, const EvLinks& aLinks) noexcept
{
    auto&& ev_peerEVs = aLinks.find(aEv);
    return ev_peerEVs == aLinks.end()
        ? defaultEVs
        : ev_peerEVs->second;
}

// ***********************************************************************************************
bool Domino::isNextFromTo_(const Event& aFromValidEv, const Event& aToValidEv) const noexcept
{
    unordered_set<Event> evVisited{aFromValidEv};  // rare to search huge events
    stack<Event> evStack;  // recursive func may stack overflow

    for (auto curEv = aFromValidEv; curEv != aToValidEv; curEv = evStack.top(), evStack.pop()) {
        for (bool branch : {true, false}) {  // search next_[true] & next_[false]
            for (auto&& nextEV : findPeerEVs(curEv, next_[branch])) {
                if (evVisited.insert(nextEV).second) {  // insert OK
                    evStack.push(nextEV);
                }
            }
        }
        if (evStack.empty())
            return false;
    }
    return true;
}

// ***********************************************************************************************
Domino::Event Domino::getEventBy(const EvName& aEvName) const noexcept
{
    auto&& en_ev = en_ev_.find(aEvName);
    return en_ev == en_ev_.end()
        ? D_EVENT_FAILED_RET
        : en_ev->second;
}

// ***********************************************************************************************
Domino::Event Domino::newEvent(const EvName& aEvName) noexcept
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
void Domino::pureSetPrev_(const Event& aValidEv, const SimuEvents& aSimuPrevEvents) noexcept
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
bool Domino::pureSetStateOK_(const Event& aValidEv, const bool aNewState) noexcept
{
    if (states_[aValidEv] != aNewState)  // do need change
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
            WRN("(Domino) !!!Failed since loop between " << aEvName << " & " << prevEn_state.first);
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
            continue;  // new ev, need to create in next step
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
Domino::EvName Domino::whyFalse(const Event& aEv) const noexcept
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

    Events::iterator it;
    // search true prev
    for (auto curEV = aEv;; curEV = *it) {
        auto&& prevEVs = findPeerEVs(curEV, prev_[true]);
        it = find_if(prevEVs.begin(), prevEVs.end(), [this](auto&& aPrevEV){ return states_[aPrevEV] == false; });
        if (it == prevEVs.end()) {  // no further true-prev to check
            if (curEV == aEv)
                break;  // try false-prev
            HID("(Domino) found unsatisfied en=" << evName_(curEV) << " from true prevEVs=" << prevEVs.size());
            return evName_(curEV) + "==false";  // found
        }
    }
    // nothing in true prev, search false prev
    auto&& prevEVs = findPeerEVs(aEv, prev_[false]);
    it = find_if(prevEVs.begin(), prevEVs.end(), [this](auto&& aPrevEV){ return states_[aPrevEV] == true; });
    if (it == prevEVs.end()) {  // no further false-prev to check
        HID("(Domino) found unsatisfied en=" << ev_en->second << " from false prevEVs=" << prevEVs.size());
        return ev_en->second + "==false";
    }
    return whyTrue_(*it);
    // - doesn't make sense that user define EvName like this
    // - but newEvent() doesn't forbid this kind of EvName to ensure safe of other Domino func
    //   that assume newEvent() always succ
}
Domino::EvName Domino::whyTrue_(const Event& aValidEv) const noexcept
{
    for (auto curEV = aValidEv; ; ) {
        auto&& truePrevEVs = findPeerEVs(curEV, prev_[true]);
        const size_t nTruePrev = truePrevEVs.size();

        auto&& falsePrevEVs = findPeerEVs(curEV, prev_[false]);
        const size_t nFalsePrev = falsePrevEVs.size();

        HID("(Domino en=" << evName_(curEV) << ", nTruePrev=" << nTruePrev << ", nFalsePrev=" << nFalsePrev);
        if (nTruePrev == 1 && nFalsePrev == 0) {
            curEV = *(truePrevEVs.begin());
            continue;
        }
        if (nTruePrev == 0 && nFalsePrev == 1)
            return whyFalse(*(falsePrevEVs.begin()));
        HID("(Domino) found true en=" << ev_en_.at(curEV));
        return ev_en_.at(curEV) + "==true";  // here is the futhest unique prev
    }
}

}  // namespace
