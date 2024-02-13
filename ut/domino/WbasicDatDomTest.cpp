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
    EXPECT_EQ(0u, valGet) << "REQ: default value for non-existed ev0" << endl;

    wbasic_setValue<TypeParam, size_t>(*PARA_DOM, "ev0", 1);
    valGet = wbasic_getValue<TypeParam, size_t>(*PARA_DOM, "ev0");
    EXPECT_EQ(1u, valGet) << "REQ: get = set" << endl;

    wbasic_setValue<TypeParam, size_t>(*PARA_DOM, "ev0", 2);
    valGet = wbasic_getValue<TypeParam, size_t>(*PARA_DOM, "ev0");
    EXPECT_EQ(2u, valGet) << "REQ: get = update" << endl;

    setValue<TypeParam, size_t>(*PARA_DOM, "ev0", 3);
    valGet = wbasic_getValue<TypeParam, size_t>(*PARA_DOM, "ev0");
    EXPECT_EQ(2u, valGet) << "REQ: legacy set failed" << endl;

    valGet = getValue<TypeParam, size_t>(*PARA_DOM, "ev0");
    EXPECT_EQ(0u, valGet) << "REQ: legacy get failed (ret default value)" << endl;

    PARA_DOM->replaceData("ev0");
    valGet = wbasic_getValue<TypeParam, size_t>(*PARA_DOM, "ev0");
    EXPECT_EQ(2u, valGet) << "REQ: legacy rm failed" << endl;

    PARA_DOM->wbasic_replaceData("ev0");
    auto shared = PARA_DOM->wbasic_getData("ev0");
    EXPECT_EQ(nullptr, shared) << "REQ: rm wr-data" << endl;
}
TYPED_TEST_P(WbasicDatDomTest, wrCtrlInterface_cannotHdl_nonWrDat)
{
    setValue<TypeParam, int>(*PARA_DOM, "ev0", 1);  // req: any type data (2nd=int>)
    auto valGet = wbasic_getValue<TypeParam, int>(*PARA_DOM, "ev0");
    EXPECT_EQ(0, valGet) << "REQ: w-get failed (ret default value)" << endl;

    wbasic_setValue<TypeParam, int>(*PARA_DOM, "ev0", 2);
    valGet = getValue<TypeParam, int>(*PARA_DOM, "ev0");
    EXPECT_EQ(1, valGet) << "REQ: w-set failed" << endl;

    PARA_DOM->wbasic_replaceData("ev0");
    valGet = getValue<TypeParam, int>(*PARA_DOM, "ev0");
    EXPECT_EQ(1, valGet) << "REQ: w-rm failed" << endl;
}
TYPED_TEST_P(WbasicDatDomTest, canNOT_setWriteCtrl_afterOwnData)
{
    setValue<TypeParam, char>(*PARA_DOM, "ev0", 'a');  // req: any type data (3rd=char>)
    auto data = PARA_DOM->getData("ev0");
    EXPECT_FALSE(PARA_DOM->wrCtrlOk("ev0")) << "REQ: failed to avoid out-ctrl" << endl;
    EXPECT_FALSE(PARA_DOM->isWrCtrl("ev0")) << "REQ: flag no change" << endl;

    auto pData = data.get();
    data.reset();
    EXPECT_NE(nullptr, pData);
    EXPECT_FALSE(PARA_DOM->wrCtrlOk("ev0")) << "REQ: still failed to avoid out-ctrl" << endl;
    EXPECT_FALSE(PARA_DOM->isWrCtrl("ev0")) << "REQ: flag no change" << endl;

    PARA_DOM->replaceData("ev0", nullptr);
    EXPECT_TRUE(PARA_DOM->wrCtrlOk("ev0")) << "REQ: succ since no data" << endl;
    EXPECT_TRUE(PARA_DOM->isWrCtrl("ev0")) << "REQ: flag change" << endl;
}

#define FLAG
// ***********************************************************************************************
TYPED_TEST_P(WbasicDatDomTest, setFlag_thenGetIt)
{
    EXPECT_FALSE(PARA_DOM->isWrCtrl("ev0")) << "REQ: invalid ev is false" << endl;

    PARA_DOM->newEvent("ev0");
    EXPECT_FALSE(PARA_DOM->isWrCtrl("ev0")) << "REQ: valid ev is false" << endl;

    PARA_DOM->wrCtrlOk("ev0");
    EXPECT_TRUE(PARA_DOM->isWrCtrl("ev0")) << "REQ: set true" << endl;

    PARA_DOM->wrCtrlOk("ev0", false);
    EXPECT_FALSE(PARA_DOM->isWrCtrl("ev0")) << "REQ: set false" << endl;

    PARA_DOM->wrCtrlOk("ev1");
    EXPECT_TRUE(PARA_DOM->isWrCtrl("ev1")) << "REQ: create & set true" << endl;

    PARA_DOM->wrCtrlOk("ev1");
    EXPECT_TRUE(PARA_DOM->isWrCtrl("ev1")) << "REQ: dup set true" << endl;
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
