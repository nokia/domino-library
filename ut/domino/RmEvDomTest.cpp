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
    EXPECT_FALSE(PARA_DOM->rmEvOK("invalid EN")) << "REQ: NOK to rm invalid Ev.";

    PARA_DOM->setPrev("e2", {{"e1", true}, {"e1b", true}});
    const auto e1 = PARA_DOM->setPrev("e1", {{"e0", false}});
    EXPECT_TRUE (PARA_DOM->state("e1"));
    EXPECT_FALSE(PARA_DOM->isRemoved(e1)) << "REQ: new ev is not removed state.";

    EXPECT_TRUE(PARA_DOM->rmEvOK("e1"))  << "REQ: OK to rm valid Ev.";
    EXPECT_TRUE(PARA_DOM->isRemoved(e1)) << "REQ: flag is set.";

    EXPECT_FALSE(PARA_DOM->state("e1")) << "REQ: reset state of removed Ev.";

    PARA_DOM->setState({{"e0", true}});
    PARA_DOM->setState({{"e0", false}});
    EXPECT_FALSE(PARA_DOM->state("e1")) << "REQ: e1's uplink is removed.";

    EXPECT_FALSE(PARA_DOM->state("e2"));
    PARA_DOM->setState({{"e1b", true}});
    EXPECT_TRUE(PARA_DOM->state("e2")) << "REQ: e1's downlink is removed.";

    EXPECT_EQ(0u, PARA_DOM->evNames().count(e1)) << "REQ: EN is removed.";
    EXPECT_EQ(Domino::D_EVENT_FAILED_RET, PARA_DOM->getEventBy("e1")) << "REQ: EN is removed.";

    EXPECT_FALSE(PARA_DOM->rmEvOK("e1")) << "REQ: NOK to rm invalid Ev.";
}

TYPED_TEST_P(RmDomTest, GOLD_reuse_ev)
{
    PARA_DOM->setPrev("e2", {{"e1", true}});  // create e2 before e1 to inc cov of recycleEv_()
    const auto e1 = PARA_DOM->setPrev("e1", {{"e0", false}});

    EXPECT_TRUE (PARA_DOM->rmEvOK("e1"));
    EXPECT_EQ(e1, PARA_DOM->newEvent("new e1")) << "REQ: reuse removed ev as soon as possible.";
    EXPECT_FALSE(PARA_DOM->isRemoved(e1)) << "REQ: new ev is not removed state.";

    const auto e0 = PARA_DOM->getEventBy("e0");
    const auto e2 = PARA_DOM->getEventBy("e2");
    set<Domino::Event> evs = {e0, e2};

    EXPECT_TRUE(PARA_DOM->rmEvOK("e0")) << "REQ: can remove more ev.";
    EXPECT_TRUE(PARA_DOM->rmEvOK("e2")) << "REQ: existing multi removed ev.";
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

    PARA_DOM->replaceData("ev", MAKE_PTR<TestData>(isDestructed));
    EXPECT_FALSE(isDestructed);
    const auto ev = PARA_DOM->getEventBy("ev");

    EXPECT_TRUE(PARA_DOM->rmEvOK("ev")) << "REQ: rm succ.";
    EXPECT_TRUE(isDestructed) << "REQ: data is removed.";
    EXPECT_EQ(nullptr, PARA_DOM->getData("ev").get()) << "REQ: get null after removed.";

    EXPECT_EQ(ev, PARA_DOM->newEvent("another ev"))  << "REQ: reuse ev.";
    EXPECT_EQ(nullptr, PARA_DOM->getData("another ev").get()) << "REQ: reuse ev's data space.";
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
    PARA_DOM->wbasic_replaceData("ev", MAKE_PTR<TestData>(isDestructed));
    EXPECT_FALSE(isDestructed);
    const auto ev = PARA_DOM->getEventBy("ev");

    EXPECT_TRUE(PARA_DOM->rmEvOK("ev")) << "REQ: rm succ.";
    EXPECT_TRUE(isDestructed) << "REQ: data is removed.";
    EXPECT_EQ(nullptr, PARA_DOM->wbasic_getData("ev").get()) << "REQ: get null after removed.";
    EXPECT_FALSE(PARA_DOM->isWrCtrl("ev")) << "REQ: reset wctrl flag.";

    EXPECT_FALSE(PARA_DOM->rmEvOK("ev")) << "REQ: fail to rm invalid.";

    EXPECT_EQ(ev, PARA_DOM->newEvent("another ev"))  << "REQ: reuse ev.";
    EXPECT_EQ(nullptr, PARA_DOM->wbasic_getData("another ev").get()) << "REQ: reuse ev's data space.";
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
    PARA_DOM->setPriority("e1", EMsgPriority::EMsgPri_LOW);
    PARA_DOM->setState({{"e1", true}});
    EXPECT_EQ(1u, MSG_SELF->nMsg()) << "REQ: e1's hdlr on road.";

    PARA_DOM->setPriority("e2", EMsgPriority::EMsgPri_LOW);
    auto e2 = PARA_DOM->multiHdlrByAliasEv("e2", [&hdlrIDs](){ hdlrIDs.insert(2); }, "e1");
    EXPECT_EQ(2u, MSG_SELF->nMsg()) << "REQ: 2 hdlrs on road.";
    EXPECT_EQ(0u, hdlrIDs.size()) << "REQ: not callback yet.";

    EXPECT_TRUE(PARA_DOM->rmEvOK("e1"));
    MSG_SELF->handleAllMsg(MSG_SELF->getValid());  // handle 1 low priority msg once
    EXPECT_EQ(multiset<int>{}, hdlrIDs) << "REQ: not exe e1 hdlr since removed.";

    EXPECT_EQ(e1, PARA_DOM->setHdlr("another e1", [&hdlrIDs](){ hdlrIDs.insert(3); }))  << "REQ: reuse e1.";
    PARA_DOM->forceAllHdlr("another e1");
    EXPECT_NE(Domino::D_EVENT_FAILED_RET, PARA_DOM->getEventBy("e2")) << "REQ: rm ev not impact its alias.";
    EXPECT_EQ(2u, MSG_SELF->nMsg()) << "REQ: another e1's hdlr is on road while alias hdlr still on road.";

    EXPECT_TRUE(PARA_DOM->rmEvOK("e2")) << "REQ: can rm alias Ev.";
    MSG_SELF->handleAllMsg(MSG_SELF->getValid());
    EXPECT_EQ(multiset<int>{3}, hdlrIDs) << "REQ: not exe e2 hdlr since removed; exe another e1's hdlr.";
}

