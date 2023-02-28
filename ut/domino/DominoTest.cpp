/**
 * Copyright 2016 Nokia. All rights reserved.
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include <memory>  // for shared_ptr
#include <set>

#include "UtInitObjAnywhere.hpp"

namespace RLib
{
// ***********************************************************************************************
template<class aParaDom>
struct DominoTest : public UtInitObjAnywhere
{
    set<Domino::Event> uniqueEVs_;
};
TYPED_TEST_SUITE_P(DominoTest);

#define BROADCAST_STATE
// ***********************************************************************************************
// - req: forward broadcast
// - req: not backward broadcast
// - req: true/false broadcast are same
TYPED_TEST_P(DominoTest, UC_autoDeduce_trueState)
{
    PARA_DOM->setPrev("eat", {{"hungry", true}, {"food", true}});  // req: set prerequisite
    EXPECT_FALSE(PARA_DOM->state("hungry"));  // req: default state
    EXPECT_FALSE(PARA_DOM->state("food"));    // req: default state
    EXPECT_FALSE(PARA_DOM->state("eat"));     // req: default state

    PARA_DOM->setState({{"hungry", true}});   // req: change state
    EXPECT_TRUE (PARA_DOM->state("hungry"));  // req: get new state
    EXPECT_FALSE(PARA_DOM->state("eat"));     // req: no change since not all prerequisite satisfied

    PARA_DOM->setState({{"food", true}});     // req: change state
    EXPECT_TRUE(PARA_DOM->state("eat"));      // req: auto deduce
#if 0
    // init, e1=up, e2=up
    PARA_DOM->setPrev("e2", {{"e1", true}});
    EXPECT_FALSE(PARA_DOM->state("e1"));
    EXPECT_FALSE(PARA_DOM->state("e2"));

    PARA_DOM->setState({{"e1", false}});  // forward-1: e1 up-up -> e2 up-up (like real domino)
    EXPECT_FALSE(PARA_DOM->state("e1"));
    EXPECT_FALSE(PARA_DOM->state("e2"));

    // -------------------------------------------------------------------------------------------
    PARA_DOM->setState({{"e2", false}});  // backward-1: e1 up-up <- e2 up-up (like real domino)
    EXPECT_FALSE(PARA_DOM->state("e1"));
    EXPECT_FALSE(PARA_DOM->state("e2"));

    PARA_DOM->setState({{"e2", true}});   // backward-2: e1 up-up <- e2 up-down (like real domino)
    EXPECT_FALSE(PARA_DOM->state("e1"));
    EXPECT_TRUE(PARA_DOM->state("e2"));

    PARA_DOM->setState({{"e2", true}});   // backward-3: e1 up-up <- e2 down-down (like real domino)
    EXPECT_FALSE(PARA_DOM->state("e1"));
    EXPECT_TRUE(PARA_DOM->state("e2"));

    PARA_DOM->setState({{"e2", false}});  // backward-4: e1 up-up <- e2 down-up (like real domino)
    EXPECT_FALSE(PARA_DOM->state("e1"));
    EXPECT_FALSE(PARA_DOM->state("e2"));

    // -------------------------------------------------------------------------------------------
    PARA_DOM->setState({{"e1", true}});   // forward-2: e1 up-down -> e2 up-down (like real domino!!!)
    EXPECT_TRUE(PARA_DOM->state("e1"));
    EXPECT_TRUE(PARA_DOM->state("e2"));

    PARA_DOM->setState({{"e1", true}});   // forward-3: e1 down-down -> e2 down-down (like real domino)
    EXPECT_TRUE(PARA_DOM->state("e1"));
    EXPECT_TRUE(PARA_DOM->state("e2"));

    PARA_DOM->setState({{"e1", false}});  // forward-4: e1 down-up -> e2 down-down (like real domino)
    EXPECT_FALSE(PARA_DOM->state("e1"));
    EXPECT_TRUE(PARA_DOM->state("e2"));

    PARA_DOM->setState({{"e1", false}});  // forward-5: e1 up-up -> e2 down-down (like real domino)
    EXPECT_FALSE(PARA_DOM->state("e1"));
    EXPECT_TRUE(PARA_DOM->state("e2"));

    PARA_DOM->setState({{"e1", true}});   // forward-6: e1 up-down -> e2 down-down (like real domino)
    EXPECT_TRUE(PARA_DOM->state("e1"));
    EXPECT_TRUE(PARA_DOM->state("e2"));

    // -------------------------------------------------------------------------------------------
    PARA_DOM->setState({{"e2", true}});   // backward-5: e1 down-down <- e2 down-down (like real domino)
    EXPECT_TRUE(PARA_DOM->state("e1"));
    EXPECT_TRUE(PARA_DOM->state("e2"));

    PARA_DOM->setState({{"e2", false}});  // backward-6: e1 down-down <- e2 down-up (no backward broadcast)
    EXPECT_TRUE(PARA_DOM->state("e1"));
    EXPECT_FALSE(PARA_DOM->state("e2"));

    PARA_DOM->setState({{"e2", false}});  // backward-7: e1 down-down <- e2 up-up (like real domino)
    EXPECT_TRUE(PARA_DOM->state("e1"));
    EXPECT_FALSE(PARA_DOM->state("e2"));

    PARA_DOM->setState({{"e2", true}});   // backward-8: e1 down-down <- e2 up-down (like real domino)
    EXPECT_TRUE(PARA_DOM->state("e1"));
    EXPECT_TRUE(PARA_DOM->state("e2"));

    // -------------------------------------------------------------------------------------------
    PARA_DOM->setState({{"e2", false}});  // e1 down-down <- e2 down-up
    EXPECT_TRUE(PARA_DOM->state("e1"));
    EXPECT_FALSE(PARA_DOM->state("e2"));
    PARA_DOM->setState({{"e1", true}});   // forward-7: e1 down-down -> e2 up-down (still forward!!!)
    EXPECT_TRUE(PARA_DOM->state("e1"));
    EXPECT_TRUE(PARA_DOM->state("e2"));

    PARA_DOM->setState({{"e2", false}});  // e1 down-down <- e2 down-up
    EXPECT_TRUE(PARA_DOM->state("e1"));
    EXPECT_FALSE(PARA_DOM->state("e2"));
    PARA_DOM->setState({{"e1", false}});  // forward-8: e1 down-up -> e2 up-up (like real domino)
    EXPECT_FALSE(PARA_DOM->state("e1"));
    EXPECT_FALSE(PARA_DOM->state("e2"));
#endif
}
TYPED_TEST_P(DominoTest, UC_immediateDeduce_trueState)
{
    PARA_DOM->newEvent("work");             // req: create
    EXPECT_FALSE(PARA_DOM->state("work"));  // req: default state

    PARA_DOM->setPrev("work", {{"weekend", false}, {"OT", false}});  // req: prerequisite can be false
    EXPECT_FALSE(PARA_DOM->state("weekend"));  // req: default state
    EXPECT_FALSE(PARA_DOM->state("OT"));    // req: default state
    EXPECT_TRUE (PARA_DOM->state("work"));  // req: immediate deduce

    PARA_DOM->setState({{"OT", true}});
    EXPECT_TRUE(PARA_DOM->state("work"));   // req: no deduce if downstream tile already down/true

    PARA_DOM->setState({{"work", false}});
    EXPECT_FALSE(PARA_DOM->state("work"));  // req: support tile re-up (then re-down)

    PARA_DOM->setState({{"OT", false}});
    EXPECT_TRUE(PARA_DOM->state("work"));   // req: re-down
#if 0
    // false broadcast
    PARA_DOM->setPrev("e2", {{"e1", false}});
    EXPECT_FALSE(PARA_DOM->state("e1"));
    EXPECT_TRUE(PARA_DOM->state("e2"));

    // true broadcast
    PARA_DOM->setState({{"e3", true}});
    PARA_DOM->setPrev("e4", {{"e3", true}});
    EXPECT_TRUE(PARA_DOM->state("e3"));
    EXPECT_TRUE(PARA_DOM->state("e4"));
#endif
}
TYPED_TEST_P(DominoTest, UC_broadcast_onlyWhen_allPrev_satisfied)
{
    PARA_DOM->setPrev("e1", {{"e2", true}, {"e3", true}});  // multi
    PARA_DOM->setPrev("e4", {{"e1", true}});  // chain
    EXPECT_FALSE(PARA_DOM->state("e1"));
    EXPECT_FALSE(PARA_DOM->state("e4"));

    PARA_DOM->setState({{"e2", true}});  // req: not allPrev satisfied
    EXPECT_FALSE(PARA_DOM->state("e1"));
    EXPECT_FALSE(PARA_DOM->state("e4"));

    PARA_DOM->setState({{"e3", true}});  // req: full satisfied
    EXPECT_TRUE(PARA_DOM->state("e1"));
    EXPECT_TRUE(PARA_DOM->state("e4"));
}
TYPED_TEST_P(DominoTest, prevSelf_is_invalid)
{
    EXPECT_EQ(Domino::D_EVENT_FAILED_RET, PARA_DOM->setPrev("e1", {{"e1", true}}));
    EXPECT_EQ(Domino::D_EVENT_FAILED_RET, PARA_DOM->setPrev("e1", {{"e1", false}}));
}

#define WHY_FALSE
// ***********************************************************************************************
// req: SmodAgent's file-need-check is based on 200+ files, too hard to debug why decide not to rom/plan
// - ask Domino to find 1 prev-event with specified flag (true/false)
//   . so SmodAgent can log_ << PARA_DOM.whyFalse(EnSmod_IS_FNC_TO_ROM_PLAN)
// - no need whyTrue() since all prev-event must be satisfied
// ***********************************************************************************************
TYPED_TEST_P(DominoTest, UC_multi_retOne)
{
    PARA_DOM->setPrev("master succ", {{"all agents succ", true}, {"user abort", false}});
    EXPECT_EQ("all agents succ==false", PARA_DOM->whyFalse("master succ"));

    PARA_DOM->setState({{"all agents succ", true}, {"user abort", true}});
    EXPECT_EQ("user abort==true", PARA_DOM->whyFalse("master succ"));
}
TYPED_TEST_P(DominoTest, UC_trueEvent_retEmpty)
{
    PARA_DOM->setPrev("master succ", {{"all agents succ", true}, {"user abort", false}});
    PARA_DOM->setState({{"all agents succ", true}});
    EXPECT_TRUE(PARA_DOM->state("master succ"));
    EXPECT_TRUE(PARA_DOM->whyFalse("master succ").empty());
}
TYPED_TEST_P(DominoTest, eventWithoutPrev_retEmpty)
{
    PARA_DOM->newEvent("no prev ev");
    EXPECT_FALSE(PARA_DOM->state("no prev ev"));
    EXPECT_TRUE(PARA_DOM->whyFalse("no prev ev").empty());
}
TYPED_TEST_P(DominoTest, invalidEvent_retEmpty)
{
    EXPECT_TRUE(PARA_DOM->whyFalse("").empty());
}

#define SEARCH_PARTIAL_EVNAME
// ***********************************************************************************************
TYPED_TEST_P(DominoTest, UC_search_partial_evName)
{
    PARA_DOM->newEvent("/A");
    PARA_DOM->newEvent("/A/B");
    auto&& evNames = PARA_DOM->evNames();

    size_t nFound = 0;
    for (auto&& evName : evNames)
    {
        if (evName.find("/A") != string::npos) ++nFound;
    }
    EXPECT_EQ(2u, nFound);  // req: found

    nFound = 0;
    for (auto&& evName : evNames)
    {
        if (evName.find("/X") != string::npos) ++nFound;
    }
    EXPECT_EQ(0u, nFound);  // req: not found
}

#define ID_STATE
// ***********************************************************************************************
// req: both event & EvName are ID
// ***********************************************************************************************
TYPED_TEST_P(DominoTest, nonConstInterface_shall_createUnExistEvent_withStateFalse)
{
    // req: new ID by newEvent()
    this->uniqueEVs_.insert(PARA_DOM->newEvent(""));  // req: create new
    this->uniqueEVs_.insert(PARA_DOM->newEvent(""));  // req: no dup
    EXPECT_EQ(1u, this->uniqueEVs_.size());
    EXPECT_FALSE(PARA_DOM->state(""));  // req: default state is false

    // req: new ID by setState()
    PARA_DOM->setState({{"", false}, {"e2", false}});
    this->uniqueEVs_.insert(PARA_DOM->getEventBy("e2"));
    PARA_DOM->setState({{"", false}, {"e2", false}});
    this->uniqueEVs_.insert(PARA_DOM->getEventBy("e2"));
    EXPECT_EQ(2u, this->uniqueEVs_.size());
    EXPECT_FALSE(PARA_DOM->state("e2"));

    // req: new ID by setPrev()
    PARA_DOM->setPrev("e3", {{"e4", true}});
    this->uniqueEVs_.insert(PARA_DOM->getEventBy("e3"));
    this->uniqueEVs_.insert(PARA_DOM->getEventBy("e4"));
    PARA_DOM->setPrev("e3", {{"e4", true}});
    this->uniqueEVs_.insert(PARA_DOM->getEventBy("e3"));
    this->uniqueEVs_.insert(PARA_DOM->getEventBy("e4"));
    EXPECT_EQ(4u, this->uniqueEVs_.size());
    EXPECT_FALSE(PARA_DOM->state("e3"));
    EXPECT_FALSE(PARA_DOM->state("e4"));

    this->uniqueEVs_.insert(Domino::D_EVENT_FAILED_RET);  // req: new ID != Domino::D_EVENT_FAILED_RET
    EXPECT_EQ(5u, this->uniqueEVs_.size());
}
TYPED_TEST_P(DominoTest, noID_for_not_exist_EvName)
{
    EXPECT_EQ(Domino::D_EVENT_FAILED_RET, PARA_DOM->getEventBy(""));
}
TYPED_TEST_P(DominoTest, stateFalse_for_not_exist_EvName)
{
    EXPECT_FALSE(PARA_DOM->state(""));
}
TYPED_TEST_P(DominoTest, GOLD_setState_thenGetIt)
{
    PARA_DOM->setState({{"", true}, {"e2", false}});  // init set multi
    EXPECT_TRUE(PARA_DOM->state(""));
    EXPECT_FALSE(PARA_DOM->state("e2"));

    PARA_DOM->setState({{"", false}});  // dup set 1 of the 2
    EXPECT_FALSE(PARA_DOM->state(""));
    EXPECT_FALSE(PARA_DOM->state("e2"));
}

// ***********************************************************************************************
REGISTER_TYPED_TEST_SUITE_P(DominoTest
    , UC_autoDeduce_trueState
    , UC_immediateDeduce_trueState
    , UC_broadcast_onlyWhen_allPrev_satisfied
    , prevSelf_is_invalid

    , UC_multi_retOne
    , UC_trueEvent_retEmpty
    , eventWithoutPrev_retEmpty
    , invalidEvent_retEmpty

    , UC_search_partial_evName

    , nonConstInterface_shall_createUnExistEvent_withStateFalse
    , noID_for_not_exist_EvName
    , stateFalse_for_not_exist_EvName
    , GOLD_setState_thenGetIt
);
using AnyDom = Types<Domino, MinDatDom, MinWbasicDatDom, MinHdlrDom, MinMhdlrDom, MinPriDom,
    MinFreeDom, MaxNofreeDom, MaxDom>;
INSTANTIATE_TYPED_TEST_SUITE_P(PARA, DominoTest, AnyDom);
}  // namespace
