/**
 * Copyright 2023 Nokia. All rights reserved.
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include <gtest/gtest.h>
#include <set>

#include "UtInitObjAnywhere.hpp"

namespace RLib
{
// ***********************************************************************************************
template<class aParaDom>
struct RmEvDomTest : public UtInitObjAnywhere
{
};

#define RM_DOM
// ***********************************************************************************************
template<class aParaDom> using RmDomTest = RmEvDomTest<aParaDom>;
TYPED_TEST_SUITE_P(RmDomTest);

TYPED_TEST_P(RmDomTest, GOLD_rm_dom_resrc)
{
    EXPECT_FALSE(PARA_DOM->rmEvOK(Domino::D_EVENT_FAILED_RET)) << "REQ: NOK to rm invalid Ev.";

    PARA_DOM->setPrev("e2", {{"e1", true}, {"e1b", true}});
    const auto e1 = PARA_DOM->setPrev("e1", {{"e0", false}});
    EXPECT_TRUE (PARA_DOM->state("e1"));
    EXPECT_FALSE(PARA_DOM->isRemoved(e1)) << "REQ: new ev is not removed state.";

    EXPECT_TRUE (PARA_DOM->rmEvOK(e1)) << "REQ: OK to rm valid Ev.";
    EXPECT_FALSE(PARA_DOM->rmEvOK(e1)) << "REQ: NOK to rm invalid Ev.";

    PARA_DOM->setState({{"e0", true}});
    PARA_DOM->setState({{"e0", false}});
    EXPECT_FALSE(PARA_DOM->state("e1")) << "REQ: e1's uplink is removed.";

    EXPECT_FALSE(PARA_DOM->state("e2"));
    PARA_DOM->setState({{"e1b", true}});
    EXPECT_TRUE(PARA_DOM->state("e2")) << "REQ: e1's downlink is removed.";

    EXPECT_FALSE(PARA_DOM->state("e1")) << "REQ: removed Ev's state=false";

    EXPECT_EQ(Domino::invalidEvName, PARA_DOM->evNames().at(e1)) << "REQ: EN is removed.";
    EXPECT_EQ(Domino::D_EVENT_FAILED_RET, PARA_DOM->getEventBy("e1")) << "REQ: EN is removed.";
}

TYPED_TEST_P(RmDomTest, GOLD_reuse_ev)
{
    PARA_DOM->setPrev("e2", {{"e1", true}});  // create e2 before e1 to inc cov of recycleEv()
    const auto e1 = PARA_DOM->setPrev("e1", {{"e0", false}});

    EXPECT_TRUE (PARA_DOM->rmEvOK(e1));
    EXPECT_EQ(e1, PARA_DOM->newEvent("new e1")) << "REQ: reuse removed ev.";
    EXPECT_FALSE(PARA_DOM->isRemoved(e1)) << "REQ: new ev is not removed state.";

    const auto e0 = PARA_DOM->getEventBy("e0");
    const auto e2 = PARA_DOM->getEventBy("e2");
    set<Domino::Event> evs = {e0, e2};

    EXPECT_TRUE(PARA_DOM->rmEvOK(e0)) << "REQ: can remove more ev.";
    EXPECT_TRUE(PARA_DOM->rmEvOK(e2)) << "REQ: existing multi removed ev.";
    EXPECT_EQ(1u, evs.count(PARA_DOM->newEvent("e3"))) << "REQ: can reuse removed ev.";
    EXPECT_EQ(1u, evs.count(PARA_DOM->newEvent("e4"))) << "REQ: can reuse removed ev.";
    EXPECT_NE(PARA_DOM->getEventBy("e3"), PARA_DOM->getEventBy("e4")) << "REQ: diff reused ev.";
}

REGISTER_TYPED_TEST_SUITE_P(RmDomTest
    , GOLD_rm_dom_resrc
    , GOLD_reuse_ev
);
using AnyRmDom = Types<MinRmEvDom, MaxNofreeDom, MaxDom>;
INSTANTIATE_TYPED_TEST_SUITE_P(PARA, RmDomTest, AnyRmDom);

#define RM_DATA_DOM
// ***********************************************************************************************
template<class aParaDom> using RmDataDomTest = RmEvDomTest<aParaDom>;
TYPED_TEST_SUITE_P(RmDataDomTest);

TYPED_TEST_P(RmDataDomTest, GOLD_rm_DataDom_resrc)
{
    struct TestData
    {
        bool& isDestructed_;
        explicit TestData(bool& aExtFlag) : isDestructed_(aExtFlag) { isDestructed_ = false; }
        ~TestData() { isDestructed_ = true; }
    };
    bool isDestructed;

    PARA_DOM->replaceShared("ev", make_shared<TestData>(isDestructed));
    EXPECT_FALSE(isDestructed);
    const auto ev = PARA_DOM->getEventBy("ev");
    EXPECT_FALSE(PARA_DOM->isRemoved(ev)) << "REQ: new ev is not removed state.";

    EXPECT_TRUE(PARA_DOM->rmEvOK(ev)) << "REQ: rm succ.";
    EXPECT_TRUE(isDestructed) << "REQ: data is removed.";
    EXPECT_EQ(nullptr, PARA_DOM->getShared("ev")) << "REQ: get null after removed." << endl;

    EXPECT_FALSE(PARA_DOM->rmEvOK(ev)) << "REQ: fail to rm invalid.";
}

REGISTER_TYPED_TEST_SUITE_P(RmDataDomTest
    , GOLD_rm_DataDom_resrc
);
using AnyRmDataDom = Types<MaxNofreeDom, MaxDom>;
INSTANTIATE_TYPED_TEST_SUITE_P(PARA, RmDataDomTest, AnyRmDataDom);

#define RM_W_DATA_DOM
// ***********************************************************************************************
template<class aParaDom> using RmWdatDomTest = RmEvDomTest<aParaDom>;
TYPED_TEST_SUITE_P(RmWdatDomTest);

TYPED_TEST_P(RmWdatDomTest, GOLD_rm_WdatDom_resrc)
{
    struct TestData
    {
        bool& isDestructed_;
        explicit TestData(bool& aExtFlag) : isDestructed_(aExtFlag) { isDestructed_ = false; }
        ~TestData() { isDestructed_ = true; }
    };
    bool isDestructed;

    EXPECT_TRUE(PARA_DOM->wrCtrlOk("ev", true)) << "REQ: test wctrl data.";
    PARA_DOM->wbasic_replaceShared("ev", make_shared<TestData>(isDestructed));
    EXPECT_FALSE(isDestructed);
    const auto ev = PARA_DOM->getEventBy("ev");
    EXPECT_FALSE(PARA_DOM->isRemoved(ev)) << "REQ: new ev is not removed state.";

    EXPECT_TRUE(PARA_DOM->rmEvOK(ev)) << "REQ: rm succ.";
    EXPECT_TRUE(isDestructed) << "REQ: data is removed.";
    EXPECT_EQ(nullptr, PARA_DOM->wbasic_getShared("ev")) << "REQ: get null after removed." << endl;
    EXPECT_FALSE(PARA_DOM->isWrCtrl("ev")) << "REQ: reset wctrl flag.";

    EXPECT_FALSE(PARA_DOM->rmEvOK(ev)) << "REQ: fail to rm invalid.";
}

REGISTER_TYPED_TEST_SUITE_P(RmWdatDomTest
    , GOLD_rm_WdatDom_resrc
);
using AnyRmWdatDom = Types<MaxNofreeDom, MaxDom>;
INSTANTIATE_TYPED_TEST_SUITE_P(PARA, RmWdatDomTest, AnyRmWdatDom);

#define RM_HDLR_DOM
// ***********************************************************************************************
template<class aParaDom> using RmHdlrDomTest = RmEvDomTest<aParaDom>;
TYPED_TEST_SUITE_P(RmHdlrDomTest);

TYPED_TEST_P(RmHdlrDomTest, GOLD_rm_HdlrDom_resrc)
{
    multiset<int> hdlrIDs;
    auto e1 = PARA_DOM->setHdlr("e1", [&hdlrIDs](){ hdlrIDs.insert(1); });
    auto e2 = PARA_DOM->multiHdlrByAliasEv("e2", [&hdlrIDs](){ hdlrIDs.insert(2); }, "e1");

    EXPECT_EQ(e1, PARA_DOM->setPriority("e1", EMsgPriority::EMsgPri_LOW));
    EXPECT_EQ(e2, PARA_DOM->setPriority("e2", EMsgPriority::EMsgPri_LOW));

    PARA_DOM->setState({{"e1", true}});
    EXPECT_EQ(2u, MSG_SELF->nMsg()) << "REQ: 2 hdlrs on road.";
    EXPECT_EQ(0u, hdlrIDs.size()) << "REQ: not callback yet.";

    EXPECT_TRUE(PARA_DOM->rmEvOK(e1));
    MSG_SELF->handleAllMsg(MSG_SELF->getValid());  // handle 1 low priority msg once
    EXPECT_EQ(1u, MSG_SELF->nMsg()) << "REQ: e1 not on road.";
    EXPECT_EQ(multiset<int>{}, hdlrIDs) << "REQ: not exe e1 hdlr since removed.";

    EXPECT_TRUE(PARA_DOM->rmEvOK(e2)) << "REQ: can rm alias Ev.";
    MSG_SELF->handleAllMsg(MSG_SELF->getValid());
    EXPECT_EQ(0u, MSG_SELF->nMsg()) << "REQ: e2 not on road.";
    EXPECT_EQ(multiset<int>{}, hdlrIDs) << "REQ: not exe e2 hdlr since removed.";
}

REGISTER_TYPED_TEST_SUITE_P(RmHdlrDomTest
    , GOLD_rm_HdlrDom_resrc
);
using AnyRmHdlrDom = Types<MaxNofreeDom>;
INSTANTIATE_TYPED_TEST_SUITE_P(PARA, RmHdlrDomTest, AnyRmHdlrDom);

#define RM_FREE_HDLR_DOM
// ***********************************************************************************************
template<class aParaDom> using RmFreeHdlrDomTest = RmEvDomTest<aParaDom>;
TYPED_TEST_SUITE_P(RmFreeHdlrDomTest);

TYPED_TEST_P(RmFreeHdlrDomTest, GOLD_rm_FreeHdlrDom_resrc)
{
    auto e1 = PARA_DOM->repeatedHdlr("e1");
    EXPECT_TRUE(PARA_DOM->isRepeatHdlr(e1));

    EXPECT_TRUE(PARA_DOM->rmEvOK(e1));
    EXPECT_FALSE(PARA_DOM->isRepeatHdlr(e1)) << "REQ: rm Ev shall clean auto-free flag.";
}

REGISTER_TYPED_TEST_SUITE_P(RmFreeHdlrDomTest
    , GOLD_rm_FreeHdlrDom_resrc
);
using AnyRmFreeHdlrDom = Types<MaxDom>;
INSTANTIATE_TYPED_TEST_SUITE_P(PARA, RmFreeHdlrDomTest, AnyRmFreeHdlrDom);

}  // namespace
