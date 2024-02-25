/**
 * Copyright 2018 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - what: Domino with priority
// - why/VALUE:
//   . separate class to lighten Domino (let PriDomino & Domin to focus it own)
//   * priority call hdlr rather than FIFO in Domino [MUST-HAVE!]
// - core: priorities_
// - mem safe: yes
// ***********************************************************************************************
#ifndef PRI_DOMINO_HPP_
#define PRI_DOMINO_HPP_

#include <unordered_map>
#include "UniLog.hpp"

namespace RLib
{
// ***********************************************************************************************
template<class aDominoType>
class PriDomino : public aDominoType
{
public:
    explicit PriDomino(const SafeString& aUniLogName = ULN_DEFAULT) : aDominoType(aUniLogName) {}

    // -------------------------------------------------------------------------------------------
    // Extend Tile record:
    // - priority: Tile's priority to call hdlr, optional
    // -------------------------------------------------------------------------------------------
    EMsgPriority  getPriority(const Domino::Event) const override;  // key/min change other Dominos
    Domino::Event setPriority(const Domino::EvName&, const EMsgPriority);
protected:
    void innerRmEv(const Domino::Event) override;

private:
    // -------------------------------------------------------------------------------------------
    unordered_map<Domino::Event, EMsgPriority> priorities_;    // [event]=priority
public:
    using aDominoType::oneLog;
};

// ***********************************************************************************************
template<class aDominoType>
EMsgPriority PriDomino<aDominoType>::getPriority(const Domino::Event aEv) const
{
    auto&& it = priorities_.find(aEv);
    if (it == priorities_.end())
        return aDominoType::getPriority(aEv);  // default
    else
        return it->second;
}

// ***********************************************************************************************
template<typename aDominoType>
void PriDomino<aDominoType>::innerRmEv(const Domino::Event aEv)
{
    priorities_.erase(aEv);
    aDominoType::innerRmEv(aEv);
}

// ***********************************************************************************************
template<class aDominoType>
Domino::Event PriDomino<aDominoType>::setPriority(const Domino::EvName& aEvName, const EMsgPriority aPri)
{
    DBG("(PriDom) EvName=" << aEvName << ", newPri=" << size_t(aPri));
    auto&& event = this->newEvent(aEvName);
    if (aPri == aDominoType::getPriority(event))
        priorities_.erase(event);  // less mem & faster searching
    else if (aPri < EMsgPri_MAX)
        priorities_[event] = aPri;
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
// 2022-08-18  CSZ       - replace CppLog by UniLog
// ***********************************************************************************************
