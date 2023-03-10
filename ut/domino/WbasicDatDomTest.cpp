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
TYPED_TEST_P(WbasicDatDomTest, GOLD_wrCtrl_set_get_rm)  // non-wrData is covered by DataDominoTest
{
    PARA_DOM->wrCtrlOk("ev0");  // write-ctrl data
    auto valGet = wbasic_getValue<TypeParam, size_t>(*PARA_DOM, "ev0");  // req: any type data (1st=size_t>)
    EXPECT_EQ(0u, valGet);  // req: default value for non-existed "ev0"

    wbasic_setValue<TypeParam, size_t>(*PARA_DOM, "ev0", 1);
    valGet = wbasic_getValue<TypeParam, size_t>(*PARA_DOM, "ev0");
    EXPECT_EQ(1u, valGet);  // req: get = set

    wbasic_setValue<TypeParam, size_t>(*PARA_DOM, "ev0", 2);
    valGet = wbasic_getValue<TypeParam, size_t>(*PARA_DOM, "ev0");
    EXPECT_EQ(2u, valGet);  // req: get = update

    setValue<TypeParam, size_t>(*PARA_DOM, "ev0", 3);
    valGet = wbasic_getValue<TypeParam, size_t>(*PARA_DOM, "ev0");
    EXPECT_EQ(2u, valGet);  // req: legacy set failed

    valGet = getValue<TypeParam, size_t>(*PARA_DOM, "ev0");
    EXPECT_EQ(0u, valGet);  // req: legacy get failed (ret default value)

    PARA_DOM->replaceShared("ev0");
    valGet = wbasic_getValue<TypeParam, size_t>(*PARA_DOM, "ev0");
    EXPECT_EQ(2u, valGet);  // req: legacy rm failed

    PARA_DOM->wbasic_replaceShared("ev0");
    auto shared = PARA_DOM->wbasic_getShared("ev0");
    EXPECT_EQ(nullptr, shared);  // req: rm wr-data
}
TYPED_TEST_P(WbasicDatDomTest, wrCtrlInterface_cannotHdl_nonWrDat)
{
    setValue<TypeParam, int>(*PARA_DOM, "ev0", 1);  // req: any type data (2nd=int>)
    auto valGet = wbasic_getValue<TypeParam, int>(*PARA_DOM, "ev0");
    EXPECT_EQ(0, valGet);  // req: w-get failed (ret default value)

    wbasic_setValue<TypeParam, int>(*PARA_DOM, "ev0", 2);
    valGet = getValue<TypeParam, int>(*PARA_DOM, "ev0");
    EXPECT_EQ(1, valGet);  // req: w-set failed

    PARA_DOM->wbasic_replaceShared("ev0");
    valGet = getValue<TypeParam, int>(*PARA_DOM, "ev0");
    EXPECT_EQ(1, valGet);  // req: w-rm failed
}
TYPED_TEST_P(WbasicDatDomTest, canNOT_setWriteCtrl_afterOwnData)
{
    setValue<TypeParam, char>(*PARA_DOM, "ev0", 'a');  // req: any type data (3rd=char>)
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
}

#define FLAG
// ***********************************************************************************************
TYPED_TEST_P(WbasicDatDomTest, setFlag_thenGetIt)
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
TYPED_TEST_P(WbasicDatDomTest, nonConstInterface_shall_createUnExistEvent_withStateFalse)
{
    this->uniqueEVs_.insert(Domino::D_EVENT_FAILED_RET);
    EXPECT_EQ(1u, this->uniqueEVs_.size());

    PARA_DOM->wrCtrlOk("e1");  // req: new Event
    this->uniqueEVs_.insert(PARA_DOM->getEventBy("e1"));
    EXPECT_EQ(2u, this->uniqueEVs_.size());
    EXPECT_FALSE(PARA_DOM->state("e1"));

    wbasic_getValue<TypeParam, char>(*PARA_DOM, "e4");  // req: no Event
    this->uniqueEVs_.insert(PARA_DOM->getEventBy("e4"));
    EXPECT_EQ(2u, this->uniqueEVs_.size());

    wbasic_setValue<TypeParam, int>(*PARA_DOM, "e5", 0);  // req: no Event since not isWrCtrl("e5")
    this->uniqueEVs_.insert(PARA_DOM->getEventBy("e5"));
    EXPECT_EQ(2u, this->uniqueEVs_.size());
}

// ***********************************************************************************************
REGISTER_TYPED_TEST_SUITE_P(WbasicDatDomTest
    , GOLD_wrCtrl_set_get_rm
    , wrCtrlInterface_cannotHdl_nonWrDat
    , canNOT_setWriteCtrl_afterOwnData
    , setFlag_thenGetIt
    , setFlag_holeWorkWell
    , nonConstInterface_shall_createUnExistEvent_withStateFalse
);
using AnyDatDom = Types<MinWbasicDatDom, MaxNofreeDom, MaxDom>;
INSTANTIATE_TYPED_TEST_SUITE_P(PARA, WbasicDatDomTest, AnyDatDom);

}  // namespace
