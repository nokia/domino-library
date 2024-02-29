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
TYPED_TEST_P(DominoTest, GOLD_setState_thenGetIt)
{
    EXPECT_FALSE(PARA_DOM->state("e1")) << "REQ: non-exist ev's state=F";

    PARA_DOM->newEvent("e1");
    EXPECT_FALSE(PARA_DOM->state("e1")) << "REQ: new ev's state=F";

    PARA_DOM->setState({{"e1", true}});
    EXPECT_TRUE(PARA_DOM->state("e1")) << "REQ: set T then get it";

    PARA_DOM->setState({{"e1", false}});
    EXPECT_FALSE(PARA_DOM->state("e1")) << "REQ: set F then get it";
}

#define BROADCAST_STATE
// ***********************************************************************************************
TYPED_TEST_P(DominoTest, GOLD_forward_broadcast)
{
    // e1->e2->e3
    PARA_DOM->setPrev("e2", {{"e1", true}});
    PARA_DOM->setPrev("e3", {{"e2", true}});
    EXPECT_FALSE(PARA_DOM->state("e1"));
    EXPECT_FALSE(PARA_DOM->state("e2"));
    EXPECT_FALSE(PARA_DOM->state("e3"));

    PARA_DOM->setState({{"e1", true}});  // set beginning
    EXPECT_TRUE(PARA_DOM->state("e1"));
    EXPECT_TRUE(PARA_DOM->state("e2")) << "REQ: broadcast e1->e2";
    EXPECT_TRUE(PARA_DOM->state("e3")) << "REQ: broadcast e2->e3";
}
TYPED_TEST_P(DominoTest, no_backward_broadcast)
{
    // e1->e2->e3
    PARA_DOM->setPrev("e2", {{"e1", true}});
    PARA_DOM->setPrev("e3", {{"e2", true}});

    PARA_DOM->setState({{"e2", true}});   // set middle
    EXPECT_FALSE(PARA_DOM->state("e1")) << "REQ: no e1<-e2";
    EXPECT_TRUE (PARA_DOM->state("e2"));
    EXPECT_TRUE (PARA_DOM->state("e3"));
}
TYPED_TEST_P(DominoTest, GOLD_re_broadcast_byTrue)
{
    // e1->e2->e3
    PARA_DOM->setPrev("e2", {{"e1", true}});
    PARA_DOM->setPrev("e3", {{"e2", true}});
    PARA_DOM->setState({{"e1", true}});   // 1st broadcast

    PARA_DOM->setState({{"e3", false}});  // REQ: can force T->F
    EXPECT_TRUE (PARA_DOM->state("e1"));
    EXPECT_TRUE (PARA_DOM->state("e2"));
    EXPECT_FALSE(PARA_DOM->state("e3"));

    PARA_DOM->setState({{"e1", true}});   // req: e1=T->T also trigger broadcast
    EXPECT_TRUE(PARA_DOM->state("e1"));
    EXPECT_TRUE(PARA_DOM->state("e2")) << "REQ: broadcast e1->e2";
    EXPECT_TRUE(PARA_DOM->state("e3")) << "REQ: broadcast e2->e3";
}
TYPED_TEST_P(DominoTest, GOLD_re_broadcast_byFalse)
{
    // e4->e5
    PARA_DOM->setPrev("e5", {{"e4", false}});  // false relationship
    EXPECT_FALSE(PARA_DOM->state("e4"));
    EXPECT_TRUE (PARA_DOM->state("e5"));       // 1st broadcast

    PARA_DOM->setState({{"e5", false}});       // force e5: T->F
    EXPECT_FALSE(PARA_DOM->state("e5"));
    EXPECT_FALSE(PARA_DOM->state("e4"));

    PARA_DOM->setState({{"e4", false}});  // REQ: force e4: F->F, also trigger broadcast
    EXPECT_TRUE (PARA_DOM->state("e5"));
    EXPECT_FALSE(PARA_DOM->state("e4"));
}
TYPED_TEST_P(DominoTest, bugFix_shallDeduceAll)
{
    PARA_DOM->setPrev("e2", {{"e1", true}});
    PARA_DOM->setState({
        {"e0", false},  // no-next e0 shall not abort deducing e1
        {"e1", true},
        {"e0", false}});
    EXPECT_TRUE(PARA_DOM->state("e2"));
}

