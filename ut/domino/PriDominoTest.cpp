/**
 * Copyright 2018-2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include <gtest/gtest.h>
#include <memory>
#include <queue>
#include <set>

#include "UtInitObjAnywhere.hpp"

using namespace testing;

namespace RLib
{
// ***********************************************************************************************
template<class aParaDom>
struct PriDominoTest : public Test, public UniLog
{
    PriDominoTest()
        : UniLog(UnitTest::GetInstance()->current_test_info()->name())
        , utInit_(uniLogName())
    {
        ObjAnywhere::get<aParaDom>(*this)->setMsgSelf(msgSelf_);

        *d1EventHdlr_ = [&](){ hdlrIDs_.push(1); };
        *d2EventHdlr_ = [&](){ hdlrIDs_.push(2); };
        *d3EventHdlr_ = [&](){ hdlrIDs_.push(3); };
        *d4EventHdlr_ = [&](){ hdlrIDs_.push(4); };
        *d5EventHdlr_ = [&]()
        {
            hdlrIDs_.push(5);

            ObjAnywhere::get<aParaDom>(*this)->setState({{"e2", true}});
            ObjAnywhere::get<aParaDom>(*this)->setPriority("e2", EMsgPri_HIGH);
            ObjAnywhere::get<aParaDom>(*this)->setHdlr("e2", *(this->d2EventHdlr_));  // raise when d5() is exe
        };
    }
    ~PriDominoTest() { GTEST_LOG_FAIL }

    // -------------------------------------------------------------------------------------------
    UtInitObjAnywhere utInit_;
    shared_ptr<MsgSelf> msgSelf_ = make_shared<MsgSelf>(
        [this](LoopBackFUNC aFunc){ loopbackFunc_ = aFunc; }, uniLogName());
    LoopBackFUNC loopbackFunc_;

    SharedMsgCB d1EventHdlr_ = make_shared<MsgCB>();
    SharedMsgCB d2EventHdlr_ = make_shared<MsgCB>();
    SharedMsgCB d3EventHdlr_ = make_shared<MsgCB>();
    SharedMsgCB d4EventHdlr_ = make_shared<MsgCB>();
    SharedMsgCB d5EventHdlr_ = make_shared<MsgCB>();

    queue<int> hdlrIDs_;
    set<Domino::Event> uniqueEVs_;
};
TYPED_TEST_SUITE_P(PriDominoTest);

// ***********************************************************************************************
template<class aParaDom> using NofreePriDominoTest = PriDominoTest<aParaDom>;  // for no-free testcase
TYPED_TEST_SUITE_P(NofreePriDominoTest);

#define PRI
// ***********************************************************************************************
TYPED_TEST_P(PriDominoTest, setPriority_thenGetIt)
{
    auto event = PARA_DOM->setPriority("event", EMsgPri_HIGH);
    EXPECT_EQ(EMsgPri_HIGH, PARA_DOM->getPriority(event));  // get set

    PARA_DOM->setPriority("event", EMsgPri_LOW);
    EXPECT_EQ(EMsgPri_LOW, PARA_DOM->getPriority(event));   // get updated
}
TYPED_TEST_P(PriDominoTest, defaultPriority)
{
    auto event = PARA_DOM->newEvent("");
    EXPECT_EQ(EMsgPri_NORM, PARA_DOM->getPriority(event));                       // req: valid event

    EXPECT_EQ(EMsgPri_NORM, PARA_DOM->getPriority(Domino::D_EVENT_FAILED_RET));  // req: invalid event
}

#define PRI_FIFO
// ***********************************************************************************************
TYPED_TEST_P(PriDominoTest, GOLD_setPriority_thenPriorityFifoCallback)
{
    PARA_DOM->setState({{"e1", true}});
    PARA_DOM->setHdlr("e1", *(this->d1EventHdlr_));

    PARA_DOM->setState({{"e5", true}});
    PARA_DOM->setPriority("e5", EMsgPri_HIGH);       // req: higher firstly, & derived callback
    PARA_DOM->setHdlr("e5", *(this->d5EventHdlr_));

    PARA_DOM->setState({{"e3", true}});
    PARA_DOM->setHdlr("e3", *(this->d3EventHdlr_));  // req: fifo same priority

    PARA_DOM->setState({{"e4", true}});
    PARA_DOM->setPriority("e4", EMsgPri_HIGH);
    PARA_DOM->setHdlr("e4", *(this->d4EventHdlr_));

    PARA_DOM->setPriority("e4", EMsgPri_NORM);       // req: new pri effective immediately, but no impact on road
    PARA_DOM->setState({{"e4", false}});
    PARA_DOM->setState({{"e4", true}});

    if (this->msgSelf_->hasMsg()) this->loopbackFunc_();
    if (this->hdlrIDs_.size() == 6) EXPECT_EQ(queue<int>({5, 4, 2, 1, 3, 4}), this->hdlrIDs_);
    else EXPECT_EQ(queue<int>({5, 4, 2, 1, 3}), this->hdlrIDs_);  // auto-rm-hdlr dom
}

#define ID_STATE
// ***********************************************************************************************
// event & EvName are ID
// ***********************************************************************************************
TYPED_TEST_P(PriDominoTest, GOLD_nonConstInterface_shall_createUnExistEvent_withStateFalse)
{
    PARA_DOM->setPriority("e1", EMsgPri_HIGH);
    PARA_DOM->setPriority("e1", EMsgPri_HIGH);
    this->uniqueEVs_.insert(PARA_DOM->getEventBy("e1"));
    EXPECT_EQ(1u, this->uniqueEVs_.size());
    EXPECT_FALSE(PARA_DOM->state("e1"));

    this->uniqueEVs_.insert(Domino::D_EVENT_FAILED_RET);
    EXPECT_EQ(2u, this->uniqueEVs_.size());
}

// ***********************************************************************************************
REGISTER_TYPED_TEST_SUITE_P(PriDominoTest
    , setPriority_thenGetIt
    , defaultPriority
    , GOLD_nonConstInterface_shall_createUnExistEvent_withStateFalse
    , GOLD_setPriority_thenPriorityFifoCallback
);
using AnyPriDom = Types<MinPriDom, MaxNofreeDom, MaxDom>;
INSTANTIATE_TYPED_TEST_SUITE_P(PARA, PriDominoTest, AnyPriDom);
}  // namespace
