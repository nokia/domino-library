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
}
TYPED_TEST_P(DominoTest, UC_immediateDeduce_trueState)
{
    PARA_DOM->newEvent("rest");             // req: create
    EXPECT_FALSE(PARA_DOM->state("rest"));  // req: default state

    PARA_DOM->setPrev("rest", {{"weekday", false}, {"OT", false}});  // req: prerequisite can be false
    EXPECT_FALSE(PARA_DOM->state("weekday"));  // req: default state
    EXPECT_FALSE(PARA_DOM->state("OT"));    // req: default state
    EXPECT_TRUE (PARA_DOM->state("rest"));  // req: immediate deduce
}
TYPED_TEST_P(DominoTest, UC_reDeduce_trueState)
{
    PARA_DOM->setState({{"hungry", true}, {"food", true}});  // req: can set state simultaneously
    PARA_DOM->setPrev("eat", {{"hungry", true}, {"food", true}});  // req: set prev simultaneously to avoid mis-deduce
    EXPECT_TRUE (PARA_DOM->state("eat"));  // req: 1st deduce

    PARA_DOM->setState({{"eat", false}});  // req: can force tile up
    EXPECT_FALSE(PARA_DOM->state("eat"));  // req: no deduce since no trigger

    PARA_DOM->setState({{"food", false}});
    EXPECT_FALSE(PARA_DOM->state("eat"));  // req: no trigger

    PARA_DOM->setState({{"food", true}});  // retrigger
    EXPECT_TRUE(PARA_DOM->state("eat"));   // req: re-deduce
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
    , UC_reDeduce_trueState
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