TYPED_TEST_P(RmHdlrDomTest, rmTruePrev_callHdlr_ifSatisfied)
{
    multiset<int> hdlrIDs;
    PARA_DOM->setHdlr("e0", [&hdlrIDs](){ hdlrIDs.insert(0); });
    PARA_DOM->setPrev("e0", {{"e1", true}, {"e2", false}});
    EXPECT_FALSE(PARA_DOM->state("e0"));

    EXPECT_TRUE(PARA_DOM->rmEvOK("e1")) << "req: rm succ";
    EXPECT_TRUE(PARA_DOM->state("e0")) << "REQ: deduce related next";
    MSG_SELF->handleAllMsg(MSG_SELF->getValid());
    EXPECT_EQ(multiset<int>{0}, hdlrIDs) << "REQ: prerequisite satisfied -> call hdlr.";
}
TYPED_TEST_P(RmHdlrDomTest, rmFalsePrev_callHdlr_ifSatisfied)
{
    multiset<int> hdlrIDs;
    PARA_DOM->setHdlr("e0", [&hdlrIDs](){ hdlrIDs.insert(0); });
    PARA_DOM->setPrev("e0", {{"e1", true}, {"e2", false}});
    PARA_DOM->setState({{"e1", true}, {"e2", true}});
    EXPECT_FALSE(PARA_DOM->state("e0"));

    EXPECT_TRUE(PARA_DOM->rmEvOK("e2")) << "req: rm succ";
    EXPECT_TRUE(PARA_DOM->state("e0")) << "REQ: deduce related next";
    MSG_SELF->handleAllMsg(MSG_SELF->getValid());
    EXPECT_EQ(multiset<int>{0}, hdlrIDs) << "REQ: prerequisite satisfied -> call hdlr.";
}

