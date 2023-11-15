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
TYPED_TEST_SUITE_P(RmEvDomTest);

#define RM_LEAF
// ***********************************************************************************************
TYPED_TEST_P(RmEvDomTest, GOLD_rm_leaf)
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

//    PARA_DOM->
}

// ***********************************************************************************************
REGISTER_TYPED_TEST_SUITE_P(RmEvDomTest
    , GOLD_rm_leaf
);
using AnyRmDom = Types<MinRmEvDom, MaxNofreeDom, MaxDom>;
INSTANTIATE_TYPED_TEST_SUITE_P(PARA, RmEvDomTest, AnyRmDom);

}  // namespace
