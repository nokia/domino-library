/**
 * Copyright 2023 Nokia. All rights reserved.
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include <gtest/gtest.h>

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

    const auto e1 = PARA_DOM->setPrev("e1", {{"e0", false}});
    PARA_DOM->setPrev("e2", {{"e1", true}, {"e1b", true}});
    EXPECT_TRUE (PARA_DOM->state("e1"));

    EXPECT_TRUE (PARA_DOM->rmEvOK(e1)) << "REQ: OK to rm leaf Ev.";
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

REGISTER_TYPED_TEST_SUITE_P(RmDomTest
    , GOLD_rm_dom_resrc
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

}  // namespace
