/**
 * Copyright 2018 Nokia. All rights reserved.
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <algorithm>
#include <string>

#include "Domino.hpp"

using namespace std;

namespace rlib
{
static const Domino::Events defaultEVs;  // internal use only

// ***********************************************************************************************
void Domino::deduceStateFrom_(const Event& aValidEv) noexcept
{
    stack<Event> evStack;
    for (auto curEV = aValidEv; ; curEV = move(evStack.top()), evStack.pop()) {
        HID("(Domino) en=" << evName_(curEV));

        // recalc self state
        auto newState = deduceStateSelf_(curEV, true) && deduceStateSelf_(curEV, false);
        if (pureSetStateOK_(curEV, newState))  // state real changed
        {
            // deduce next
            for (bool branch : {true, false}) {  // search next_[true] & next_[false]
                for (auto&& nextEV : findPeerEVs(curEV, next_[branch])) {
                    evStack.push(move(nextEV));
                }
            }
        }

        if (evStack.empty()) {  // no more to deduce
            return;
        }
    }
}

// ***********************************************************************************************
bool Domino::deduceStateSelf_(const Event& aValidEv, bool aPrevType) const noexcept
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
    Events evVisited{aFromValidEv};  // rare to search huge events
    stack<Event> evStack;  // recursive func may stack overflow

    for (auto curEv = aFromValidEv; curEv != aToValidEv; curEv = evStack.top(), evStack.pop())
    {
        for (bool branch : {true, false})  // search next_[true] & next_[false]
        {
            for (auto&& nextEV : findPeerEVs(curEv, next_[branch]))
            {
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

    // deduce impacted; safer to dup-deduce same ev
    for (auto&& nextEV : trueNextEVs)
        deduceStateFrom_(nextEV);
    for (auto&& nextEV : falseNextEVs)
        deduceStateFrom_(nextEV);

    // call hdlr
    effect_();
}

// ***********************************************************************************************
Domino::Event Domino::setPrev(const EvName& aEvName, const SimuEvents& aSimuPrevEvents) noexcept
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

    // set ALL state(s) before deduce
    Events simuEVs;
    for (auto&& en_state : aSimuEvents)
    {
        const auto ev = newEvent(en_state.first);
        if (pureSetStateOK_(ev, en_state.second)) {  // real changed
            simuEVs.insert(ev);  // insert unique
        }
    }

    // decude next(s)
    for (auto&& curEV : simuEVs) {
        for (bool branch : {true, false}) {
            for (auto&& nextEV : findPeerEVs(curEV, next_[branch])) {
                deduceStateFrom_(nextEV);  // dup-deduce is safer (like real domino)
            }
        }
    }

    // safer to call hdlr(s) after deduce
    effect_();
    return simuEVs.size();  // real changed
}

// ***********************************************************************************************
Domino::EvName Domino::whyFalse(const Event& aEv) const noexcept
{
    // validate to safe public interface
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

    // loop search
    WhyStep step{aEv, false, EvName()};
    while (step.resultEN_.empty()) {
        step.whyFlag_ ? whyTrue_ (step) : whyFalse_(step);
    }
    return step.resultEN_;
}
void Domino::whyFalse_(WhyStep& aStep) const noexcept
{
    Events::const_iterator it;
    // search true prev
    for (auto curEV = aStep.curEV_;; curEV = *it) {
        auto&& prevEVs = findPeerEVs(curEV, prev_[true]);
        it = find_if(prevEVs.begin(), prevEVs.end(), [this](auto&& aPrevEV){ return states_[aPrevEV] == false; });
        if (it == prevEVs.end()) {  // nothing in true-prev
            if (curEV == aStep.curEV_) {
                break;  // try false-prev
            }

            aStep.resultEN_ = evName_(curEV) + "==false";  // found
            HID("(Domino) found false en=" << evName_(curEV) << " from true prevEVs=" << prevEVs.size());
            return;
        }
    }

    // search false prev
    auto&& prevEVs = findPeerEVs(aStep.curEV_, prev_[false]);
    it = find_if(prevEVs.begin(), prevEVs.end(), [this](auto&& aPrevEV){ return states_[aPrevEV] == true; });
    if (it == prevEVs.end()) {  // nothing in false-prev
        HID("(Domino) found true en=" << evName_(aStep.curEV_) << " from false prevEVs=" << prevEVs.size());
        aStep.resultEN_ = evName_(aStep.curEV_) + "==false";
        return;
    }
    // found true-ev in false-prev, next whyTrue_()
    aStep.curEV_ = *it;
    aStep.whyFlag_ = true;
}
void Domino::whyTrue_(WhyStep& aStep) const noexcept
{
    for (;;) {
        auto&&  truePrevEVs = findPeerEVs(aStep.curEV_, prev_[true]);
        auto&& falsePrevEVs = findPeerEVs(aStep.curEV_, prev_[false]);

        HID("(Domino en=" << evName_(aStep.curEV_) << ", nTruePrev=" << truePrevEVs.size()
            << ", nFalsePrev=" << falsePrevEVs.size());
        if (truePrevEVs.size() == 1 && falsePrevEVs.empty()) {
            aStep.curEV_ = *(truePrevEVs.begin());
            continue;
        }
        if (truePrevEVs.empty() && falsePrevEVs.size() == 1) {
            aStep.curEV_ = *(falsePrevEVs.begin());
            aStep.whyFlag_ = false;
            return;  // next whyFalse_()
        }
        // found true-ev with 0-prev/multi-prev, stop here for single root cause
        HID("(Domino) found true en=" << evName_(aStep.curEV_));
        aStep.resultEN_ = evName_(aStep.curEV_) + "==true";
        return;
    }
}

}  // namespace
