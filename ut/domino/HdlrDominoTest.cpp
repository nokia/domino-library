/**
 * Copyright 2020 Nokia. All rights reserved.
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <set>

#include "UtInitObjAnywhere.hpp"

using namespace testing;

namespace RLib
{
// ***********************************************************************************************
template<class aParaDom>
struct HdlrDominoTest : public Test, public UniLog
{
    HdlrDominoTest()
        : UniLog(UnitTest::GetInstance()->current_test_info()->name())
        , utInit_(uniLogName())
        , hdlr0_([this](){ this->hdlr0(); })
        , hdlr1_([this](){ this->hdlr1(); })
        , hdlr2_([this](){ this->hdlr2(); })
    {}
    ~HdlrDominoTest() { GTEST_LOG_FAIL }

    MOCK_METHOD0(hdlr0, void());
    MOCK_METHOD0(hdlr1, void());
    MOCK_METHOD0(hdlr2, void());

    // -------------------------------------------------------------------------------------------
    UtInitObjAnywhere utInit_;
    LoopBackFUNC loopbackFunc_;
    MsgCB hdlr0_;
    MsgCB hdlr1_;
    MsgCB hdlr2_;

    std::set<Domino::Event> uniqueEVs_;
};
TYPED_TEST_SUITE_P(HdlrDominoTest);

// ***********************************************************************************************
template<class aParaDom> using NofreeHdlrDominoTest = HdlrDominoTest<aParaDom>;  // for no-free testcase
TYPED_TEST_SUITE_P(NofreeHdlrDominoTest);

#define ADD_AND_CALL
// ***********************************************************************************************
// add & call hdlr
// ***********************************************************************************************
TYPED_TEST_P(HdlrDominoTest, GOLD_addHdlr_ok)
{
    PARA_DOM->setHdlr("event", this->hdlr0_);

    EXPECT_CALL(*this, hdlr0()).Times(1);      // req: added & called
    PARA_DOM->setState({{"event", true}});
}
TYPED_TEST_P(HdlrDominoTest, dupAdd_nok)
{
    PARA_DOM->setHdlr("event", this->hdlr0_);
    PARA_DOM->setHdlr("event", this->hdlr1_);  // repeat

    EXPECT_CALL(*this, hdlr0()).Times(1);
    EXPECT_CALL(*this, hdlr1()).Times(0);      // req: fail repeat
    PARA_DOM->setState({{"event", true}});
}
// ***********************************************************************************************
// special call hdlr
// ***********************************************************************************************
TYPED_TEST_P(HdlrDominoTest, immediateCallback_ok)
{
    EXPECT_CALL(*this, hdlr0()).Times(1);
    PARA_DOM->setState({{"event", true}});
    PARA_DOM->setHdlr("event", this->hdlr0_);  // req: immediate call
}
TYPED_TEST_P(NofreeHdlrDominoTest, GOLD_trigger_callback_reTrigger_reCallback)
{
    PARA_DOM->setHdlr("event", this->hdlr0_);
    EXPECT_CALL(*this, hdlr0()).Times(1);
    PARA_DOM->setState({{"event", true}});     // 1st cb

    EXPECT_CALL(*this, hdlr1()).Times(0);      // req: T->T no call (only F->T will call)
    PARA_DOM->setState({{"event", true}});

    PARA_DOM->setState({{"event", false}});
    EXPECT_CALL(*this, hdlr0()).Times(1);
    PARA_DOM->setState({{"event", true}});     // req: repeat cb
}
TYPED_TEST_P(NofreeHdlrDominoTest, GOLD_trigger_reTrigger_callback_reCallback)
{
    // not auto-cb but manually
    auto msgSelf = std::make_shared<MsgSelf>(
        [this](LoopBackFUNC aFunc){ this->loopbackFunc_ = aFunc; }, this->uniLogName());
    PARA_DOM->setMsgSelf(msgSelf);          // req: change MsgSelf

    PARA_DOM->setHdlr("event", this->hdlr0_);
    EXPECT_CALL(*this, hdlr0()).Times(0);
    PARA_DOM->setState({{"event", true}});  // 1st on road

    EXPECT_CALL(*this, hdlr0()).Times(0);
    PARA_DOM->setState({{"event", false}});
    PARA_DOM->setState({{"event", true}});  // 2nd on road

    EXPECT_CALL(*this, hdlr0()).Times(2);
    this->loopbackFunc_();                  // manual trigger on road cb
}

#define CHAIN
// ***********************************************************************************************
// chain satisfy -> hdlr
// ***********************************************************************************************
TYPED_TEST_P(HdlrDominoTest, GOLD_hdlrInChain_callbackOk)
{
    PARA_DOM->setHdlr("event", this->hdlr0_);
    PARA_DOM->setPrev("event", {{"prev", true}});
    EXPECT_CALL(*this, hdlr0()).Times(1);           // req: call in chain
    PARA_DOM->setState({{"prev", true}});
}
TYPED_TEST_P(HdlrDominoTest, hdlrInChain_immediateCallback)
{
    PARA_DOM->setHdlr("event", this->hdlr0_);
    PARA_DOM->setState({{"prev", true}});
    EXPECT_CALL(*this, hdlr0()).Times(1);           // req: immediate call
    PARA_DOM->setPrev("event", {{"prev", true}});
}
TYPED_TEST_P(HdlrDominoTest, hdlrInChain_dupSatisfy_callbackOnce)
{
    EXPECT_CALL(*this, hdlr0()).Times(1);           // req: call once
    PARA_DOM->setHdlr("event", this->hdlr0_);
    PARA_DOM->setState({{"event", true}});          // 1st satisfy

    EXPECT_CALL(*this, hdlr0()).Times(0);           // req: no call
    PARA_DOM->setPrev("event", {{"prev", false}});  // 2nd satisfy
}
TYPED_TEST_P(HdlrDominoTest, hdlrInChain_callAllHdlrs)
{
    PARA_DOM->setPrev("event", {{"prev", true}});
    PARA_DOM->setPrev("prev", {{"prev prev", true}});
    PARA_DOM->setHdlr("prev", this->hdlr0_);
    PARA_DOM->setHdlr("event", this->hdlr0_);

    EXPECT_CALL(*this, hdlr0()).Times(2);           // req: call each
    PARA_DOM->setState({{"prev prev", true}});
}
TYPED_TEST_P(HdlrDominoTest, GOLD_hdlrInChain_simultaneousPrevStates_rightCallback)
{
    EXPECT_CALL(*this, hdlr0()).Times(0);
    EXPECT_CALL(*this, hdlr1()).Times(1);

    PARA_DOM->setHdlr("event1", this->hdlr0_);
    PARA_DOM->setPrev("event1", {{"prev1", true}});
    PARA_DOM->setPrev("event1", {{"prev2", false}});

    PARA_DOM->setHdlr("event2", this->hdlr1_);
    PARA_DOM->setPrev("event2", {{"prev1", true}});
    PARA_DOM->setPrev("event2", {{"prev2", true}});

    PARA_DOM->setState({{"prev1", true}, {"prev2", true}});
}
TYPED_TEST_P(HdlrDominoTest, hdlrInChain_wrongOrderPrev_wrongCallback)
{
    EXPECT_CALL(*this, hdlr0()).Times(0);                                                    // no callback
    PARA_DOM->setHdlr("event with simu prev setting", this->hdlr0_);
    PARA_DOM->setPrev("event with simu prev setting", {{"prev1", true}, {"prev2", false}});  // simu prev

    EXPECT_CALL(*this, hdlr0()).Times(1);                                                    // wrong callback
    PARA_DOM->setHdlr("event with wrong prev setting", this->hdlr0_);
    PARA_DOM->setPrev("event with wrong prev setting", {{"prev2", false}});                  // wrong order
    PARA_DOM->setPrev("event with wrong prev setting", {{"prev1", true}});
}

#define MULTI_HDLR
// ***********************************************************************************************
// multi hdlr by alias event
// ***********************************************************************************************
TYPED_TEST_P(HdlrDominoTest, multiHdlr_onOneEvent_nok)
{
    PARA_DOM->setHdlr("event", this->hdlr0_);
    PARA_DOM->setHdlr("event", this->hdlr1_);  // req: refuse

    EXPECT_CALL(*this, hdlr0()).Times(1);
    EXPECT_CALL(*this, hdlr1()).Times(0);
    PARA_DOM->setState({{"event", true}});
}
TYPED_TEST_P(HdlrDominoTest, GOLD_multiHdlr_onDiffEvent_ok)
{
    PARA_DOM->setHdlr("event", this->hdlr0_);
    PARA_DOM->multiHdlrByAliasEv("alias event", this->hdlr1_, "event");  // req: accept on alias ev
    PARA_DOM->multiHdlrByAliasEv("alias-2", this->hdlr2_, "event");      // req: accept on diff alias

    EXPECT_CALL(*this, hdlr0()).Times(1);
    EXPECT_CALL(*this, hdlr1()).Times(1);
    EXPECT_CALL(*this, hdlr2()).Times(1);
    PARA_DOM->setState({{"event", true}});
}
TYPED_TEST_P(HdlrDominoTest, multiHdlr_onOneAliasEvent_nok)
{
    PARA_DOM->setHdlr("event", this->hdlr0_);
    PARA_DOM->multiHdlrByAliasEv("alias event", this->hdlr1_, "event");
    PARA_DOM->multiHdlrByAliasEv("alias event", this->hdlr2_, "event");  // req: refuse

    EXPECT_CALL(*this, hdlr0()).Times(1);
    EXPECT_CALL(*this, hdlr1()).Times(1);
    EXPECT_CALL(*this, hdlr2()).Times(0);
    PARA_DOM->setState({{"event", true}});
}
TYPED_TEST_P(NofreeHdlrDominoTest, multiHdlr_hubTrigger)
{
    PARA_DOM->multiHdlrByAliasEv("e0", this->hdlr0_, "e");
    PARA_DOM->multiHdlrByAliasEv("e1", this->hdlr1_, "e");
    PARA_DOM->multiHdlrByAliasEv("e2", this->hdlr2_, "e");

    EXPECT_CALL(*this, hdlr0()).Times(1);
    EXPECT_CALL(*this, hdlr1()).Times(1);
    EXPECT_CALL(*this, hdlr2()).Times(1);
    PARA_DOM->setState({{"e", true}});      // T->(T,T,T)

    EXPECT_CALL(*this, hdlr0()).Times(0);
    EXPECT_CALL(*this, hdlr1()).Times(0);
    EXPECT_CALL(*this, hdlr2()).Times(0);
    PARA_DOM->setState({{"e2", false}});    // T->(T,T,F)

    EXPECT_CALL(*this, hdlr0()).Times(0);
    EXPECT_CALL(*this, hdlr1()).Times(0);
    EXPECT_CALL(*this, hdlr2()).Times(0);
    PARA_DOM->setState({{"e", false}});     // F->(T,T,F)

    EXPECT_CALL(*this, hdlr0()).Times(0);
    EXPECT_CALL(*this, hdlr1()).Times(0);
    EXPECT_CALL(*this, hdlr2()).Times(1);
    PARA_DOM->setState({{"e", true}});      // T->(T,T,T)
}
TYPED_TEST_P(NofreeHdlrDominoTest, multiHdlr_chainTrigger)
{
    PARA_DOM->setHdlr("e0", this->hdlr0_);
    PARA_DOM->multiHdlrByAliasEv("e1", this->hdlr1_, "e0");
    PARA_DOM->multiHdlrByAliasEv("e2", this->hdlr2_, "e1");

    EXPECT_CALL(*this, hdlr0()).Times(1);
    EXPECT_CALL(*this, hdlr1()).Times(1);
    EXPECT_CALL(*this, hdlr2()).Times(1);
    PARA_DOM->setState({{"e0", true}});     // T->T->T

    EXPECT_CALL(*this, hdlr0()).Times(0);
    EXPECT_CALL(*this, hdlr1()).Times(0);
    EXPECT_CALL(*this, hdlr2()).Times(0);
    PARA_DOM->setState({{"e2", false}});    // T->T->F

    EXPECT_CALL(*this, hdlr0()).Times(0);
    EXPECT_CALL(*this, hdlr1()).Times(0);
    EXPECT_CALL(*this, hdlr2()).Times(0);
    PARA_DOM->setState({{"e0", false}});    // F->T->F

    EXPECT_CALL(*this, hdlr0()).Times(1);
    EXPECT_CALL(*this, hdlr1()).Times(0);
    EXPECT_CALL(*this, hdlr2()).Times(1);   // req: trigger cross e1
    PARA_DOM->setState({{"e0", true}});     // T->T->T
}

#define RM_HDLR
// ***********************************************************************************************
// rm hdlr, then F->T can't cb
// ***********************************************************************************************
TYPED_TEST_P(HdlrDominoTest, rmHdlr_thenNoCallback)
{
    PARA_DOM->setHdlr("event", this->hdlr0_);
    EXPECT_TRUE(PARA_DOM->rmOneHdlrOK("event"));

    EXPECT_CALL(*this, hdlr0()).Times(0);  // req: no call since rm
    PARA_DOM->setState({{"event", true}});
}
TYPED_TEST_P(HdlrDominoTest, rmHdlr_fail)
{
    PARA_DOM->setHdlr("event", this->hdlr0_);
    EXPECT_TRUE(PARA_DOM->rmOneHdlrOK("event"));
    EXPECT_FALSE(PARA_DOM->rmOneHdlrOK("event"));            // req: rm unexist hdlr

    EXPECT_FALSE(PARA_DOM->rmOneHdlrOK("not exist event"));  // req: rm unexist ev
}
// ***********************************************************************************************
// rm on-road-hdlr
// ***********************************************************************************************
TYPED_TEST_P(HdlrDominoTest, rmHdlrOnRoad_noCallback)
{
    // not auto-cb but manually
    auto msgSelf = std::make_shared<MsgSelf>(
        [this](LoopBackFUNC aFunc){ this->loopbackFunc_ = aFunc; }, this->uniLogName());
    PARA_DOM->setMsgSelf(msgSelf);

    PARA_DOM->multiHdlrByAliasEv("e0", this->hdlr0_, "e");
    PARA_DOM->multiHdlrByAliasEv("e1", this->hdlr1_, "e");
    PARA_DOM->setState({{"e", true}});         // cb on road
    EXPECT_TRUE(msgSelf->hasMsg());
    EXPECT_TRUE(PARA_DOM->rmOneHdlrOK("e0"));  // req: rm hdlr on-road

    EXPECT_CALL(*this, hdlr0()).Times(0);
    EXPECT_CALL(*this, hdlr1()).Times(1);
    this->loopbackFunc_();                     // manual trigger on road cb
}
TYPED_TEST_P(NofreeHdlrDominoTest, rmHdlrOnRoad_thenReAdd_noCallbackUntilReTrigger)
{
    // not auto-cb but manually
    auto msgSelf = std::make_shared<MsgSelf>(
        [this](LoopBackFUNC aFunc){ this->loopbackFunc_ = aFunc; }, this->uniLogName());
    PARA_DOM->setMsgSelf(msgSelf);

    PARA_DOM->setHdlr("event", this->hdlr0_);
    PARA_DOM->setState({{"event", true}});
    EXPECT_EQ(1U, msgSelf->nMsg(EMsgPri_NORM));   // 1 cb on road

    PARA_DOM->setState({{"event", false}});
    PARA_DOM->setState({{"event", true}});
    EXPECT_EQ(2U, msgSelf->nMsg(EMsgPri_NORM));   // 2 cb on road

    EXPECT_TRUE(PARA_DOM->rmOneHdlrOK("event"));  // req: rm hdlr include all on-road

    PARA_DOM->setState({{"event", false}});       // not auto-cb
    PARA_DOM->setHdlr("event", this->hdlr0_);     // re-add hdlr

    EXPECT_CALL(*this, hdlr0()).Times(0);         // req: no cb since rm-ed
    this->loopbackFunc_();

    PARA_DOM->setState({{"event", true}});
    EXPECT_TRUE(msgSelf->hasMsg());
    EXPECT_CALL(*this, hdlr0()).Times(1);         // req: new cb
    this->loopbackFunc_();
}

#define ID_STATE
// ***********************************************************************************************
// event & EvName are ID
// ***********************************************************************************************
TYPED_TEST_P(HdlrDominoTest, GOLD_nonConstInterface_shall_createUnExistEvent_withStateFalse)
{
    PARA_DOM->setHdlr("e1", this->hdlr0_);
    PARA_DOM->setHdlr("e1", this->hdlr0_);
    this->uniqueEVs_.insert(PARA_DOM->getEventBy("e1"));
    EXPECT_EQ(1u, this->uniqueEVs_.size());
    EXPECT_FALSE(PARA_DOM->state("e1"));

    PARA_DOM->multiHdlrByAliasEv("e2", this->hdlr2_, "e3");
    PARA_DOM->multiHdlrByAliasEv("e2", this->hdlr2_, "e3");
    this->uniqueEVs_.insert(PARA_DOM->getEventBy("e2"));
    EXPECT_EQ(2u, this->uniqueEVs_.size());
    this->uniqueEVs_.insert(PARA_DOM->getEventBy("e3"));
    EXPECT_EQ(3u, this->uniqueEVs_.size());
    EXPECT_FALSE(PARA_DOM->state("e2"));
    EXPECT_FALSE(PARA_DOM->state("e3"));

    this->uniqueEVs_.insert(Domino::D_EVENT_FAILED_RET);
    EXPECT_EQ(4u, this->uniqueEVs_.size());

    PARA_DOM->rmOneHdlrOK("e4");  // shall NOT generate new event
    this->uniqueEVs_.insert(PARA_DOM->getEventBy("e4"));
    EXPECT_EQ(4u, this->uniqueEVs_.size());
}

// ***********************************************************************************************
REGISTER_TYPED_TEST_SUITE_P(HdlrDominoTest
    , GOLD_addHdlr_ok
    , dupAdd_nok
    , GOLD_hdlrInChain_callbackOk
    , hdlrInChain_callAllHdlrs
    , hdlrInChain_dupSatisfy_callbackOnce
    , hdlrInChain_immediateCallback
    , GOLD_hdlrInChain_simultaneousPrevStates_rightCallback
    , hdlrInChain_wrongOrderPrev_wrongCallback
    , immediateCallback_ok
    , GOLD_multiHdlr_onDiffEvent_ok
    , multiHdlr_onOneAliasEvent_nok
    , multiHdlr_onOneEvent_nok
    , rmHdlr_thenNoCallback
    , rmHdlr_fail
    , rmHdlrOnRoad_noCallback
    , GOLD_nonConstInterface_shall_createUnExistEvent_withStateFalse
);
using AnyHdlrDom = Types<MinHdlrDom, MinMhdlrDom, MinFreeDom, MinPriDom, MaxNofreeDom, MaxDom>;
INSTANTIATE_TYPED_TEST_SUITE_P(PARA, HdlrDominoTest, AnyHdlrDom);

// ***********************************************************************************************
REGISTER_TYPED_TEST_SUITE_P(NofreeHdlrDominoTest
    , rmHdlrOnRoad_thenReAdd_noCallbackUntilReTrigger
    , multiHdlr_hubTrigger
    , multiHdlr_chainTrigger
    , GOLD_trigger_callback_reTrigger_reCallback
    , GOLD_trigger_reTrigger_callback_reCallback
);
using AnyNofreeHdlrDom = Types<MinHdlrDom, MinMhdlrDom, MinPriDom, MaxNofreeDom>;
INSTANTIATE_TYPED_TEST_SUITE_P(PARA, NofreeHdlrDominoTest, AnyNofreeHdlrDom);
}  // namespace
