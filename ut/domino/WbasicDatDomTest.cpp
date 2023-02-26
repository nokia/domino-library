/**
 * Copyright 2021-2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include <memory>  // make_shared
#include <set>
#include <string>

#include "UtInitObjAnywhere.hpp"

namespace RLib
{
// ***********************************************************************************************
template<class aParaDom>
struct WbasicDatDomTest : public UtInitObjAnywhere
{
    set<Domino::Event> uniqueEVs_;
};
TYPED_TEST_SUITE_P(WbasicDatDomTest);

#define SET_GET
// ***********************************************************************************************
TYPED_TEST_P(WbasicDatDomTest, GOLD_writeCtrlData_set_get_rm)
{
    PARA_DOM->wrCtrlOk("ev0");         // write-ctrl data
    wbasic_setValue<TypeParam, size_t>(*PARA_DOM, "ev0", 1);
    auto valGet = wbasic_getValue<TypeParam, size_t>(*PARA_DOM, "ev0");
    EXPECT_EQ(1u, valGet);             // req: get=set

    wbasic_setValue<TypeParam, size_t>(*PARA_DOM, "ev0", 2);
    valGet = wbasic_getValue<TypeParam, size_t>(*PARA_DOM, "ev0");
    EXPECT_EQ(2u, valGet);             // req: get=update

    setValue<TypeParam, size_t>(*PARA_DOM, "ev0", 3);  // legacy set
    valGet = wbasic_getValue<TypeParam, size_t>(*PARA_DOM, "ev0");
    EXPECT_EQ(2u, valGet);             // req: legacy set failed

    valGet = getValue<TypeParam, size_t>(*PARA_DOM, "ev0");
    EXPECT_NE(3u, valGet);             // req: legacy get failed

    PARA_DOM->replaceShared("ev0");
    valGet = wbasic_getValue<TypeParam, size_t>(*PARA_DOM, "ev0");
    EXPECT_EQ(2u, valGet);             // req: legacy can't rm wr-data
    valGet = getValue<TypeParam, size_t>(*PARA_DOM, "ev0");
    EXPECT_EQ(0u, valGet);             // req: legacy get default value

    PARA_DOM->wbasic_replaceShared("ev0");
    valGet = wbasic_getValue<TypeParam, size_t>(*PARA_DOM, "ev0");
    EXPECT_EQ(0u, valGet);             // req: rm wr-data
}
TYPED_TEST_P(WbasicDatDomTest, GOLD_noWriteCtrl_set_get_rm)
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

    PARA_DOM->wbasic_replaceShared("ev0");
    valGet = getValue<TypeParam, size_t>(*PARA_DOM, "ev0");
    EXPECT_EQ(2u, valGet);  // req: w- can't rm non-w-data
    valGet = wbasic_getValue<TypeParam, size_t>(*PARA_DOM, "ev0");
    EXPECT_EQ(0u, valGet);  // req: w- get default value

    PARA_DOM->replaceShared("ev0");
    valGet = getValue<TypeParam, size_t>(*PARA_DOM, "ev0");
    EXPECT_EQ(0u, valGet);  // req: rm non-w-data
}
TYPED_TEST_P(WbasicDatDomTest, canNOT_setWriteCtrl_afterOwnData)
{
    setValue<TypeParam, size_t>(*PARA_DOM, "ev0", 1);
    auto data = PARA_DOM->getShared("ev0");
    EXPECT_FALSE(PARA_DOM->wrCtrlOk("ev0"));  // req: failed to avoid out-ctrl
    EXPECT_FALSE(PARA_DOM->isWrCtrl("ev0"));  // req: flag no change

    auto pData = data.get();
    data.reset();
    EXPECT_NE(nullptr, pData);
    EXPECT_FALSE(PARA_DOM->wrCtrlOk("ev0"));  // req: still failed to avoid out-ctrl
    EXPECT_FALSE(PARA_DOM->isWrCtrl("ev0"));  // req: flag no change

    PARA_DOM->replaceShared("ev0", nullptr);
    EXPECT_TRUE(PARA_DOM->wrCtrlOk("ev0"));   // req: succ since no data
    EXPECT_TRUE(PARA_DOM->isWrCtrl("ev0"));   // req: flag change

    PARA_DOM->wrCtrlOk("ev1");
    wbasic_setValue<TypeParam, size_t>(*PARA_DOM, "ev1", 1);
    EXPECT_FALSE(PARA_DOM->wrCtrlOk("ev1", false));  // req: failed to avoid out-ctrl
    EXPECT_TRUE(PARA_DOM->isWrCtrl("ev1"));          // req: flag no change
}

#define FLAG
// ***********************************************************************************************
TYPED_TEST_P(WbasicDatDomTest, GOLD_setFlag_thenGetIt)
{
    EXPECT_FALSE(PARA_DOM->isWrCtrl("ev0"));  // req: invalid ev is false

    PARA_DOM->newEvent("ev0");
    EXPECT_FALSE(PARA_DOM->isWrCtrl("ev0"));  // req: valid ev is false

    PARA_DOM->wrCtrlOk("ev0");
    EXPECT_TRUE(PARA_DOM->isWrCtrl("ev0"));   // req: set true

    PARA_DOM->wrCtrlOk("ev0", false);
    EXPECT_FALSE(PARA_DOM->isWrCtrl("ev0"));  // req: set false

    PARA_DOM->wrCtrlOk("ev1");
    EXPECT_TRUE(PARA_DOM->isWrCtrl("ev1"));   // req: create & set true

    PARA_DOM->wrCtrlOk("ev1");
    EXPECT_TRUE(PARA_DOM->isWrCtrl("ev1"));   // req: dup set true

    PARA_DOM->newEvent("ev2");
    PARA_DOM->wrCtrlOk("ev3");
    // req: alloc ev3 in bitmap can't impact ev2(still not exist in bitmap)
    EXPECT_TRUE (PARA_DOM->isWrCtrl("ev3"));
    EXPECT_FALSE(PARA_DOM->isWrCtrl("ev2"));
}
TYPED_TEST_P(WbasicDatDomTest, setFlag_holeWorkWell)
{
    PARA_DOM->newEvent("ev2");
    PARA_DOM->wrCtrlOk("ev3");
    // req: alloc ev3 in bitmap can't impact ev2(still not exist in bitmap)
    EXPECT_TRUE (PARA_DOM->isWrCtrl("ev3"));
    EXPECT_FALSE(PARA_DOM->isWrCtrl("ev2"));  // in bitmap hole
}

#define ID_STATE
// ***********************************************************************************************
// event & EvName are ID
// ***********************************************************************************************
TYPED_TEST_P(WbasicDatDomTest, GOLD_nonConstInterface_shall_createUnExistEvent_withStateFalse)
{
    this->uniqueEVs_.insert(Domino::D_EVENT_FAILED_RET);
    EXPECT_EQ(1u, this->uniqueEVs_.size());

    PARA_DOM->wrCtrlOk("e1");                       // req: new Event
    this->uniqueEVs_.insert(PARA_DOM->getEventBy("e1"));
    EXPECT_EQ(2u, this->uniqueEVs_.size());
    EXPECT_FALSE(PARA_DOM->state("e1"));

    PARA_DOM->getShared("e2");                      // req: no Event
    this->uniqueEVs_.insert(PARA_DOM->getEventBy("e2"));
    EXPECT_EQ(2u, this->uniqueEVs_.size());
    EXPECT_FALSE(PARA_DOM->state("e2"));

    PARA_DOM->replaceShared("e3", make_shared<int>(0));  // req: new Event
    this->uniqueEVs_.insert(PARA_DOM->getEventBy("e3"));
    EXPECT_EQ(3u, this->uniqueEVs_.size());
    EXPECT_FALSE(PARA_DOM->state("e3"));

    PARA_DOM->wbasic_getShared("e4");               // req: no Event
    this->uniqueEVs_.insert(PARA_DOM->getEventBy("e4"));
    EXPECT_EQ(3u, this->uniqueEVs_.size());

    PARA_DOM->wbasic_replaceShared("e5", make_shared<int>(0));  // req: no Event since not isWrCtrl("e5")
    this->uniqueEVs_.insert(PARA_DOM->getEventBy("e5"));
    EXPECT_EQ(3u, this->uniqueEVs_.size());
}

// ***********************************************************************************************
REGISTER_TYPED_TEST_SUITE_P(WbasicDatDomTest
    , GOLD_setFlag_thenGetIt
    , setFlag_holeWorkWell
    , GOLD_writeCtrlData_set_get_rm
    , GOLD_noWriteCtrl_set_get_rm
    , canNOT_setWriteCtrl_afterOwnData
    , GOLD_nonConstInterface_shall_createUnExistEvent_withStateFalse
);
using AnyDatDom = Types<MinWbasicDatDom, MaxNofreeDom, MaxDom>;
INSTANTIATE_TYPED_TEST_SUITE_P(PARA, WbasicDatDomTest, AnyDatDom);

}  // namespace
