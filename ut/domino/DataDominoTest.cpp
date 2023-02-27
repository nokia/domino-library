/**
 * Copyright 2020 Nokia. All rights reserved.
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
struct DataDominoTest : public UtInitObjAnywhere
{
};
TYPED_TEST_SUITE_P(DataDominoTest);

#define SET_GET
// ***********************************************************************************************
const string XPATH_BW =
    "/o-ran-delay-management:delay-management/bandwidth-scs-delay-state[bandwidth=50000][subcarrier-spacing=30]"
    "/bandwidth";
TYPED_TEST_P(DataDominoTest, UC_setValue_thenGetIt)
{
    auto valGet = getValue<TypeParam, size_t>(*PARA_DOM, XPATH_BW);  // req: any type data (1st=size_t)
    EXPECT_EQ(0, valGet);  // req: default value for unexisted event

    size_t initValue = 50000;
    setValue<TypeParam, size_t>(*PARA_DOM, XPATH_BW, initValue);
    valGet = getValue<TypeParam, size_t>(*PARA_DOM, XPATH_BW);
    EXPECT_EQ(initValue, valGet);  // req: get = set

    size_t newValue = 10000;
    setValue<TypeParam, size_t>(*PARA_DOM, XPATH_BW, newValue);
    valGet = getValue<TypeParam, size_t>(*PARA_DOM, XPATH_BW);
    EXPECT_EQ(newValue, valGet);   // req: newGet = newSet
}
TYPED_TEST_P(DataDominoTest, UC_setShared_thenGetIt_thenRmIt)
{
    EXPECT_EQ(nullptr, PARA_DOM->getShared("ev0"));  // req: get null since “ev0" not exist"

    PARA_DOM->replaceShared("ev0", make_shared<string>("ev0's data"));  // req: any type data (2nd=string)
    auto sharedString = static_pointer_cast<string>(PARA_DOM->getShared("ev0"));
    ASSERT_NE(nullptr, sharedString);
    EXPECT_EQ("ev0's data", *sharedString);  // req: get = set

    *sharedString = "ev0's updated data";
    EXPECT_EQ("ev0's updated data", (getValue<TypeParam, string>(*PARA_DOM, "ev0")));  // req: get=update

    PARA_DOM->replaceShared("ev0", make_shared<string>("replace ev0's data"));
    EXPECT_EQ("replace ev0's data", (getValue<TypeParam, string>(*PARA_DOM, "ev0")));  // req: get replaced
    EXPECT_NE(sharedString, static_pointer_cast<string>(PARA_DOM->getShared("ev0")));  // req: replace != old

    PARA_DOM->replaceShared("ev0");  // req: rm data
    EXPECT_EQ(nullptr, PARA_DOM->getShared("ev0"));  // req: get null
}

#define DESTRUCT
// ***********************************************************************************************
TYPED_TEST_P(DataDominoTest, desruct_data)
{
    PARA_DOM->replaceShared("ev0", make_shared<char>('A'));  // req: any type data (3rd=char)
    weak_ptr<void> weak = PARA_DOM->getShared("ev0");
    EXPECT_NE(0, weak.use_count());

    PARA_DOM->replaceShared("ev0", make_shared<char>('B'));
    EXPECT_EQ(0, weak.use_count());  // req: old is rm

    weak = PARA_DOM->getShared("ev0");
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

    PARA_DOM->replaceShared("ev", make_shared<TestData>(isDestructed));  // req: any type data (4th=TestData)
    EXPECT_FALSE(isDestructed);
    PARA_DOM->replaceShared("ev", shared_ptr<TestData>());
    EXPECT_TRUE(isDestructed);
}

#define ID_STATE
// ***********************************************************************************************
// event & EvName are ID
// ***********************************************************************************************
TYPED_TEST_P(DataDominoTest, GOLD_nonConstInterface_shall_createUnExistEvent_withStateFalse)
{
    // DataDomino::
    PARA_DOM->getShared("e1");
    EXPECT_EQ(Domino::D_EVENT_FAILED_RET, PARA_DOM->getEventBy("e1"));  // req: no Event
    EXPECT_FALSE(PARA_DOM->state("e1"));

    PARA_DOM->replaceShared("e2", make_shared<int>(0));  // req: any type data (5th=int)
    EXPECT_NE(Domino::D_EVENT_FAILED_RET, PARA_DOM->getEventBy("e2"));  // req: new Event
    EXPECT_FALSE(PARA_DOM->state("e2"));
}

// ***********************************************************************************************
REGISTER_TYPED_TEST_SUITE_P(DataDominoTest
    , UC_setValue_thenGetIt
    , UC_setShared_thenGetIt_thenRmIt
    , desruct_data
    , correct_data_destructor
    , GOLD_nonConstInterface_shall_createUnExistEvent_withStateFalse
);
using AnyDatDom = Types<MinDatDom, MinWbasicDatDom, MaxNofreeDom, MaxDom>;
INSTANTIATE_TYPED_TEST_SUITE_P(PARA, DataDominoTest, AnyDatDom);
}  // namespace
