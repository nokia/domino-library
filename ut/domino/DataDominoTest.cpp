/**
 * Copyright 2020 Nokia. All rights reserved.
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
*/
// ***********************************************************************************************
#include <gtest/gtest.h>
#include <memory>  // make_shared
#include <set>
#include <string>

#include "CellLog.hpp"
#include "UtInitObjAnywhere.hpp"

using namespace testing;

namespace RLib
{
// ***********************************************************************************************
template<class aParaDom>
struct DataDominoTest : public Test, public CellLog
{
    GTEST_LOG_FAIL

    UtInitObjAnywhere utInit_;
    std::set<Domino::Event> uniqueEVs_;
};
TYPED_TEST_SUITE_P(DataDominoTest);

// ***********************************************************************************************
TYPED_TEST_P(DataDominoTest, GOLD_setShared_thenGetIt)
{
    PARA_DOM->replaceShared("ev0", std::make_shared<std::string>("ev0's data"));
    EXPECT_NE(0u, PARA_DOM->nShared("ev0"));                                                     // req: after creation

    auto sharedString = std::static_pointer_cast<std::string>(PARA_DOM->getShared("ev0"));
    EXPECT_EQ("ev0's data", *sharedString);                                                      // req: get=set

    *sharedString = "ev0's updated data";
    EXPECT_EQ("ev0's updated data", (getValue<TypeParam, std::string>(*PARA_DOM, "ev0")));       // req: get=update

    PARA_DOM->replaceShared("ev0", std::make_shared<std::string>("replace ev0's data"));
    EXPECT_EQ("replace ev0's data", (getValue<TypeParam, std::string>(*PARA_DOM, "ev0")));       // req: get replaced
    EXPECT_NE(sharedString, std::static_pointer_cast<std::string>(PARA_DOM->getShared("ev0")));  // req: replace != old
}
const std::string XPATH_BW =
    "/o-ran-delay-management:delay-management/bandwidth-scs-delay-state[bandwidth=50000][subcarrier-spacing=30]"
    "/bandwidth";
TYPED_TEST_P(DataDominoTest, GOLD_setValue_thenGetIt)
{
    size_t initValue = 50000;
    setValue<TypeParam, size_t>(*PARA_DOM, XPATH_BW, initValue);
    EXPECT_NE(0u, PARA_DOM->nShared(XPATH_BW));  // req: after creation

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
    getValue<TypeParam, int>(*PARA_DOM, "not exist event");
    EXPECT_EQ(0u, PARA_DOM->nShared("not exist event"));

    PARA_DOM->replaceShared("not exist event", std::shared_ptr<size_t>());
    EXPECT_EQ(0u, PARA_DOM->nShared("not exist event"));                     // req: setValue empty data
    EXPECT_EQ(0u, ((const TypeParam)*PARA_DOM).nShared("not exist event"));  // req: const domino
}

// ***********************************************************************************************
TYPED_TEST_P(DataDominoTest, GOLD_desruct_data)
{
    PARA_DOM->replaceShared("ev0", std::make_shared<char>('A'));
    std::weak_ptr<void> weak = PARA_DOM->getShared("ev0");
    EXPECT_NE(0, weak.use_count());

    PARA_DOM->replaceShared("ev0", std::make_shared<char>('B'));
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

    PARA_DOM->replaceShared("ev", std::make_shared<TestData>(isDestructed));
    EXPECT_FALSE(isDestructed);
    PARA_DOM->replaceShared("ev", std::shared_ptr<TestData>());
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
    PARA_DOM->getShared("e1");
    this->uniqueEVs_.insert(PARA_DOM->getEventBy("e1"));
    EXPECT_EQ(1u, this->uniqueEVs_.size());
    EXPECT_FALSE(PARA_DOM->state("e1"));

    PARA_DOM->replaceShared("e2", nullptr);
    PARA_DOM->replaceShared("e2", nullptr);
    this->uniqueEVs_.insert(PARA_DOM->getEventBy("e2"));
    EXPECT_EQ(2u, this->uniqueEVs_.size());
    EXPECT_FALSE(PARA_DOM->state("e2"));

    this->uniqueEVs_.insert(Domino::D_EVENT_FAILED_RET);
    EXPECT_EQ(3u, this->uniqueEVs_.size());
}

// ***********************************************************************************************
REGISTER_TYPED_TEST_SUITE_P(DataDominoTest
    , GOLD_setShared_thenGetIt
    , GOLD_setValue_thenGetIt
    , get_noData
    , GOLD_desruct_data
    , GOLD_correct_data_destructor
    , GOLD_nonConstInterface_shall_createUnExistEvent_withStateFalse
);
using AnyDatDom = Types<MinDatDom, MinWbasicDatDom, MaxNofreeDom, MaxDom>;
INSTANTIATE_TYPED_TEST_SUITE_P(PARA, DataDominoTest, AnyDatDom);
}  // namespace
