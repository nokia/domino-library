/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - Usage:
//   . get most matchable domino by EvName
//   . store different dominos in 1 (EvName) tree, each domino covers 1 branch (no overlap)
// - CONSTITUTION / VALUE:
//   * diff processes may use SyncDom to work together, hide impl details (eg SyncDom) to user
//   . most matchable algorithm
//   . EvName as matching key, no extra new info
// - note:
//   . DomDoor is not a domino (so need not domino APIs), but a door
// ***********************************************************************************************
#pragma once

#include <map>

#include "Domino.hpp"
#include "UniLog.hpp"

using namespace std;

namespace RLib
{
// ***********************************************************************************************
class DomDoor
{
public:
    void subTree(const Domino::EvName& aSubRoot, shared_ptr<void> aDom) { domStore_[aSubRoot] = aDom; }

    template<typename aDominoType>
    shared_ptr<aDominoType> subTree(const Domino::EvName& aEvName) const;

private:
    // -------------------------------------------------------------------------------------------
    std::map<Domino::EvName, std::shared_ptr<void> > domStore_;  // [EvName]=shared_ptr<aDominoType>
};

// ***********************************************************************************************
template<typename aDominoType>
shared_ptr<aDominoType> DomDoor::subTree(const Domino::EvName& aEvName) const
{
    auto maxNameMatched = 0;
    shared_ptr<void> domMatched;
    for (auto&& it : domStore_)
    {
        const auto subRootLen = it.first.length();
        if (subRootLen <= maxNameMatched) continue;
        if (aEvName.substr(0, subRootLen) != it.first) continue;
        maxNameMatched = subRootLen;
        domMatched = it.second;
    }
    return static_pointer_cast<aDominoType>(domMatched);
}
}  // namespace
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2022-09-28  CSZ       1)create
// ***********************************************************************************************
