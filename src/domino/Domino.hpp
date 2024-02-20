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
// - Q&A: (below; lots)
// - assmuption:
//   . each event-hdlr is called only when event state F->T
//   . repeated event/hdlr is complex, be careful(eg DominoTests.newTriggerViaChain)
//   . dead-loop: complex so not code check but may tool check (eg excel)
//
// - core: states_
//
// - VALUE:
//   * auto broadcast, auto callback, auto shape [MUST-HAVE!]
//   . share state & hdlr
//   . EvName index
//   . template extension (PriDomino, etc)
//
// - mem safe: yes with limit
//   . no use-up mem which is impossible in most cases
//   . user shall not loop link ev - hard, expensive & unreasonable
// ***********************************************************************************************
#ifndef DOMINO_HPP_
#define DOMINO_HPP_

#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "UniLog.hpp"

namespace RLib
{
// ***********************************************************************************************
class Domino : public UniLog
{
public:
    using Event      = size_t;  // smaller size can save mem; larger size can support more events
    using Events     = set<Event>;
    using EvName     = string;
    using SimuEvents = map<EvName, bool>;  // not unordered-map since most traversal
    using EvNames    = unordered_map<Event, EvName>;  // map is less mem than vector<EvName>
    using EvLinks    = map<Event, Events>;

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
    explicit Domino(const UniLogName& aUniLogName = ULN_DEFAULT) : UniLog(aUniLogName) {}
    virtual ~Domino() = default;

    Event    newEvent(const EvName&);
    Event    getEventBy(const EvName&) const;
    const EvNames& evNames() const { return evNames_; }

    bool     state(const EvName& aEvName) const { return state(getEventBy(aEvName)); }
    void     setState(const SimuEvents&);
    Event    setPrev(const EvName&, const SimuEvents& aSimuPrevEvents);
    EvName   whyFalse(const EvName&) const;

protected:
    const EvName& evName(const Event aEv) const { return evNames_.at(aEv); }  // aEv must valid
    bool          state (const Event aEv) const { return aEv < states_.size() ? states_[aEv] : false; }
    virtual void  effect(const Event) {}  // can't const since FreeDom will rm hdlr

    // - rm self dom's resource (RISK: aEv's leaf(s) may become orphan!!!)
    // - virtual for each dom (& trigger base to free its resource)
    virtual void innerRmEv(const Event aEv);
    virtual Event recycleEv() { return D_EVENT_FAILED_RET; }

private:
    void deduceState(const Event);
    void pureSetState(const Event, const bool aNewState);
    void pureRmLink(const Event, EvLinks& aMyLinks, EvLinks& aNeighborLinks);

    // -------------------------------------------------------------------------------------------
    vector<bool>                 states_;               // bitmap & dyn expand, [event]=t/f

    EvLinks                      prev_[N_EVENT_STATE];  // not unordered-map since most traversal
    EvLinks                      next_[N_EVENT_STATE];  // not unordered-map since most traversal
    unordered_map<EvName, Event> events_;               // [evName]=event
    EvNames                      evNames_;              // [event]=evName for easy debug
    bool                         sthChanged_ = false;   // for debug
};

}  // namespace
#endif  // DOMINO_HPP_
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
//                       + minimize derived dominos (eg FreeHdlrDomino is only pureRmHdlrOK() in triggerHdlr())
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
// - why whole DomLib not throw exception?
//   * simpler code without exception that inc 77% branches (Dec,2022)
//     . means exception may break current code
//   . wider usage (eg moam not allow exception)
//   . alloc resrc for new Ev may throw exception (from std lib)
//     . dom try to reuse rm-ed Ev as early as possible to avoid mem-use-up in the most usage
//     . dom  can't solve mem-use-up so no code to handle exception
// - why not noexcept for all member func?
//   * std::function not support noexcept (Feb,2024; g++11)
//   * no class level noexcept, func level is verbose, compile-opt -fno-exceptions is simpler
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
// - why rm "#define PRI_DOMINO (ObjAnywhere::get<PriDomino<Domino> >())"
//   . PriDomino<Domino> != PriDomino<DataDomino<Domino> >, ObjAnywhere::get() may ret null
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
