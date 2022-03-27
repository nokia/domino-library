/**
 * Copyright 2018-2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - what: Domino with priority
// - why/VALUE:
//   . separate class to lighten Domino (let PriDomino & Domin to focus it own)
//   * priority call hdlr rather than FIFO in Domino [MUST-HAVE!]
// - core: priorities_
// ***********************************************************************************************
#ifndef PRI_DOMINO_HPP_
#define PRI_DOMINO_HPP_

#include <unordered_map>

namespace RLib
{
// ***********************************************************************************************
template<class aDominoType>
class PriDomino : public aDominoType
{
public:
    // -------------------------------------------------------------------------------------------
    // Extend Tile record:
    // - priority: Tile's priority to call hdlr, optional
    // -------------------------------------------------------------------------------------------
    EMsgPriority  getPriority(const Domino::Event) const override;  // key/min change other Dominos
    Domino::Event setPriority(const Domino::EvName&, const EMsgPriority);

private:
    // -------------------------------------------------------------------------------------------
    std::unordered_map<Domino::Event, EMsgPriority> priorities_;    // [event]=priority
public:
    using aDominoType::log_;
};

// ***********************************************************************************************
template<class aDominoType>
EMsgPriority PriDomino<aDominoType>::getPriority(const Domino::Event aEv) const
{
    auto&& it = priorities_.find(aEv);
    return it == priorities_.end() ? EMsgPri_NORM : it->second;
}

// ***********************************************************************************************
template<class aDominoType>
Domino::Event PriDomino<aDominoType>::setPriority(const Domino::EvName& aEvName, const EMsgPriority aPri)
{
    DBG("(PriDomino) EvName=" << aEvName << ", newPri=" << aPri);
    auto&& event = this->newEvent(aEvName);
    if (aPri == EMsgPri_NORM) priorities_.erase(event);  // less mem & faster searching
    else priorities_[event] = aPri;
    return event;
}
}  // namespace
#endif  // PRI_DOMINO_HPP_
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   ......................................................................
// 2018-04-30  CSZ       1)create
// 2021-04-01  CSZ       - coding req
// 2022-01-04  PJ & CSZ  - formal log & naming
// 2022-03-26  CSZ       - ut's PARA_DOM include self class & ALL its base class(es)
// 2022-03-27  CSZ       - if ut case can test base class, never specify derive
// ***********************************************************************************************
