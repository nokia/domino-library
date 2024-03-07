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
TYPED_TEST_P(DominoTest, invalid_loopSelf)
{
    EXPECT_EQ(Domino::D_EVENT_FAILED_RET, PARA_DOM->setPrev("e1", {{"e1", true}})) << "REQ: can't loop self";
    EXPECT_EQ(Domino::D_EVENT_FAILED_RET, PARA_DOM->setPrev("e1", {{"e1", false}})) << "REQ: can't loop self";
}
TYPED_TEST_P(DominoTest, invalid_deepLoop)
{
    EXPECT_NE(Domino::D_EVENT_FAILED_RET, PARA_DOM->setPrev("e0", {{"e1", true}}));
    EXPECT_EQ(Domino::D_EVENT_FAILED_RET, PARA_DOM->setPrev("e1", {{"e0", true}})) << "REQ: can't deep loop";
}
TYPED_TEST_P(DominoTest, invalid_deeperLoop)
{
    EXPECT_NE(Domino::D_EVENT_FAILED_RET, PARA_DOM->setPrev("e0", {{"e1", false}}));
    EXPECT_NE(Domino::D_EVENT_FAILED_RET, PARA_DOM->setPrev("e1", {{"e2", false}}));
    EXPECT_EQ(Domino::D_EVENT_FAILED_RET, PARA_DOM->setPrev("e2", {{"e0", false}})) << "REQ: can't deeper loop";
}
TYPED_TEST_P(DominoTest, invalid_mixLoop)
{
    EXPECT_NE(Domino::D_EVENT_FAILED_RET, PARA_DOM->setPrev("e0", {{"e1", false}}));
    EXPECT_EQ(Domino::D_EVENT_FAILED_RET, PARA_DOM->setPrev("e1", {{"e0", true}})) << "REQ: can't T/F mix loop";
}
TYPED_TEST_P(DominoTest, strangeLoop_prevBothTrueAndFalse)
{
    // e11 <- (T) <- e10
    //    \         /
    //     <- (F) <-
    auto e10 = PARA_DOM->setPrev("e10", {{"e11", true }});
    PARA_DOM->setPrev("e10", {{"e11", false}});
    EXPECT_EQ("e11==false", PARA_DOM->whyFalse(e10)) << "REQ: simply found the root cause";
    PARA_DOM->setState({{"e11", true}});
    EXPECT_EQ("e11==true",  PARA_DOM->whyFalse(e10)) << "REQ: simply found the root cause";

    // e21 <- (F) <- e20
    // (T)\         /(T)
    //     <- e22 <-
    auto e20 = PARA_DOM->setPrev("e20", {{"e21", false}, {"e22", true}});
    PARA_DOM->setPrev("e22", {{"e21", true}});
    EXPECT_EQ("e21==false", PARA_DOM->whyFalse(e20)) << "REQ: simply found the futhest root cause";
    PARA_DOM->setState({{"e21", true}});
    EXPECT_EQ("e21==true",  PARA_DOM->whyFalse(e20)) << "REQ: simply found the futhest root cause";

    // e31 <--------------- (F) <- e30
    // (F)\                       /(F)
    //     <- e33 <- (T) <- e32 <-
    PARA_DOM->setPrev("e33", {{"e31", false}});
    PARA_DOM->setPrev("e32", {{"e33", true}});
    auto e30 = PARA_DOM->setPrev("e30", {{"e31", false}, {"e32", false}});
    EXPECT_EQ("e31==false", PARA_DOM->whyFalse(e30)) << "REQ: simply found the futhest root cause";
    PARA_DOM->setState({{"e31", true}});
    EXPECT_EQ("e31==true",  PARA_DOM->whyFalse(e30)) << "REQ: simply found the futhest root cause";

    // - this kind of loop can be very long & complex (much more than above examples)
    //   . when occur, the end-event can't be satisfied forever (user's fault, not Domino)
    // - not find a simple way (reasonable cost-benefit) to prevent it
    //   . whyFalse() is simple to detect it (but not prevent so not perfect)
    // - so setPrev() is safe in 2/3 cases but NOT in 1/3 cases
}

#define WHY_FALSE
// ***********************************************************************************************
// req: SmodAgent's file-need-check is based on 200+ files, too hard to debug why decide not to rom/plan
// - ask Domino to find 1 prev-event
//   . so SmodAgent can log_ << PARA_DOM.whyFalse(EnSmod_IS_FNC_TO_ROM_PLAN)
// ***********************************************************************************************
TYPED_TEST_P(DominoTest, GOLD_multi_retOne)
{
    auto master = PARA_DOM->setPrev("master succ", {{"all agents succ", true}, {"user abort", false}});
    EXPECT_EQ("all agents succ==false", PARA_DOM->whyFalse(master)) << "REQ: get 1 root cause";

    PARA_DOM->setState({{"all agents succ", true}, {"user abort", true}});  // REQ: simultaneous state
    EXPECT_EQ("user abort==true", PARA_DOM->whyFalse(master)) << "REQ: get diff root cause";
}
TYPED_TEST_P(DominoTest, trueEvent_retEmpty)
{
    auto master = PARA_DOM->setPrev("master succ", {{"all agents succ", true}, {"user abort", false}});
    PARA_DOM->setState({{"all agents succ", true}});
    EXPECT_TRUE(PARA_DOM->state("master succ"));
    EXPECT_EQ("[Dom Reserved EvName] whyFalse() found nothing", PARA_DOM->whyFalse(master));
}
TYPED_TEST_P(DominoTest, eventWithoutPrev_retSelf)
{
    auto no_prev_ev = PARA_DOM->newEvent("no prev ev");
    EXPECT_FALSE(PARA_DOM->state("no prev ev"));
    EXPECT_EQ("no prev ev==false", PARA_DOM->whyFalse(no_prev_ev));
}
TYPED_TEST_P(DominoTest, invalidEvent_retEmpty)
{
    EXPECT_EQ("[Dom Reserved EvName] whyFalse() found nothing", PARA_DOM->whyFalse(Domino::D_EVENT_FAILED_RET));
    EXPECT_EQ("[Dom Reserved EvName] whyFalse() found nothing", PARA_DOM->whyFalse(0));
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

    , invalid_loopSelf
    , invalid_deepLoop
    , invalid_deeperLoop
    , invalid_mixLoop
    , strangeLoop_prevBothTrueAndFalse

    , GOLD_multi_retOne
    , trueEvent_retEmpty
    , eventWithoutPrev_retSelf
    , invalidEvent_retEmpty

    , search_partial_evName

    , nonConstInterface_shall_createUnExistEvent_withStateFalse
    , noID_for_not_exist_EvName
);
using AnyDom = Types<Domino, MinDatDom, MinWbasicDatDom, MinHdlrDom, MinMhdlrDom, MinPriDom,
    MinFreeDom, MinRmEvDom, MaxNofreeDom, MaxDom>;
INSTANTIATE_TYPED_TEST_SUITE_P(PARA, DominoTest, AnyDom);
}  // namespace