#define LOOP
// ***********************************************************************************************
TYPED_TEST_P(DominoTest, loopSelf_is_invalid)
{
    EXPECT_EQ(Domino::D_EVENT_FAILED_RET, PARA_DOM->setPrev("e1", {{"e1", true}}));
    EXPECT_EQ(Domino::D_EVENT_FAILED_RET, PARA_DOM->setPrev("e1", {{"e1", false}}));
}
TYPED_TEST_P(DominoTest, loop_check)
{
    EXPECT_FALSE(PARA_DOM->deeperLinkThan(0)) << "REQ: empty's deep=0";

    PARA_DOM->newEvent("e1");
    EXPECT_TRUE (PARA_DOM->deeperLinkThan(0)) << "REQ: deep=1";
    EXPECT_FALSE(PARA_DOM->deeperLinkThan(1)) << "REQ: deep=1";

    PARA_DOM->setPrev("e1", {{"e2", true}});
    EXPECT_TRUE (PARA_DOM->deeperLinkThan(1)) << "REQ: deep=2";
    EXPECT_FALSE(PARA_DOM->deeperLinkThan(2)) << "REQ: deep=1";

    PARA_DOM->setPrev("e2", {{"e1", true}});
    EXPECT_TRUE(PARA_DOM->deeperLinkThan(10000)) << "REQ: found loop";
}

#define WHY_FALSE
// ***********************************************************************************************
// req: SmodAgent's file-need-check is based on 200+ files, too hard to debug why decide not to rom/plan
// - ask Domino to find 1 prev-event with specified flag (true/false)
//   . so SmodAgent can log_ << PARA_DOM.whyFalse(EnSmod_IS_FNC_TO_ROM_PLAN)
// - no need whyTrue() since all prev-event must be satisfied
// ***********************************************************************************************
TYPED_TEST_P(DominoTest, GOLD_multi_retOne)
{
    PARA_DOM->setPrev("master succ", {{"all agents succ", true}, {"user abort", false}});  // REQ: simultaneous set
    EXPECT_EQ("all agents succ==false", PARA_DOM->whyFalse("master succ"));

    PARA_DOM->setState({{"all agents succ", true}, {"user abort", true}});  // REQ: simultaneous state
    EXPECT_EQ("user abort==true", PARA_DOM->whyFalse("master succ")) << "REQ: ret 1 unsatisfied pre";
}
TYPED_TEST_P(DominoTest, trueEvent_retEmpty)
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
TYPED_TEST_P(DominoTest, search_partial_evName)
{
    PARA_DOM->newEvent("/A");
    PARA_DOM->newEvent("/A/B");
    auto&& evNames = PARA_DOM->evNames();

    size_t nFound = 0;
    for (auto&& evName : evNames)
    {
        if (evName.second.find("/A") != string::npos) ++nFound;
    }
    EXPECT_EQ(2u, nFound) << "REQ: found";

    nFound = 0;
    for (auto&& evName : evNames)
    {
        if (evName.second.find("/X") != string::npos) ++nFound;
    }
    EXPECT_EQ(0u, nFound) << "REQ: not found";
}

#define ID
// ***********************************************************************************************
// req: both event & EvName are ID
// ***********************************************************************************************
TYPED_TEST_P(DominoTest, nonConstInterface_shall_createUnExistEvent_withStateFalse)
{
    // req: new ID by newEvent()
    this->uniqueEVs_.insert(PARA_DOM->newEvent(""));  // REQ: create new
    this->uniqueEVs_.insert(PARA_DOM->newEvent(""));  // REQ: no dup
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

    this->uniqueEVs_.insert(Domino::D_EVENT_FAILED_RET);  // REQ: new ID != Domino::D_EVENT_FAILED_RET
    EXPECT_EQ(5u, this->uniqueEVs_.size());
}
TYPED_TEST_P(DominoTest, noID_for_not_exist_EvName)
{
    EXPECT_EQ(Domino::D_EVENT_FAILED_RET, PARA_DOM->getEventBy(""));
}

// ***********************************************************************************************
REGISTER_TYPED_TEST_SUITE_P(DominoTest
    , GOLD_setState_thenGetIt

    , GOLD_forward_broadcast
    , no_backward_broadcast
    , GOLD_re_broadcast_byTrue
    , GOLD_re_broadcast_byFalse
    , bugFix_shallDeduceAll
    , loopSelf_is_invalid
    , loop_check

    , GOLD_multi_retOne
    , trueEvent_retEmpty
    , eventWithoutPrev_retEmpty
    , invalidEvent_retEmpty

    , search_partial_evName

    , nonConstInterface_shall_createUnExistEvent_withStateFalse
    , noID_for_not_exist_EvName
);
using AnyDom = Types<Domino, MinDatDom, MinWbasicDatDom, MinHdlrDom, MinMhdlrDom, MinPriDom,
    MinFreeDom, MinRmEvDom, MaxNofreeDom, MaxDom>;
INSTANTIATE_TYPED_TEST_SUITE_P(PARA, DominoTest, AnyDom);
}  // namespace
