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
// - core: ev_pri_S_
// - class safe: yes
//   . forbid change flag when hdlr available, avoid confusing scenario eg  hdlr on-road
// ***********************************************************************************************
#pragma once

#include <unordered_map>
#include "UniLog.hpp"

namespace RLib
{
// ***********************************************************************************************
template<class aDominoType>
class PriDomino : public aDominoType
{
public:
    explicit PriDomino(const LogName& aUniLogName = ULN_DEFAULT) : aDominoType(aUniLogName) {}

    // -------------------------------------------------------------------------------------------
    // Extend Tile record:
    // - priority: Tile's priority to call hdlr, optional
    // -------------------------------------------------------------------------------------------
    EMsgPriority  getPriority(const Domino::Event&) const override;  // key/min change other Dominos
    Domino::Event setPriority(const Domino::EvName&, const EMsgPriority);
protected:
    void rmEv_(const Domino::Event& aValidEv) override;

private:
    // -------------------------------------------------------------------------------------------
    std::unordered_map<Domino::Event, EMsgPriority> ev_pri_S_;  // [event]=priority
public:
    using aDominoType::oneLog;
};

// ***********************************************************************************************
template<class aDominoType>
EMsgPriority PriDomino<aDominoType>::getPriority(const Domino::Event& aEv) const
{
    auto&& ev_pri = ev_pri_S_.find(aEv);
    if (ev_pri == ev_pri_S_.end())
        return aDominoType::getPriority(aEv);  // default
    else
        return ev_pri->second;
}

// ***********************************************************************************************
template<typename aDominoType>
void PriDomino<aDominoType>::rmEv_(const Domino::Event& aValidEv)
{
    ev_pri_S_.erase(aValidEv);
    aDominoType::rmEv_(aValidEv);
}

// ***********************************************************************************************
template<class aDominoType>
Domino::Event PriDomino<aDominoType>::setPriority(const Domino::EvName& aEvName, const EMsgPriority aPri)
{
    // validate
    if (this->nHdlr(aEvName) > 0)
    {
        ERR("(PriDom) FAILED since exist hdlr(s) in en=" << aEvName << ", avoid complex/mislead result");
        return Domino::D_EVENT_FAILED_RET;
    }

    HID("(PriDom) EvName=" << aEvName << ", newPri=" << size_t(aPri));
    auto&& event = this->newEvent(aEvName);
    if (aPri == aDominoType::getPriority(event))
        ev_pri_S_.erase(event);  // less mem & faster searching
    else if (aPri < EMsgPri_MAX)
        ev_pri_S_[event] = aPri;
    return event;
}

}  // namespace
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
