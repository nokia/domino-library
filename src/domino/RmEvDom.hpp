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
//         . REQ: newEvent() not reuse rm-ed Ev until no new Ev, then call RmDom.reuseEv() (simple & safe in most case)
//         . REQ: RmDom to encapsulate related func
//         . REQ: RmDom prefers to rmEvOK() Ev only (simple & limited)
//         . REQ: RmDom can rm branch Ev by nested rmEvOK()
// ***********************************************************************************************
#pragma once

using namespace std;

namespace RLib
{
// ***********************************************************************************************
template<typename aDominoType>
class RmEvDom : public aDominoType
{
public:
    explicit RmEvDom(const UniLogName& aUniLogName) : aDominoType(aUniLogName) {}

    Domino::Event canRm(const Domino::Event aEv) const override;
    bool rmEvOK(const Domino::Event) override;
    bool isRemoved(const Domino::Event aEv) const { return aEv < isRemovedEv_.size() ? isRemovedEv_[aEv] : false; }

    // -------------------------------------------------------------------------------------------
private:
    vector<bool> isRemovedEv_;  // bitmap & dyn expand, [event]=true(rm-ed) or not

public:
    using aDominoType::oneLog;
};

// ***********************************************************************************************
template<typename aDominoType>
Domino::Event RmEvDom<aDominoType>::canRm(const Domino::Event aEv) const
{
    if (isRemoved(aEv))
        return Domino::D_EVENT_FAILED_RET;

    return aDominoType::canRm(aEv);
}

// ***********************************************************************************************
template<typename aDominoType>
bool RmEvDom<aDominoType>::rmEvOK(const Domino::Event aEv)
{
    if (isRemoved(aEv))  // already removed
        return false;

    if (! aDominoType::rmEvOK(aEv))  // fail
        return false;

    // rmEvOK() succ
    if (aEv >= isRemovedEv_.size())
        isRemovedEv_.resize(aEv + 1);
    isRemovedEv_[aEv] = true;
    return true;
}

}  // namespace
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2023-11-14  CSZ       1)create
// ***********************************************************************************************
