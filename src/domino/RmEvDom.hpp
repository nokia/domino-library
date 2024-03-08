/**
 * Copyright 2023 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - why/VALUE:
//   * rm Ev and all its resource, so Dom can run very long time
//   * REQ:
//     * as simple as possible
//     * de-couple as much as possible
//
// - how:
//   . each dom can rm its own resource independently as public interface (de-couple)
//   . newEvent() has to reuse rm-ed Ev as early as possible - to avoid alloc fail
//   . standalone class to min impact other dom
//   . how about orphan ev after rm its parent?
//     . let user take care his rm rather than complex dom
//
// - RISK:
//   . rm ev may break links (then user shall take care)
//   . rm ev impact/complex domino
//   . so use RmEvDom when necessary
//
// - mem safe: yes with limit
//   . no use-up mem (too many removed events; impossible in most cases)
// ***********************************************************************************************
#pragma once

#include <unordered_set>

using namespace std;

namespace RLib
{
// ***********************************************************************************************
template<typename aDominoType>
class RmEvDom : public aDominoType
{
public:
    explicit RmEvDom(const LogName& aUniLogName = ULN_DEFAULT) : aDominoType(aUniLogName) {}

    bool rmEvOK(const Domino::EvName& aEN);
    bool isRemoved(const Domino::Event& aEv) const { return isRemovedEv_.count(aEv); }
protected:
    void rmEv_(const Domino::Event& aValidEv) override;
    Domino::Event recycleEv_() override;

private:
    // -------------------------------------------------------------------------------------------
    // - REQ: min mem (so set<Event> is better than vector<bool> when almost empty
    // - REQ: fast (eg isRemoved(), insert, del)
    // - req: better FIFO
    unordered_set<Domino::Event> isRemovedEv_;

public:
    using aDominoType::oneLog;
};

// ***********************************************************************************************
template<typename aDominoType>
Domino::Event RmEvDom<aDominoType>::recycleEv_()
{
    if (isRemovedEv_.empty())
        return Domino::D_EVENT_FAILED_RET;

    const auto ev = *(isRemovedEv_.begin());
    isRemovedEv_.erase(ev);
    return ev;
}

// ***********************************************************************************************
template<typename aDominoType>
void RmEvDom<aDominoType>::rmEv_(const Domino::Event& aValidEv)
{
    aDominoType::rmEv_(aValidEv);
    isRemovedEv_.insert(aValidEv);
}

// ***********************************************************************************************
template<typename aDominoType>
bool RmEvDom<aDominoType>::rmEvOK(const Domino::EvName& aEN)
{
    const auto validEv = this->getEventBy(aEN);
    if (validEv == Domino::D_EVENT_FAILED_RET)  // invalid; most beginning check, avoid useless exe
        return false;

    rmEv_(validEv);
    return true;
}

}  // namespace
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2023-11-14  CSZ       1)create
// 2023-11-22  CSZ       - better isRemovedEv_
// 2023-11-24  CSZ       - rmEvOK->rmEv_ since ev para (EN can outer use)
// ***********************************************************************************************
