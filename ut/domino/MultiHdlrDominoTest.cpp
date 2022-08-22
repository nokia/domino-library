/**
 * Copyright 2020-2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>  // for shared_ptr
#include <set>

#include "UtInitObjAnywhere.hpp"

using namespace testing;

namespace RLib
{
// ***********************************************************************************************
template<class aParaDom>
struct MultiHdlrDominoTest : public Test, public CellLog
{
    MultiHdlrDominoTest()
        : hdlr0_([this](){ this->hdlr0(); })
        , hdlr1_([this](){ this->hdlr1(); })
        , hdlr2_([this](){ this->hdlr2(); })
    {}
    ~MultiHdlrDominoTest() { GTEST_LOG_FAIL }

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
TYPED_TEST_SUITE_P(MultiHdlrDominoTest);

// ***********************************************************************************************
template<class aParaDom> using NofreeMultiHdlrDominoTest = MultiHdlrDominoTest<aParaDom>;  // for no-free testcase
TYPED_TEST_SUITE_P(NofreeMultiHdlrDominoTest);

#define ADD_AND_CALL
// ***********************************************************************************************
// add hdlr & normal call
// ***********************************************************************************************
TYPED_TEST_P(MultiHdlrDominoTest, GOLD_multiAddHdlr_ok)
{
    PARA_DOM->setHdlr("event", this->hdlr0_);
    PARA_DOM->multiHdlrOnSameEv("event", this->hdlr1_, "this->hdlr1_");
    PARA_DOM->multiHdlrOnSameEv("event", this->hdlr2_, "this->hdlr2_");

    EXPECT_CALL(*this, hdlr0()).Times(1);  // req: legacy
    EXPECT_CALL(*this, hdlr1()).Times(1);  // req: multi-hdlr
    EXPECT_CALL(*this, hdlr2()).Times(1);  // req: multi-hdlr
    PARA_DOM->setState({{"event", true}});
}
TYPED_TEST_P(MultiHdlrDominoTest, aloneAddHdlr_ok)
{
    PARA_DOM->multiHdlrOnSameEv("event", this->hdlr1_, "this->hdlr1_");  // req: alone add & call

    EXPECT_CALL(*this, hdlr1()).Times(1);
    PARA_DOM->setState({{"event", true}});
}
TYPED_TEST_P(MultiHdlrDominoTest, multiAddHdlr_bySameHdlrName_nok)
{
    PARA_DOM->multiHdlrOnSameEv("event", this->hdlr1_, "this->hdlr1_");
    PARA_DOM->multiHdlrOnSameEv("event", this->hdlr2_, "this->hdlr1_");  // req: same HdlrName -> fail
    EXPECT_CALL(*this, hdlr1()).Times(1);
    EXPECT_CALL(*this, hdlr2()).Times(0);
    PARA_DOM->setState({{"event", true}});
}
// ***********************************************************************************************
// special call hdlr
// ***********************************************************************************************
TYPED_TEST_P(MultiHdlrDominoTest, immediateCallback_ok)
{
    EXPECT_CALL(*this, hdlr1()).Times(1);
    PARA_DOM->setState({{"event", true}});
    PARA_DOM->multiHdlrOnSameEv("event", this->hdlr1_, "this->hdlr1_");  // req: immediate call
}
TYPED_TEST_P(NofreeMultiHdlrDominoTest, repeatCallback_ok)
{
    PARA_DOM->setHdlr("event", this->hdlr0_);
    PARA_DOM->multiHdlrOnSameEv("event", this->hdlr1_, "this->hdlr1_");

    EXPECT_CALL(*this, hdlr0()).Times(1);
    EXPECT_CALL(*this, hdlr1()).Times(1);
    PARA_DOM->setState({{"event", true}});  // req: samultaneous call

    PARA_DOM->setState({{"event", false}});
    EXPECT_CALL(*this, hdlr0()).Times(1);
    EXPECT_CALL(*this, hdlr1()).Times(1);
    PARA_DOM->setState({{"event", true}});  // req: repeat call
}

#define CHAIN
// ***********************************************************************************************
// chain satisfy -> hdlr
// ***********************************************************************************************
TYPED_TEST_P(MultiHdlrDominoTest, chain_callbackAllHdlr)
{
    PARA_DOM->setHdlr("event", this->hdlr0_);
    PARA_DOM->multiHdlrOnSameEv("event", this->hdlr1_, "this->hdlr1_");
    PARA_DOM->multiHdlrOnSameEv("event", this->hdlr2_, "this->hdlr2_");
    PARA_DOM->setPrev("event", {{"prev", true}});

    EXPECT_CALL(*this, hdlr0()).Times(1);
    EXPECT_CALL(*this, hdlr1()).Times(1);
    EXPECT_CALL(*this, hdlr2()).Times(1);
    PARA_DOM->setState({{"prev", true}});
}
TYPED_TEST_P(MultiHdlrDominoTest, newChain_immediateCallbackAll)
{
    PARA_DOM->setHdlr("event", this->hdlr0_);
    PARA_DOM->multiHdlrOnSameEv("event", this->hdlr1_, "this->hdlr1_");
    PARA_DOM->multiHdlrOnSameEv("event", this->hdlr2_, "this->hdlr2_");
    PARA_DOM->setState({{"prev", true}});

    EXPECT_CALL(*this, hdlr0()).Times(1);
    EXPECT_CALL(*this, hdlr1()).Times(1);
    EXPECT_CALL(*this, hdlr2()).Times(1);
    PARA_DOM->setPrev("event", {{"prev", true}});  // new chain
}
TYPED_TEST_P(MultiHdlrDominoTest, newChain_dupSatisfy_callbackOnce)
{
    EXPECT_CALL(*this, hdlr0()).Times(1);
    EXPECT_CALL(*this, hdlr1()).Times(1);
    EXPECT_CALL(*this, hdlr2()).Times(1);
    PARA_DOM->setHdlr("event", this->hdlr0_);
    PARA_DOM->multiHdlrOnSameEv("event", this->hdlr1_, "this->hdlr1_");
    PARA_DOM->multiHdlrOnSameEv("event", this->hdlr2_, "this->hdlr2_");
    PARA_DOM->setState({{"event", true}});         // 1st satisfy

    PARA_DOM->setState({{"prev", true}});
    PARA_DOM->setPrev("event", {{"prev", true}});  // 2nd satisfy
}
TYPED_TEST_P(MultiHdlrDominoTest, chain_sameHdlrOnDiffEvent_callbackEach)
{
    PARA_DOM->setPrev("event", {{"prev", true}});
    PARA_DOM->setPrev("prev", {{"prev prev", true}});
    PARA_DOM->setHdlr("prev", this->hdlr0_);
    PARA_DOM->setHdlr("event", this->hdlr0_);
    PARA_DOM->multiHdlrOnSameEv("prev", this->hdlr1_, "this->hdlr1_");
    PARA_DOM->multiHdlrOnSameEv("event", this->hdlr1_, "this->hdlr1_");
    PARA_DOM->multiHdlrOnSameEv("prev", this->hdlr2_, "this->hdlr2_");
    PARA_DOM->multiHdlrOnSameEv("event", this->hdlr2_, "this->hdlr2_");

    EXPECT_CALL(*this, hdlr0()).Times(2);
    EXPECT_CALL(*this, hdlr1()).Times(2);
    EXPECT_CALL(*this, hdlr2()).Times(2);
    PARA_DOM->setState({{"prev prev", true}});
}
TYPED_TEST_P(MultiHdlrDominoTest, multiPrev_onlyAllSatisfy_thenCallback)
{
    EXPECT_CALL(*this, hdlr0()).Times(0);
    EXPECT_CALL(*this, hdlr1()).Times(3);

    PARA_DOM->setHdlr("event1", this->hdlr0_);
    PARA_DOM->multiHdlrOnSameEv("event1", this->hdlr0_, "hn0_1");
    PARA_DOM->multiHdlrOnSameEv("event1", this->hdlr0_, "hn0_2");
    PARA_DOM->setPrev("event1", {{"prev1", true}});
    PARA_DOM->setPrev("event1", {{"prev2", false}});

    PARA_DOM->setHdlr("event2", this->hdlr1_);
    PARA_DOM->multiHdlrOnSameEv("event2", this->hdlr1_, "hn1_1");
    PARA_DOM->multiHdlrOnSameEv("event2", this->hdlr1_, "hn1_2");
    PARA_DOM->setPrev("event2", {{"prev1", true}});
    PARA_DOM->setPrev("event2", {{"prev2", true}});

    PARA_DOM->setState({{"prev1", true}, {"prev2", true}});
}

#define RM_HDLR
// ***********************************************************************************************
// rm hdlr, then F->T can't cb
// ***********************************************************************************************
TYPED_TEST_P(MultiHdlrDominoTest, rmHdlr_byHdlrName)
{
    PARA_DOM->setHdlr("event", this->hdlr0_);
    PARA_DOM->multiHdlrOnSameEv("event", this->hdlr1_, "this->hdlr1_");
    EXPECT_TRUE(PARA_DOM->rmOneHdlrOK("event", "this->hdlr1_"));        // req: rm MultiHdlrDomino

    EXPECT_CALL(*this, hdlr0()).Times(1);
    EXPECT_CALL(*this, hdlr1()).Times(0);
    PARA_DOM->setState({{"event", true}});

    EXPECT_FALSE(PARA_DOM->rmOneHdlrOK("event", "this->hdlr1_"));       // req: rm unexist hdlr
    EXPECT_FALSE(PARA_DOM->rmOneHdlrOK("unexist ev", "this->hdlr1_"));  // req: rm unexist ev
}
TYPED_TEST_P(MultiHdlrDominoTest, rmLegacyHdlr_byNoHdlrName)
{
    PARA_DOM->setHdlr("event", this->hdlr0_);
    PARA_DOM->multiHdlrOnSameEv("event", this->hdlr1_, "this->hdlr1_");
    EXPECT_TRUE(PARA_DOM->rmOneHdlrOK("event"));  // req: rm HdlrDomino that has no hdlr name

    EXPECT_CALL(*this, hdlr0()).Times(0);
    EXPECT_CALL(*this, hdlr1()).Times(1);
    PARA_DOM->setState({{"event", true}});
}
// ***********************************************************************************************
// rm on-road-hdlr
// ***********************************************************************************************
TYPED_TEST_P(NofreeMultiHdlrDominoTest, rmHdlrOnRoad)
{
    // not auto-cb but manually
    auto msgSelf = std::make_shared<MsgSelf>(
        [this](LoopBackFUNC aFunc){ this->loopbackFunc_ = aFunc; }, this->cellName());
    PARA_DOM->setMsgSelf(msgSelf);

    PARA_DOM->setHdlr("event", this->hdlr0_);
    PARA_DOM->multiHdlrOnSameEv("event", this->hdlr1_, "this->hdlr1_");
    PARA_DOM->multiHdlrOnSameEv("event", this->hdlr2_, "this->hdlr2_");
    PARA_DOM->setState({{"event", true}});                        // 3 cb on road
    EXPECT_TRUE(msgSelf->hasMsg());
    EXPECT_TRUE(PARA_DOM->rmOneHdlrOK("event"));                  // req: invalidate HdlrDom
    EXPECT_TRUE(PARA_DOM->rmOneHdlrOK("event", "this->hdlr2_"));  // req: invalidate MultiDom

    EXPECT_CALL(*this, hdlr0()).Times(0);
    EXPECT_CALL(*this, hdlr1()).Times(1);
    EXPECT_CALL(*this, hdlr2()).Times(0);
    if (msgSelf->hasMsg()) this->loopbackFunc_();                 // manual trigger on road cb

    PARA_DOM->setState({{"event", false}});
    PARA_DOM->setState({{"event", true}});                        // retrigger

    EXPECT_CALL(*this, hdlr0()).Times(0);
    EXPECT_CALL(*this, hdlr1()).Times(1);
    EXPECT_CALL(*this, hdlr2()).Times(0);
    if (msgSelf->hasMsg()) this->loopbackFunc_();                 // manual trigger on road cb
}
// ***********************************************************************************************
// rm invalid
// ***********************************************************************************************
TYPED_TEST_P(MultiHdlrDominoTest, rmHdlr_invalid)
{
    PARA_DOM->newEvent("event");
    EXPECT_FALSE(PARA_DOM->rmOneHdlrOK("event", "invalid hdlr"));
}

#define ID_STATE
// ***********************************************************************************************
// event & EvName are ID
// ***********************************************************************************************
TYPED_TEST_P(MultiHdlrDominoTest, GOLD_nonConstInterface_shall_createUnExistEvent_withStateFalse)
{
    PARA_DOM->multiHdlrOnSameEv("e1", this->hdlr0_, "h0");
    PARA_DOM->multiHdlrOnSameEv("e1", this->hdlr1_, "h1");
    this->uniqueEVs_.insert(PARA_DOM->getEventBy("e1"));
    EXPECT_EQ(1u, this->uniqueEVs_.size());
    EXPECT_FALSE(PARA_DOM->state("e1"));

    this->uniqueEVs_.insert(Domino::D_EVENT_FAILED_RET);
    EXPECT_EQ(2u, this->uniqueEVs_.size());

    PARA_DOM->rmOneHdlrOK("e2", "h2");  // shall NOT generate new event
    this->uniqueEVs_.insert(PARA_DOM->getEventBy("e2"));
    EXPECT_EQ(2u, this->uniqueEVs_.size());
}

// ***********************************************************************************************
REGISTER_TYPED_TEST_SUITE_P(MultiHdlrDominoTest
    , GOLD_multiAddHdlr_ok
    , aloneAddHdlr_ok
    , multiAddHdlr_bySameHdlrName_nok
    , immediateCallback_ok
    , rmHdlr_byHdlrName
    , rmLegacyHdlr_byNoHdlrName
    , rmHdlr_invalid
    , chain_callbackAllHdlr
    , newChain_immediateCallbackAll
    , newChain_dupSatisfy_callbackOnce
    , chain_sameHdlrOnDiffEvent_callbackEach
    , multiPrev_onlyAllSatisfy_thenCallback
    , GOLD_nonConstInterface_shall_createUnExistEvent_withStateFalse
);
using AnyMultiHdlrDom = Types<MinMhdlrDom, MaxNofreeDom, MaxDom>;
INSTANTIATE_TYPED_TEST_SUITE_P(PARA, MultiHdlrDominoTest, AnyMultiHdlrDom);

// ***********************************************************************************************
REGISTER_TYPED_TEST_SUITE_P(NofreeMultiHdlrDominoTest
    , repeatCallback_ok
    , rmHdlrOnRoad
);
using AnyNofreeMultiHdlrDom = Types<MinMhdlrDom, MaxNofreeDom>;
INSTANTIATE_TYPED_TEST_SUITE_P(PARA, NofreeMultiHdlrDominoTest, AnyNofreeMultiHdlrDom);
}  // namespace
