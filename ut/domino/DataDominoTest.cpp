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
TYPED_TEST_P(DataDominoTest, GOLD_setShared_thenGetIt_thenRmIt)
{
    PARA_DOM->replaceShared("ev0", make_shared<string>("ev0's data"));
    auto sharedString = static_pointer_cast<string>(PARA_DOM->getShared("ev0"));
    ASSERT_NE(nullptr, sharedString);
    EXPECT_EQ("ev0's data", *sharedString);  // req: get=set

    *sharedString = "ev0's updated data";
    EXPECT_EQ("ev0's updated data", (getValue<TypeParam, string>(*PARA_DOM, "ev0")));  // req: get=update

    PARA_DOM->replaceShared("ev0", make_shared<string>("replace ev0's data"));
    EXPECT_EQ("replace ev0's data", (getValue<TypeParam, string>(*PARA_DOM, "ev0")));  // req: get replaced
    EXPECT_NE(sharedString, static_pointer_cast<string>(PARA_DOM->getShared("ev0")));  // req: replace != old

    PARA_DOM->replaceShared("ev0");  // rm data
    EXPECT_EQ(nullptr, PARA_DOM->getShared("ev0"));  // req: get null
}
const string XPATH_BW =
    "/o-ran-delay-management:delay-management/bandwidth-scs-delay-state[bandwidth=50000][subcarrier-spacing=30]"
    "/bandwidth";
TYPED_TEST_P(DataDominoTest, GOLD_setValue_thenGetIt)
{
    size_t initValue = 50000;
    setValue<TypeParam, size_t>(*PARA_DOM, XPATH_BW, initValue);
    auto valGet = getValue<TypeParam, size_t>(*PARA_DOM, XPATH_BW);
    EXPECT_EQ(initValue, valGet);                // req: getValue = setValue

    size_t newValue = 10000;
    setValue<TypeParam, size_t>(*PARA_DOM, XPATH_BW, newValue);
    EXPECT_NE(newValue, valGet);                 // req: oldGet != newSet
    valGet = getValue<TypeParam, size_t>(*PARA_DOM, XPATH_BW);
    EXPECT_EQ(newValue, valGet);                 // req: newGet = newSet
}
TYPED_TEST_P(DataDominoTest, get_noData)
{
    auto value = getValue<TypeParam, int>(*PARA_DOM, "not exist event");
    EXPECT_EQ(0, value);  // req: no value return default

    PARA_DOM->replaceShared("not exist event");
    EXPECT_EQ(nullptr, ((const TypeParam)*PARA_DOM).getShared("not exist event"));  // req: const domino (coverage)

    PARA_DOM->replaceShared("not exist event", shared_ptr<void>());
    EXPECT_EQ(nullptr, ((const TypeParam)*PARA_DOM).getShared("not exist event"));  // req: const domino (coverage)
}

#define DESTRUCT
// ***********************************************************************************************
TYPED_TEST_P(DataDominoTest, GOLD_desruct_data)
{
    PARA_DOM->replaceShared("ev0", make_shared<char>('A'));
    weak_ptr<void> weak = PARA_DOM->getShared("ev0");
    EXPECT_NE(0, weak.use_count());

    PARA_DOM->replaceShared("ev0", make_shared<char>('B'));
    EXPECT_EQ(0, weak.use_count());  // req: old is rm

    weak = PARA_DOM->getShared("ev0");
    ObjAnywhere::deinit(*this);      // req: rm all
    EXPECT_EQ(0, weak.use_count());
}
TYPED_TEST_P(DataDominoTest, GOLD_correct_data_destructor)
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

    PARA_DOM->replaceShared("e2", make_shared<int>(0));
    EXPECT_NE(Domino::D_EVENT_FAILED_RET, PARA_DOM->getEventBy("e2"));  // req: new Event
    EXPECT_FALSE(PARA_DOM->state("e2"));
}

// ***********************************************************************************************
REGISTER_TYPED_TEST_SUITE_P(DataDominoTest
    , GOLD_setShared_thenGetIt_thenRmIt
    , GOLD_setValue_thenGetIt
    , get_noData
    , GOLD_desruct_data
    , GOLD_correct_data_destructor
    , GOLD_nonConstInterface_shall_createUnExistEvent_withStateFalse
);
using AnyDatDom = Types<MinDatDom, MinWbasicDatDom, MaxNofreeDom, MaxDom>;
INSTANTIATE_TYPED_TEST_SUITE_P(PARA, DataDominoTest, AnyDatDom);
}  // namespace
