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

#define STATE
// ***********************************************************************************************
TYPED_TEST_P(DominoTest, UC_setState_thenGetIt)
{
    EXPECT_FALSE(PARA_DOM->state("e1"));  // req: non-exist ev's state=F

    PARA_DOM->newEvent("e1");
    EXPECT_FALSE(PARA_DOM->state("e1"));  // req: new ev's state=F

    PARA_DOM->setState({{"e1", true}});
    EXPECT_TRUE(PARA_DOM->state("e1"));   // req: set T then get it

    PARA_DOM->setState({{"e1", false}});
    EXPECT_FALSE(PARA_DOM->state("e1"));  // req: set F then get it
}

#define BROADCAST_STATE
// ***********************************************************************************************
TYPED_TEST_P(DominoTest, UC_forward_broadcast)
{
    // e1->e2->e3
    PARA_DOM->setPrev("e2", {{"e1", true}});
    PARA_DOM->setPrev("e3", {{"e2", true}});
    EXPECT_FALSE(PARA_DOM->state("e1"));
    EXPECT_FALSE(PARA_DOM->state("e2"));
    EXPECT_FALSE(PARA_DOM->state("e3"));

    PARA_DOM->setState({{"e1", true}});  // set beginning
    EXPECT_TRUE(PARA_DOM->state("e1"));
    EXPECT_TRUE(PARA_DOM->state("e2"));  // req: broadcast e1->e2
    EXPECT_TRUE(PARA_DOM->state("e3"));  // req: broadcast e2->e3
}
TYPED_TEST_P(DominoTest, UC_no_backward_broadcast)
{
    // e1->e2->e3
    PARA_DOM->setPrev("e2", {{"e1", true}});
    PARA_DOM->setPrev("e3", {{"e2", true}});

    PARA_DOM->setState({{"e2", true}});   // set middle
    EXPECT_FALSE(PARA_DOM->state("e1"));  // req: no e1<-e2
    EXPECT_TRUE (PARA_DOM->state("e2"));
    EXPECT_TRUE (PARA_DOM->state("e3"));
}
TYPED_TEST_P(DominoTest, UC_re_broadcast_byTrue)
{
    // e1->e2->e3
    PARA_DOM->setPrev("e2", {{"e1", true}});
    PARA_DOM->setPrev("e3", {{"e2", true}});
    PARA_DOM->setState({{"e1", true}});   // 1st broadcast

    PARA_DOM->setState({{"e3", false}});  // req: can force T->F
    EXPECT_TRUE (PARA_DOM->state("e1"));
    EXPECT_TRUE (PARA_DOM->state("e2"));
    EXPECT_FALSE(PARA_DOM->state("e3"));

    PARA_DOM->setState({{"e1", true}});   // req: e1=T->T also trigger broadcast
    EXPECT_TRUE(PARA_DOM->state("e1"));
    EXPECT_TRUE(PARA_DOM->state("e2"));   // req: broadcast e1->e2
    EXPECT_TRUE(PARA_DOM->state("e3"));   // req: broadcast e2->e3
}
TYPED_TEST_P(DominoTest, UC_re_broadcast_byFalse)
{
    // e4->e5
    PARA_DOM->setPrev("e5", {{"e4", false}});  // false relationship
    EXPECT_FALSE(PARA_DOM->state("e4"));
    EXPECT_TRUE (PARA_DOM->state("e5"));       // 1st broadcast

    PARA_DOM->setState({{"e5", false}});       // force e5: T->F
    EXPECT_FALSE(PARA_DOM->state("e5"));
    EXPECT_FALSE(PARA_DOM->state("e4"));

    PARA_DOM->setState({{"e4", false}});       // req: force e4: F->F, also trigger broadcast
    EXPECT_TRUE (PARA_DOM->state("e5"));
    EXPECT_FALSE(PARA_DOM->state("e4"));
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
    PARA_DOM->setPrev("master succ", {{"all agents succ", true}, {"user abort", false}});  // req: simultaneous set
    EXPECT_EQ("all agents succ==false", PARA_DOM->whyFalse("master succ"));

    PARA_DOM->setState({{"all agents succ", true}, {"user abort", true}});  // req: simultaneous state
    EXPECT_EQ("user abort==true", PARA_DOM->whyFalse("master succ"));  // req: ret 1 unsatisfied pre
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

#define ID
// ***********************************************************************************************
// req: both event & EvName are ID
// ***********************************************************************************************
TYPED_TEST_P(DominoTest, nonConstInterface_shall_createUnExistEvent_withStateFalse)
{
    // req: new ID by newEvent()
    this->uniqueEVs_.insert(PARA_DOM->newEvent(""));  // req: create new
    this->uniqueEVs_.insert(PARA_DOM->newEvent(""));  // req: no dup
    EXPECT_EQ(1u, this->uniqueEVs_.size());

    // req: new ID by setState()
    PARA_DOM->setState({{"", false}, {"e2", false}});
    this->uniqueEVs_.insert(PARA_DOM->getEventBy("e2"));
    PARA_DOM->setState({{"", false}, {"e2", false}});
    this->uniqueEVs_.insert(PARA_DOM->getEventBy("e2"));
    EXPECT_EQ(2u, this->uniqueEVs_.size());

    // req: new ID by setPrev()
    PARA_DOM->setPrev("e3", {{"e4", true}});
    this->uniqueEVs_.insert(PARA_DOM->getEventBy("e3"));
    this->uniqueEVs_.insert(PARA_DOM->getEventBy("e4"));
    PARA_DOM->setPrev("e3", {{"e4", true}});
    this->uniqueEVs_.insert(PARA_DOM->getEventBy("e3"));
    this->uniqueEVs_.insert(PARA_DOM->getEventBy("e4"));
    EXPECT_EQ(4u, this->uniqueEVs_.size());

    this->uniqueEVs_.insert(Domino::D_EVENT_FAILED_RET);  // req: new ID != Domino::D_EVENT_FAILED_RET
    EXPECT_EQ(5u, this->uniqueEVs_.size());
}
TYPED_TEST_P(DominoTest, noID_for_not_exist_EvName)
{
    EXPECT_EQ(Domino::D_EVENT_FAILED_RET, PARA_DOM->getEventBy(""));
}

// ***********************************************************************************************
REGISTER_TYPED_TEST_SUITE_P(DominoTest
    , UC_setState_thenGetIt

    , UC_forward_broadcast
    , UC_no_backward_broadcast
    , UC_re_broadcast_byTrue
    , UC_re_broadcast_byFalse
    , prevSelf_is_invalid

    , UC_multi_retOne
    , UC_trueEvent_retEmpty
    , eventWithoutPrev_retEmpty
    , invalidEvent_retEmpty

    , UC_search_partial_evName

    , nonConstInterface_shall_createUnExistEvent_withStateFalse
    , noID_for_not_exist_EvName
);
using AnyDom = Types<Domino, MinDatDom, MinWbasicDatDom, MinHdlrDom, MinMhdlrDom, MinPriDom,
    MinFreeDom, MaxNofreeDom, MaxDom>;
INSTANTIATE_TYPED_TEST_SUITE_P(PARA, DominoTest, AnyDom);
}  // namespace
