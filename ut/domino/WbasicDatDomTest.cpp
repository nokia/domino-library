/**
 * Copyright 2021-2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include <gtest/gtest.h>
#include <memory>  // make_shared
#include <set>
#include <string>

#include "UtInitObjAnywhere.hpp"

using namespace testing;

namespace RLib
{
// ***********************************************************************************************
template<class aParaDom>
struct WbasicDatDomTest : public Test
{
    UtInitObjAnywhere utInit_;
    std::set<Domino::Event> uniqueEVs_;
};
TYPED_TEST_SUITE_P(WbasicDatDomTest);

// ***********************************************************************************************
TYPED_TEST_P(WbasicDatDomTest, GOLD_setFlag_thenGetIt)
{
    EXPECT_FALSE(PARA_DOM->isWrCtrl("ev0"));  // req: invalid ev is false

    PARA_DOM->newEvent("ev0");
    EXPECT_FALSE(PARA_DOM->isWrCtrl("ev0"));  // req: valid ev is false

    PARA_DOM->wrCtrlOk("ev0");
    EXPECT_TRUE(PARA_DOM->isWrCtrl("ev0"));   // req: set true

    PARA_DOM->wrCtrlOk("ev2");
    EXPECT_TRUE(PARA_DOM->isWrCtrl("ev2"));   // req: create & set true

    PARA_DOM->wrCtrlOk("ev2");
    EXPECT_TRUE(PARA_DOM->isWrCtrl("ev2"));   // req: dup set true
}

// ***********************************************************************************************
TYPED_TEST_P(WbasicDatDomTest, GOLD_write_ctrl)
{
    PARA_DOM->wrCtrlOk("ev0");
    wbasic_setValue<TypeParam, size_t>(*PARA_DOM, "ev0", 1);
    auto valGet = wbasic_getValue<TypeParam, size_t>(*PARA_DOM, "ev0");
    EXPECT_EQ(1u, valGet);                             // req: get=set

    wbasic_setValue<TypeParam, size_t>(*PARA_DOM, "ev0", 2);
    valGet = wbasic_getValue<TypeParam, size_t>(*PARA_DOM, "ev0");
    EXPECT_EQ(2u, valGet);                             // req: get=update

    setValue<TypeParam, size_t>(*PARA_DOM, "ev0", 3);  // legacy set
    valGet = wbasic_getValue<TypeParam, size_t>(*PARA_DOM, "ev0");
    EXPECT_EQ(2u, valGet);                             // req: legacy set failed

    valGet = getValue<TypeParam, size_t>(*PARA_DOM, "ev0");
    EXPECT_NE(3u, valGet);                             // req: legacy get failed
}
TYPED_TEST_P(WbasicDatDomTest, GOLD_no_write_ctrl)
{
    setValue<TypeParam, size_t>(*PARA_DOM, "ev0", 1);
    auto valGet = getValue<TypeParam, size_t>(*PARA_DOM, "ev0");
    EXPECT_EQ(1u, valGet);  // req: get=set

    setValue<TypeParam, size_t>(*PARA_DOM, "ev0", 2);
    valGet = getValue<TypeParam, size_t>(*PARA_DOM, "ev0");
    EXPECT_EQ(2u, valGet);  // req: get=update

    wbasic_setValue<TypeParam, size_t>(*PARA_DOM, "ev0", 3);
    valGet = getValue<TypeParam, size_t>(*PARA_DOM, "ev0");
    EXPECT_EQ(2u, valGet);  // req: wbasic_set failed

    valGet = wbasic_getValue<TypeParam, size_t>(*PARA_DOM, "ev0");
    EXPECT_NE(3u, valGet);  // req: wbasic_get failed
}
TYPED_TEST_P(WbasicDatDomTest, canNOT_setWriteCtrl_sinceOutCtrl)
{
    setValue<TypeParam, size_t>(*PARA_DOM, "ev0", 1);
    auto data = PARA_DOM->getShared("ev0");
    EXPECT_FALSE(PARA_DOM->wrCtrlOk("ev0"));  // req: can't protect data since out-ctrl
    EXPECT_FALSE(PARA_DOM->isWrCtrl("ev0"));  // req: can't protect data since out-ctrl

    auto pData = data.get();
    data.reset();
    EXPECT_NE(nullptr, pData);
    EXPECT_FALSE(PARA_DOM->wrCtrlOk("ev0"));  // req: can't protect data since still out-ctrl
    EXPECT_FALSE(PARA_DOM->isWrCtrl("ev0"));  // req: can't protect data since still out-ctrl

    PARA_DOM->replaceShared("ev0", nullptr);
    EXPECT_TRUE(PARA_DOM->wrCtrlOk("ev0"));   // req: can
    EXPECT_TRUE(PARA_DOM->isWrCtrl("ev0"));   // req: can
}

#define ID_STATE
// ***********************************************************************************************
// event & EvName are ID
// ***********************************************************************************************
TYPED_TEST_P(WbasicDatDomTest, GOLD_nonConstInterface_shall_createUnExistEvent_withStateFalse)
{
    PARA_DOM->wrCtrlOk("e1");
    PARA_DOM->wrCtrlOk("e1");
    this->uniqueEVs_.insert(PARA_DOM->getEventBy("e1"));
    EXPECT_EQ(1u, this->uniqueEVs_.size());
    EXPECT_FALSE(PARA_DOM->state("e1"));

    PARA_DOM->getShared("e2");
    PARA_DOM->getShared("e2");
    this->uniqueEVs_.insert(PARA_DOM->getEventBy("e2"));
    EXPECT_EQ(2u, this->uniqueEVs_.size());
    EXPECT_FALSE(PARA_DOM->state("e2"));

    PARA_DOM->replaceShared("e3", nullptr);
    PARA_DOM->replaceShared("e3", nullptr);
    this->uniqueEVs_.insert(PARA_DOM->getEventBy("e3"));
    EXPECT_EQ(3u, this->uniqueEVs_.size());
    EXPECT_FALSE(PARA_DOM->state("e3"));

    this->uniqueEVs_.insert(Domino::D_EVENT_FAILED_RET);
    EXPECT_EQ(4u, this->uniqueEVs_.size());

    PARA_DOM->wbasic_getShared("e4");               // shall NOT generate new event
    this->uniqueEVs_.insert(PARA_DOM->getEventBy("e4"));
    EXPECT_EQ(4u, this->uniqueEVs_.size());

    PARA_DOM->wbasic_replaceShared("e5", nullptr);  // shall NOT generate new event
    this->uniqueEVs_.insert(PARA_DOM->getEventBy("e5"));
    EXPECT_EQ(4u, this->uniqueEVs_.size());
}

// ***********************************************************************************************
REGISTER_TYPED_TEST_SUITE_P(WbasicDatDomTest
    , GOLD_setFlag_thenGetIt
    , GOLD_write_ctrl
    , GOLD_no_write_ctrl
    , canNOT_setWriteCtrl_sinceOutCtrl
    , GOLD_nonConstInterface_shall_createUnExistEvent_withStateFalse
);
using AnyDatDom = Types<MinWbasicDatDom, MaxNofreeDom, MaxDom>;
INSTANTIATE_TYPED_TEST_SUITE_P(PARA, WbasicDatDomTest, AnyDatDom);

}  // namespace
