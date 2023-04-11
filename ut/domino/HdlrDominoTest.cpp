/**
 * Copyright 2020 Nokia. All rights reserved.
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include <gmock/gmock.h>
#include <set>

#include "UtInitObjAnywhere.hpp"

namespace RLib
{
// ***********************************************************************************************
template<class aParaDom>
struct HdlrDominoTest : public UtInitObjAnywhere
{
    MOCK_METHOD(void, hdlr0, ());
    MOCK_METHOD(void, hdlr1, ());
    MOCK_METHOD(void, hdlr2, ());

    // -------------------------------------------------------------------------------------------
    FromMainFN fromMainFN_;
    MsgCB hdlr0_ = [this](){ this->hdlr0(); };
    MsgCB hdlr1_ = [this](){ this->hdlr1(); };
    MsgCB hdlr2_ = [this](){ this->hdlr2(); };

    set<Domino::Event> uniqueEVs_;
};
TYPED_TEST_SUITE_P(HdlrDominoTest);

// ***********************************************************************************************
template<class aParaDom> using NofreeHdlrDominoTest = HdlrDominoTest<aParaDom>;  // for no-free testcase
TYPED_TEST_SUITE_P(NofreeHdlrDominoTest);

#define ADD_AND_CALL
// ***********************************************************************************************
// add & call hdlr
// ***********************************************************************************************
TYPED_TEST_P(HdlrDominoTest, GOLD_add_and_call)
{
    PARA_DOM->setHdlr("event", this->hdlr0_);  // req: add hdlr
    EXPECT_CALL(*this, hdlr0());
    PARA_DOM->setState({{"event", true}});     // req: F->T trigger the call

    EXPECT_CALL(*this, hdlr1()).Times(0);      // req: T->T no call (only F->T is trigger)
    PARA_DOM->setState({{"event", true}});

    EXPECT_CALL(*this, hdlr1()).Times(0);      // req: T->F no call
    PARA_DOM->setState({{"event", false}});

    EXPECT_CALL(*this, hdlr1()).Times(0);      // req: F->F no call
    PARA_DOM->setState({{"event", false}});
}
TYPED_TEST_P(HdlrDominoTest, immediate_call)
{
    PARA_DOM->setState({{"event", true}});
    EXPECT_CALL(*this, hdlr0());
    PARA_DOM->setHdlr("event", this->hdlr0_);  // req: immediate call since already T from default (always=F)
}
TYPED_TEST_P(NofreeHdlrDominoTest, UC_reTrigger_reCall)
{
    PARA_DOM->setHdlr("event", this->hdlr0_);
    EXPECT_CALL(*this, hdlr0()).Times(2);
    PARA_DOM->setState({{"event", true}});     // 1st trigger
    PARA_DOM->setState({{"event", false}});
    PARA_DOM->setState({{"event", true}});     // 2nd trigger
}

#define CHAIN
// ***********************************************************************************************
// chain satisfy -> hdlr
// ***********************************************************************************************
TYPED_TEST_P(HdlrDominoTest, GOLD_trigger_chain_call)
{
    PARA_DOM->setHdlr("event", this->hdlr0_);      // downstream
    PARA_DOM->setPrev("event", {{"prev", true}});  // upstream

    EXPECT_CALL(*this, hdlr0());  // req: upstream trigger downstream call
    PARA_DOM->setState({{"prev", true}});
}
TYPED_TEST_P(HdlrDominoTest, immediate_chain_call)
{
    PARA_DOM->setHdlr("event", this->hdlr0_);
    PARA_DOM->setState({{"prev", true}});
    EXPECT_CALL(*this, hdlr0());  // req: immediate call
    PARA_DOM->setPrev("event", {{"prev", true}});
}
TYPED_TEST_P(HdlrDominoTest, GOLD_trigger_chain_allCall)
{
    PARA_DOM->setPrev("event", {{"prev", true}});
    PARA_DOM->setPrev("prev", {{"prev prev", true}});
    PARA_DOM->setHdlr("prev", this->hdlr0_);
    PARA_DOM->setHdlr("event", this->hdlr1_);

    EXPECT_CALL(*this, hdlr0());  // req: trigger each hdlr on the chain if F->T
    EXPECT_CALL(*this, hdlr1());  // req: trigger each hdlr on the chain if F->T
    PARA_DOM->setState({{"prev prev", true}});
}
TYPED_TEST_P(HdlrDominoTest, GOLD_trigger_chain_satisfiedCall)
{
    PARA_DOM->setHdlr("event1", this->hdlr0_);
    PARA_DOM->setPrev("event1", {{"prev1", true}, {"prev2", false}});

    PARA_DOM->setHdlr("event2", this->hdlr1_);
    PARA_DOM->setPrev("event2", {{"prev1", true}, {"prev2", true}});

    EXPECT_CALL(*this, hdlr0()).Times(0);
    EXPECT_CALL(*this, hdlr1());
    PARA_DOM->setState({{"prev1", true}, {"prev2", true}});
}
TYPED_TEST_P(HdlrDominoTest, wrongOrderPrev_wrongCallback)
{
    EXPECT_CALL(*this, hdlr0()).Times(0);                                                    // no callback
    PARA_DOM->setHdlr("event with simu prev setting", this->hdlr0_);
    PARA_DOM->setPrev("event with simu prev setting", {{"prev1", true}, {"prev2", false}});  // simu prev

    EXPECT_CALL(*this, hdlr0());                                                             // wrong callback
    PARA_DOM->setHdlr("event with wrong prev setting", this->hdlr0_);
    PARA_DOM->setPrev("event with wrong prev setting", {{"prev2", false}});                  // wrong order
    PARA_DOM->setPrev("event with wrong prev setting", {{"prev1", true}});
}

#define MULTI_HDLR  // multi hdlr by alias event
// ***********************************************************************************************
TYPED_TEST_P(HdlrDominoTest, multiHdlr_onOneEvent_nok)
{
    PARA_DOM->setHdlr("event", this->hdlr0_);
    PARA_DOM->setHdlr("event", this->hdlr1_);  // req: refuse

    EXPECT_CALL(*this, hdlr0());
    EXPECT_CALL(*this, hdlr1()).Times(0);
    PARA_DOM->setState({{"event", true}});
}
TYPED_TEST_P(HdlrDominoTest, GOLD_multiHdlr_onDiffEvent_ok)
{
    PARA_DOM->setHdlr("event", this->hdlr0_);
    PARA_DOM->multiHdlrByAliasEv("alias event", this->hdlr1_, "event");  // req: accept on alias ev
    PARA_DOM->multiHdlrByAliasEv("alias-2", this->hdlr2_, "event");      // req: accept on diff alias

    EXPECT_CALL(*this, hdlr0());
    EXPECT_CALL(*this, hdlr1());
    EXPECT_CALL(*this, hdlr2());
    PARA_DOM->setState({{"event", true}});
}
TYPED_TEST_P(HdlrDominoTest, multiHdlr_onOneAliasEvent_nok)
{
    PARA_DOM->setHdlr("event", this->hdlr0_);
    PARA_DOM->multiHdlrByAliasEv("alias event", this->hdlr1_, "event");
    PARA_DOM->multiHdlrByAliasEv("alias event", this->hdlr2_, "event");  // req: refuse

    EXPECT_CALL(*this, hdlr0());
    EXPECT_CALL(*this, hdlr1());
    EXPECT_CALL(*this, hdlr2()).Times(0);
    PARA_DOM->setState({{"event", true}});
}
TYPED_TEST_P(NofreeHdlrDominoTest, multiHdlr_chainTrigger)
{
    PARA_DOM->setHdlr("e0", this->hdlr0_);
    PARA_DOM->multiHdlrByAliasEv("e1", this->hdlr1_, "e0");
    PARA_DOM->multiHdlrByAliasEv("e2", this->hdlr2_, "e1");

    EXPECT_CALL(*this, hdlr0());
    EXPECT_CALL(*this, hdlr1());
    EXPECT_CALL(*this, hdlr2());
    PARA_DOM->setState({{"e0", true}});     // T->T->T

    EXPECT_CALL(*this, hdlr0()).Times(0);
    EXPECT_CALL(*this, hdlr1()).Times(0);
    EXPECT_CALL(*this, hdlr2()).Times(0);
    PARA_DOM->setState({{"e2", false}});    // T,T,T->F

    EXPECT_CALL(*this, hdlr0()).Times(0);
    EXPECT_CALL(*this, hdlr1()).Times(0);
    EXPECT_CALL(*this, hdlr2()).Times(0);
    PARA_DOM->setState({{"e0", false}});    // F->T,T,F

    EXPECT_CALL(*this, hdlr0());
    EXPECT_CALL(*this, hdlr1()).Times(0);
    EXPECT_CALL(*this, hdlr2());            // req: trigger cross e1
    PARA_DOM->setState({{"e0", true}});     // T->T->T
}
TYPED_TEST_P(HdlrDominoTest, BugFix_invalidHdlr_noCrash)
{
    PARA_DOM->setHdlr("e1", nullptr);
    EXPECT_EQ(Domino::D_EVENT_FAILED_RET, PARA_DOM->getEventBy("e1"));  // req: not create new Ev

    PARA_DOM->multiHdlrByAliasEv("alias e1", nullptr, "e1");
    EXPECT_EQ(Domino::D_EVENT_FAILED_RET, PARA_DOM->getEventBy("alias e1"));  // req: not create new Ev

    PARA_DOM->setState({{"e1", true}});  // req: no crash

    EXPECT_CALL(*this, hdlr0());  // req: can add hdlr upon null
    PARA_DOM->setHdlr("e1", this->hdlr0_);

    EXPECT_CALL(*this, hdlr1());  // req: can add hdlr upon null
    PARA_DOM->multiHdlrByAliasEv("alias e1", this->hdlr1_, "e1");
}

#define RM_HDLR
// ***********************************************************************************************
// rm hdlr, then F->T can't cb
// ***********************************************************************************************
TYPED_TEST_P(HdlrDominoTest, rmHdlr_thenNoCallback)
{
    PARA_DOM->setHdlr("event", this->hdlr0_);
    ASSERT_TRUE(PARA_DOM->rmOneHdlrOK("event"));

    EXPECT_CALL(*this, hdlr0()).Times(0);  // req: no call since rm
    PARA_DOM->setState({{"event", true}});
}
TYPED_TEST_P(HdlrDominoTest, rmHdlr_fail)
{
    EXPECT_FALSE(PARA_DOM->rmOneHdlrOK(Domino::D_EVENT_FAILED_RET, nullptr));  // req: rm null hdlr

    PARA_DOM->setHdlr("event", this->hdlr0_);
    EXPECT_TRUE(PARA_DOM->rmOneHdlrOK("event"));
    EXPECT_FALSE(PARA_DOM->rmOneHdlrOK("event"));  // req: rm unexist hdlr

    EXPECT_FALSE(PARA_DOM->rmOneHdlrOK("not exist event"));  // req: rm unexist ev

    EXPECT_FALSE(PARA_DOM->rmOneHdlrOK(PARA_DOM->getEventBy("event"), nullptr));  // req: rm null hdlr
}
// ***********************************************************************************************
// rm on-road-hdlr
// ***********************************************************************************************
TYPED_TEST_P(HdlrDominoTest, rmHdlrOnRoad_noCallback)
{
    // not auto-cb but manually
    auto msgSelf = make_shared<MsgSelf>(
        [this](const FromMainFN& aFromMainFN){ this->fromMainFN_ = aFromMainFN; }, this->uniLogName());
    PARA_DOM->setMsgSelf(msgSelf);

    PARA_DOM->multiHdlrByAliasEv("e0", this->hdlr0_, "e");
    PARA_DOM->multiHdlrByAliasEv("e1", this->hdlr1_, "e");
    PARA_DOM->setState({{"e", true}});            // cb on road
    EXPECT_TRUE(msgSelf->hasMsg());
    EXPECT_TRUE(PARA_DOM->rmOneHdlrOK("e0"));     // req: rm hdlr on-road

    EXPECT_CALL(*this, hdlr0()).Times(0);
    EXPECT_CALL(*this, hdlr1());
    this->fromMainFN_();                          // manual trigger on road cb
}
TYPED_TEST_P(NofreeHdlrDominoTest, rmHdlrOnRoad_thenReAdd_noCallbackUntilReTrigger)
{
    // not auto-cb but manually
    auto msgSelf = make_shared<MsgSelf>(
        [this](const FromMainFN& aFromMainFN){ this->fromMainFN_ = aFromMainFN; }, this->uniLogName());
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
    this->fromMainFN_();

    PARA_DOM->setState({{"event", true}});
    ASSERT_TRUE(msgSelf->hasMsg());
    EXPECT_CALL(*this, hdlr0());                  // req: new cb
    this->fromMainFN_();
}
TYPED_TEST_P(NofreeHdlrDominoTest, hdlrOnRoad_thenRmDom_noCrash_noLeak)
{
    // not auto-cb but manually
    auto msgSelf = make_shared<MsgSelf>(
        [this](const FromMainFN& aFromMainFN){ this->fromMainFN_ = aFromMainFN; }, this->uniLogName());
    PARA_DOM->setMsgSelf(msgSelf);

    PARA_DOM->setHdlr("event", this->hdlr0_);
    PARA_DOM->setState({{"event", true}});
    EXPECT_EQ(1U, msgSelf->nMsg(EMsgPri_NORM));   // 1 cb on road

    ObjAnywhere::set<TypeParam>(nullptr, *this);  // rm dom
    EXPECT_CALL(*this, hdlr0()).Times(0);         // req: no cb
    this->fromMainFN_();
}

#define ID_STATE
// ***********************************************************************************************
// event & EvName are ID
// ***********************************************************************************************
TYPED_TEST_P(HdlrDominoTest, nonConstInterface_shall_createUnExistEvent_withStateFalse)
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
    , GOLD_add_and_call
    , immediate_call

    , GOLD_trigger_chain_call
    , immediate_chain_call
    , GOLD_trigger_chain_allCall
    , GOLD_trigger_chain_satisfiedCall
    , wrongOrderPrev_wrongCallback

    , multiHdlr_onOneEvent_nok
    , GOLD_multiHdlr_onDiffEvent_ok
    , multiHdlr_onOneAliasEvent_nok
    , BugFix_invalidHdlr_noCrash

    , rmHdlr_thenNoCallback
    , rmHdlr_fail
    , rmHdlrOnRoad_noCallback

    , nonConstInterface_shall_createUnExistEvent_withStateFalse
);
using AnyHdlrDom = Types<MinHdlrDom, MinMhdlrDom, MinFreeDom, MinPriDom, MaxNofreeDom, MaxDom>;
INSTANTIATE_TYPED_TEST_SUITE_P(PARA, HdlrDominoTest, AnyHdlrDom);

// ***********************************************************************************************
REGISTER_TYPED_TEST_SUITE_P(NofreeHdlrDominoTest
    , UC_reTrigger_reCall

    , multiHdlr_chainTrigger

    , rmHdlrOnRoad_thenReAdd_noCallbackUntilReTrigger
    , hdlrOnRoad_thenRmDom_noCrash_noLeak
);
using AnyNofreeHdlrDom = Types<MinHdlrDom, MinMhdlrDom, MinPriDom, MaxNofreeDom>;
INSTANTIATE_TYPED_TEST_SUITE_P(PARA, NofreeHdlrDominoTest, AnyNofreeHdlrDom);
}  // namespace
