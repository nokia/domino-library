/**
 * Copyright 2017 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - ISSUE:
//   . like eNB upgrade, many clues, many tasks, how to align the mass to best performance & tidy?
//
// - how:
//   . treat each clue as an event, like a domino tile
//   . treat each task as a hdlr (callback func)
//   . each hdlr relies on limited event(s), each event may also rely on some ohter event(s)
//   . whenever an event occurred, following event(s) can be auto-triggered, so on calling hdlr(s)
//   . this will go till end (like domino)
// - clarify:
//   . each event-hdlr is called only when event state F->T
//   . 1-go domino is like SW upgrade - 1-go then discard, next will use new domino
//     . n-go domino is like IM - repeat working till reboot
//     . after 024-03-19 domino supports n-go
// - Q&A: (below; lots)
//
// - core: states_
//
// - VALUE:
//   * auto broadcast, auto callback, auto shape [MUST-HAVE!]
//   . share state & hdlr
//   . EvName index
//   . template extension (PriDomino, etc)
//   . n-go domino
//
// - MT safe: no
// - use-safe: yes with conditions:
//   . no too many events/.. that use-up mem (impossible in most cases)
//   . user shall not loop ev-link (impossible unless deliberate)
//     . domino prevent is not 100%, see UT for details
// ***********************************************************************************************
#pragma once

#include <map>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "UniLog.hpp"

namespace rlib
{
const char DOM_RESERVED_EVNAME[] = "[Dom Reserved EvName]";

// ***********************************************************************************************
class Domino : public UniLog
{
public:
    using Event      = size_t;  // smaller size can save mem; larger size can support more events
    using Events     = std::unordered_set<Event>;  // better performance than set in most cases
    using EvName     = std::string;
    using SimuEvents = std::map<EvName, bool>;  // not unordered-map since most traversal
    using EvNames    = std::unordered_map<Event, EvName>;  // map is less mem than vector<EvName>
    using EvLinks    = std::map<Event, Events>;

    enum : Event
    {
        N_EVENT_STATE      = 2,
        D_EVENT_FAILED_RET = static_cast<Event>(-1),
    };

    // -------------------------------------------------------------------------------------------
    // Each Tile in Domino is a record, containing:
    // - Event:  tile's internal ID  , mandatory
    // - EvName: tile's external ID  , mandatory
    // - state:  tile's up/down state, mandatory, default=false
    // - prev:   prev tile(s)        , optional
    // -------------------------------------------------------------------------------------------
    explicit Domino(const LogName& aUniLogName = ULN_DEFAULT) noexcept : UniLog(aUniLogName) {}
    virtual ~Domino() noexcept = default;

    Event newEvent(const EvName&) noexcept;
    Event getEventBy(const EvName&) const noexcept;
    const EvNames evNames() const noexcept { return ev_en_; }

    bool   state(const EvName& aEvName) const noexcept { return state(getEventBy(aEvName)); }
    bool   state(const Event& aEv) const noexcept { return aEv < states_.size() ? states_[aEv] : false; }
    size_t setState(const SimuEvents&);  // ret real changed ev#

    Event  setPrev(const EvName&, const SimuEvents&) noexcept;  // be careful not create eg ttue-false loop
    EvName whyFalse(const Event&) const noexcept;

protected:
    const EvName& evName_(const Event& aValidEv) const noexcept { return ev_en_.at(aValidEv); }
    virtual void  effect_(const Event& aEv) noexcept {}  // can't const since FreeDom will rm hdlr

    // - rm self dom's resource (RISK: aEv's leaf(s) may become orphan!!!)
    // - virtual for each dom (& trigger base to free its resource)
    virtual void  rmEv_(const Event& aValidEv) noexcept;
    virtual Event recycleEv_() noexcept { return D_EVENT_FAILED_RET; }

private:
    void deduceStateFrom_(const Event& aValidEv) noexcept;
    bool deduceStateSelf_(const Event& aValidEv, bool aPrevType) const noexcept;
    void effect_() noexcept;

    bool pureSetStateOK_(const Event& aValidEv, const bool aNewState) noexcept;
    void pureSetPrev_(const Event& aValidEv, const SimuEvents&) noexcept;
    void pureRmLink_(const Event& aValidEv, EvLinks& aMyLinks, EvLinks& aNeighborLinks) noexcept;

    bool isNextFromTo_(const Event& aFromValidEv, const Event& aToValidEv) const noexcept;

    struct WhyStep{ Event curEV_; bool whyFlag_; EvName resultEN_; };
    void whyTrue_ (WhyStep&) const noexcept;
    void whyFalse_(WhyStep&) const noexcept;

    static const Events& findPeerEVs(const Event&, const EvLinks&) noexcept;

    // -------------------------------------------------------------------------------------------
    std::vector<bool>                 states_;               // bitmap & dyn expand, [event]=t/f

    EvLinks                           prev_[N_EVENT_STATE];  // not unordered-map since most traversal
    EvLinks                           next_[N_EVENT_STATE];  // not unordered-map since most traversal
    std::unordered_map<EvName, Event> en_ev_;                // [evName]=event
    EvNames                           ev_en_;                // [event]=evName for easy debug
    std::unordered_set<Event>         effectEVs_;
};

}  // namespace
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2017-01-04  CSZ       1)create EventDriver
// 2017-02-10  CSZ       - multi-hdlrs
// 2017-02-13  CSZ       2)only false to true can call hdlr
// 2017-02-16  CSZ       - event key
// 2017-05-07  CSZ       - hdlr always via msgSelf (no direct call)
// 2018-01-01  CSZ       3)rename to Domino, whom() to prev()/next()
// 2018-02-24  CSZ       - separate Domino to Basic, Key, Priority, ReadWrite, etc
// 2018-02-28  CSZ       - SimuDomino
// 2018-08-16  CSZ       - rm throw-exception
// 2018-09-18  CSZ       - support repeated event/hdlr
// 2018-10-30  CSZ       - simultaneous setPrev()
// 2018-10-31  CSZ       4)Event:EvName=1:1(simpler); Event=internal, EvName=external
// 2019-11-11  CSZ       - debug(rm/hide/nothingChanged)
//                       5)domino is practical; waiting formal & larger practice
// 2020-02-06  CSZ       - whyFalse(EvName)
// 2020-04-03  CSZ       - contain msgSelf_ so multi Domino can be used simulteneously
// 2020-06-10  CSZ       - dmnID_ to identify different domino instance
// 2020-08-21  CSZ       - most map to unordered_map for search performance
// 2020-11-17  CSZ       - multi-hdlr from Domino to MultiHdlrDomino since single hdlr is enough
//                         for most Domino usecases
// 2020-11-26  CSZ       - SmartLog for UT
//                       + DOMINO return full nested dominos
//                       + minimize derived dominos (eg FreeHdlrDomino is only rmOneHdlrOK() in triggerHdlr_())
//                       - minimize all dominos' mem func
//                       - ref instead of cp in all mem func
//                       - EvName for outer interface, Event for inner
//                       - priority req/ut first
// 2020-12-11  CSZ       6)separate hdlr from Domino to HdlrDomino
// 2021-03-24  CSZ       - auto&& for-loop
//                       - auto&& var = func()
//                       - ref ret of getEvNameBy()
// 2021-04-01  CSZ       - coding req
// 2022-01-17  PJ & CSZ  - formal log & naming
// 2022-03-26  CSZ       - ut's PARA_DOM include self class & ALL its base class(es)
// 2022-03-27  CSZ       - if ut case can test base class, never specify derive
// 2022-08-18  CSZ       - replace CppLog by UniLog
// 2023-01-11  CSZ       - search partial EvName
// 2023-11-14  CSZ       7)rm event
// 2024-02-29  CSZ       8)setPrev() is simple-all-safe
// 2024-03-03  CSZ       - enhance safe of rm ev (shall deduceState_(next))
//                       - enhance safe of whyFalse() while keep safe of newEvent()
// 2024-03-19  CSZ       9)1-go domino -> n-go domino
// 2025-03-31  CSZ       10)tolerate exception; rm recursion
// ***********************************************************************************************
// - where:
//   . start using domino for time-cost events
//   . 99% for intra-process (while can support inter-process)
//   . Ltd. - limit duty
//
// - why Event is not a class?
//   . states_ is better as bitmap, prev_/next_/hdlrs_/priorities_ may not exist for a event
//   . easier BasicDomino, PriDomino, etc
//   . no need new/share_ptr the Event class
//   . only 1 class(Domino) is for all access
//
// - dom try to reuse rm-ed Ev as early as possible to avoid mem-use-up in the most usage
//   . dom  can't solve mem-use-up so no code to handle exception
//
// - why template PriDomino, etc
//   . easy combine: eg Basic + Pri + Rw or Basic + Rw or Basic + Pri
//   . direct to get interface of all combined classes (inherit)
//
// - why not separate EvNameDomino from BasicDomino?
//   . BasicDomino's debug needs EvNameDomino
//
// - why not separate SimuDomino from BasicDomino?
//   . tight couple at setState()
//
// - why Event:EvName=1:1?
//   . simplify complex scenario eg rmOneHdlrOK() may relate with multi-hdlr
//   . simplify interface: EvName only, Event is internal-use only
//
// - why rm "#define PRI_DOMINO (ObjAnywhere::getObj<PriDomino<Domino> >())"
//   . PriDomino<Domino> != PriDomino<DataDomino<Domino> >, ObjAnywhere::getObj() may ret null
//
// - why no nHdlr() & hasHdlr()
//   . MultiHdlrDomino also need to support them
//   . UT shall not check them but check cb - more direct
//
// - why must EvName on ALL interface?
//   . EvName is much safer than Event (random Event could be valid, but rare of random EvName)
//     . read-only public interface can use Event since no hurt inner Dom
//     . inner mem-func can use Event
//   . EvName is lower performance than Event?
//     . a little, hope compiler optimize it
//   . Can buffer last EvName ptr to speedup?
//     . dangeous: diff func could create EvName at same address in stack
//     . 021-09-22: all UT, only 41% getEventBy() can benefit by buffer, not worth vs dangeous
//
// - why not rm Ev
//   . may impact related prev/next Ev, complex & out-control
//   . dangeous: may break deduced path
//     . rm entire Dom is safer
//     . not must-have req
//     . but keep inc Ev is not sustainable (may use up mem), eg swm's dom for version ctrl
//       . details in RmEvDomino
//   . can rm hdlr & data
//
// - why global states (like global_var) via eg ObjAnywhere
//   * easily bind & auto broadcast (like real dominos)
//     * auto shape in different scenario
//   * easily link hdlr with these states & auto callback
//     . conclusive callback instead of trigger
//     * can min hdlr's conditions & call as-early-as possible
//     . easy adapt change since each hdlr/event has limited pre-condition, auto impact others
//   * light weight (better than IM)
//   * need not base-derive classes, eg RuAgent/SmodAgent need not derive from BaseAgent
//     . 1 set hdlrs with unique NDL/precheck/RFM/RB/FB domino set
//   . evNames()
//     . to search partial EvName (for eg rm subtree's hdlrs)
//     . simplest to ret & (let user impl partial/template/etc match)
//     . safe to ret const (don't defense users' abusing)
//     . search DomDoor? no untill real req
//
// - how:
//   *)trigger
//     . prefer time-cost events
//     . how serial coding while parallel running
//       1) prepare events/tiles step by step
//       2) when all done, let DominoEvents start execution
//     . the more complex, the more benefit
//   *)mobile: any code can be called via Domino from anywhere
//     . cloud vs tree (intra-process)
//     . multi-cloud vs 1 bigger tree (inter-process)
//     . share data
//   .)replace state machine
//     . more direct
//     . no waste code for each state
//   .)msgSelf instead of callback directly
//     . avoid strange bugs
//     . serial coding/reading but parallel executing
//     . can also support callback, but not must-have, & complex code/logic
//
// - suggestion:
//   . use domino only when better than no-use
//   . event hdlr shall cover succ, fail, not needed, etc all cases, to ease following code
//
// - todo:
//   . limit-write event to owner
//   . micro-service of PriDomino, RwDomino
//   . connect micro-service?
