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
//   . newEvent() may alloc fail (throw in diff dom), so has to reuse rm-ed Ev as early as possible
//   . standalone class to min impact other dom
//   . how about orphan ev after rm its parent?
//     . let user take care his rm rather than complex dom
//
// - RISK:
//   . rm ev may break links (user shall take care)
//   . rm ev impact/complex domino
//   . so use RmEvDom when necessary
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
    explicit RmEvDom(const UniLogName& aUniLogName) : aDominoType(aUniLogName) {}

    bool rmEvOK(const Domino::EvName aEN) { return innerRmEvOK(this->getEventBy(aEN)); }
    bool isRemoved(const Domino::Event aEv) const { return isRemovedEv_.count(aEv); }
protected:
    bool innerRmEvOK(const Domino::Event) override;
    Domino::Event recycleEv() override;

private:
    // -------------------------------------------------------------------------------------------
    // - REQ: min mem (so set is better than vector<bool> when almost empty
    // - REQ: fast (eg isRemoved(), insert, del)
    // - req: better FIFO
    unordered_set<Domino::Event> isRemovedEv_;

public:
    using aDominoType::oneLog;
};

// ***********************************************************************************************
template<typename aDominoType>
Domino::Event RmEvDom<aDominoType>::recycleEv()
{
    if (isRemovedEv_.empty())
        return Domino::D_EVENT_FAILED_RET;

    const auto ev = *(isRemovedEv_.begin());
    isRemovedEv_.erase(isRemovedEv_.begin());
    return ev;
}

// ***********************************************************************************************
template<typename aDominoType>
bool RmEvDom<aDominoType>::innerRmEvOK(const Domino::Event aEv)
{
    if (isRemoved(aEv))  // already removed
        return false;

    if (! aDominoType::innerRmEvOK(aEv))  // fail eg invalid aEv
        return false;

    // innerRmEvOK() succ; must NOT rely on aDominoType at all since info of aEv is removed
    isRemovedEv_.insert(aEv);
    return true;
}

}  // namespace
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2023-11-14  CSZ       1)create
// 2023-11-22  CSZ       - better isRemovedEv_
// 2023-11-24  CSZ       - rmEvOK->innerRmEvOK since ev para (EN can outer use)
// ***********************************************************************************************