REGISTER_TYPED_TEST_SUITE_P(RmHdlrDomTest
    , GOLD_rm_HdlrDom_resrc
    , rmFalsePrev_callHdlr_ifSatisfied
    , rmTruePrev_callHdlr_ifSatisfied
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

    EXPECT_TRUE(PARA_DOM->rmEvOK("e1"));
    EXPECT_FALSE(PARA_DOM->isRepeatHdlr(e1)) << "REQ: rm Ev shall clean auto-free flag.";

    EXPECT_EQ(e1, PARA_DOM->repeatedHdlr("another e1")) << "REQ: reuse e1.";
    EXPECT_TRUE(PARA_DOM->isRepeatHdlr(e1));
}

REGISTER_TYPED_TEST_SUITE_P(RmFreeHdlrDomTest
    , GOLD_rm_FreeHdlrDom_resrc
);
using AnyRmFreeHdlrDom = Types<MaxDom>;
INSTANTIATE_TYPED_TEST_SUITE_P(PARA, RmFreeHdlrDomTest, AnyRmFreeHdlrDom);

#define RM_PRI_DOM
// ***********************************************************************************************
template<class aParaDom> using RmPriDomTest = RmEvDomTest<aParaDom>;
TYPED_TEST_SUITE_P(RmPriDomTest);

TYPED_TEST_P(RmPriDomTest, GOLD_rm_PriDom_resrc)
{
    auto e1 = PARA_DOM->setPriority("e1", EMsgPriority::EMsgPri_LOW);
    EXPECT_EQ(EMsgPriority::EMsgPri_LOW, PARA_DOM->getPriority(e1));

    EXPECT_TRUE(PARA_DOM->rmEvOK("e1"));
    EXPECT_EQ(EMsgPriority::EMsgPri_NORM, PARA_DOM->getPriority(e1)) << "REQ: reset pri.";

    EXPECT_EQ(e1, PARA_DOM->setPriority("e1", EMsgPriority::EMsgPri_HIGH)) << "REQ: reuse e1.";
    EXPECT_EQ(EMsgPriority::EMsgPri_HIGH, PARA_DOM->getPriority(e1)) << "REQ: new pri";
}

REGISTER_TYPED_TEST_SUITE_P(RmPriDomTest
    , GOLD_rm_PriDom_resrc
);
using AnyRmPriDom = Types<MaxNofreeDom, MaxDom>;
INSTANTIATE_TYPED_TEST_SUITE_P(PARA, RmPriDomTest, AnyRmPriDom);

#define RM_M_HDLR_DOM
// ***********************************************************************************************
template<class aParaDom> using RmMhdlrDomTest = RmEvDomTest<aParaDom>;
TYPED_TEST_SUITE_P(RmMhdlrDomTest);

TYPED_TEST_P(RmMhdlrDomTest, GOLD_rm_MhdlrDom_resrc)
{
    multiset<int> hdlrIDs;
    auto e1 = PARA_DOM->setHdlr("e1", [&hdlrIDs](){ hdlrIDs.insert(1); });
    PARA_DOM->multiHdlrOnSameEv("e1", [&hdlrIDs](){ hdlrIDs.insert(2); }, "h2");

    PARA_DOM->setState({{"e1", true}});
    EXPECT_EQ(2u, MSG_SELF->nMsg()) << "REQ: 2 hdlrs on road.";
    EXPECT_EQ(0u, hdlrIDs.size()) << "REQ: not callback yet.";

    EXPECT_TRUE(PARA_DOM->rmEvOK("e1"));
    MSG_SELF->handleAllMsg(MSG_SELF->getValid());
    EXPECT_EQ(multiset<int>{}, hdlrIDs) << "REQ: not exe e2 hdlr since removed.";

    EXPECT_EQ(e1, PARA_DOM->multiHdlrOnSameEv("reuse e1", [&hdlrIDs](){ hdlrIDs.insert(3); }, "h3"));
    PARA_DOM->forceAllHdlr("reuse e1");
    MSG_SELF->handleAllMsg(MSG_SELF->getValid());
    EXPECT_EQ(multiset<int>{3}, hdlrIDs) << "REQ: exe new hdlr.";
}

REGISTER_TYPED_TEST_SUITE_P(RmMhdlrDomTest
    , GOLD_rm_MhdlrDom_resrc
);
using AnyRmMhdlrDom = Types<MaxNofreeDom>;
INSTANTIATE_TYPED_TEST_SUITE_P(PARA, RmMhdlrDomTest, AnyRmMhdlrDom);

}  // namespace
