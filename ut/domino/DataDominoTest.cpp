/**
 * Copyright 2020 Nokia. All rights reserved.
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
*/
// ***********************************************************************************************
#include <set>
#include <string>

#include "UtInitObjAnywhere.hpp"

namespace RLib
{
// ***********************************************************************************************
template<class aParaDom>
struct DataDominoTest : public UtInitObjAnywhere
{
};
TYPED_TEST_SUITE_P(DataDominoTest);

#define SET_GET
// ***********************************************************************************************
const string XPATH_BW =
    "/o-ran-delay-management:delay-management/bandwidth-scs-delay-state[bandwidth=50000][subcarrier-spacing=30]"
    "/bandwidth";
TYPED_TEST_P(DataDominoTest, GOLD_setValue_thenGetIt)
{
    auto valGet = getValue<TypeParam, size_t>(*PARA_DOM, XPATH_BW);  // req: any type data (1st=size_t)
    EXPECT_EQ(0, valGet) << "REQ: for nonexistent event, getValue() shall return default value." << endl;

    size_t initValue = 50000;
    setValue<TypeParam, size_t>(*PARA_DOM, XPATH_BW, initValue);
    valGet = getValue<TypeParam, size_t>(*PARA_DOM, XPATH_BW);
    EXPECT_EQ(initValue, valGet) << "REQ: get = set" << endl;

    size_t newValue = 10000;
    setValue<TypeParam, size_t>(*PARA_DOM, XPATH_BW, newValue);
    valGet = getValue<TypeParam, size_t>(*PARA_DOM, XPATH_BW);
    EXPECT_EQ(newValue, valGet) << "REQ: new get = new set" << endl;
}
TYPED_TEST_P(DataDominoTest, setShared_thenGetIt_thenRmIt)
{
    EXPECT_EQ(nullptr, PARA_DOM->getData("ev0")) << "REQ: get null since ev0 not exist" << endl;

    PARA_DOM->replaceData("ev0", MAKE_UNI_DATA<string>("ev0's data"));  // req: any type data (2nd=string)
    auto sharedString = static_pointer_cast<string>(PARA_DOM->getData("ev0"));
    ASSERT_NE(nullptr, sharedString);
    EXPECT_EQ("ev0's data", *sharedString) << "REQ: get = set" << endl;

    *sharedString = "ev0's updated data";
    EXPECT_EQ("ev0's updated data", (getValue<TypeParam, string>(*PARA_DOM, "ev0"))) << "REQ: get=update" << endl;

    PARA_DOM->replaceData("ev0", MAKE_UNI_DATA<string>("replace ev0's data"));
    EXPECT_EQ("replace ev0's data", (getValue<TypeParam, string>(*PARA_DOM, "ev0"))) << "REQ: get replaced" << endl;
    EXPECT_NE(sharedString, static_pointer_cast<string>(PARA_DOM->getData("ev0"))) << "REQ: replace != old" << endl;

    PARA_DOM->replaceData("ev0");  // req: rm data
    EXPECT_EQ(nullptr, PARA_DOM->getData("ev0")) << "REQ: get null" << endl;
}

#define DESTRUCT
// ***********************************************************************************************
TYPED_TEST_P(DataDominoTest, desruct_data)
{
    PARA_DOM->replaceData("ev0", MAKE_UNI_DATA<char>('A'));  // req: any type data (3rd=char)
    weak_ptr<void> weak = PARA_DOM->getData("ev0");
    EXPECT_NE(0, weak.use_count());

    PARA_DOM->replaceData("ev0", MAKE_UNI_DATA<char>('B'));
    EXPECT_EQ(0, weak.use_count()) << "REQ: old is rm-ed" << endl;

    weak = PARA_DOM->getData("ev0");
    ObjAnywhere::deinit(*this);  // req: rm all
    EXPECT_EQ(0, weak.use_count());
}
TYPED_TEST_P(DataDominoTest, correct_data_destructor)
{
    struct TestData
    {
        bool& isDestructed_;
        explicit TestData(bool& aExtFlag) : isDestructed_(aExtFlag) { isDestructed_ = false; }
        ~TestData() { isDestructed_ = true; }
    };
    bool isDestructed;

    PARA_DOM->replaceData("ev", MAKE_UNI_DATA<TestData>(isDestructed));  // req: any type data (4th=TestData)
    EXPECT_FALSE(isDestructed);
    PARA_DOM->replaceData("ev", shared_ptr<TestData>());
    EXPECT_TRUE(isDestructed);
}

#define ID_STATE
// ***********************************************************************************************
// event & EvName are ID
// ***********************************************************************************************
TYPED_TEST_P(DataDominoTest, nonConstInterface_shall_createUnExistEvent_withStateFalse)
{
    // DataDomino::
    PARA_DOM->getData("e1");
    EXPECT_EQ(Domino::D_EVENT_FAILED_RET, PARA_DOM->getEventBy("e1")) << "REQ: nonexistent" << endl;
    EXPECT_FALSE(PARA_DOM->state("e1"));

    PARA_DOM->replaceData("e2", MAKE_UNI_DATA<int>(0));  // REQ: any type data (5th=int)
    EXPECT_NE(Domino::D_EVENT_FAILED_RET, PARA_DOM->getEventBy("e2")) << "REQ: new Event" << endl;
    EXPECT_FALSE(PARA_DOM->state("e2"));
}

// ***********************************************************************************************
REGISTER_TYPED_TEST_SUITE_P(DataDominoTest
    , GOLD_setValue_thenGetIt
    , setShared_thenGetIt_thenRmIt
    , desruct_data
    , correct_data_destructor
    , nonConstInterface_shall_createUnExistEvent_withStateFalse
);
using AnyDatDom = Types<MinDatDom, MinWbasicDatDom, MaxNofreeDom, MaxDom>;
INSTANTIATE_TYPED_TEST_SUITE_P(PARA, DataDominoTest, AnyDatDom);
}  // namespace
