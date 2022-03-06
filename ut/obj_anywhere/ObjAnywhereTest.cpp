/**
 * Copyright 2018-2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include <gtest/gtest.h>
#include <memory>  // for shared_ptr

#include "MsgSelf.hpp"
#include "ObjAnywhere.hpp"

using namespace testing;

namespace RLib
{
#define GET_SET
// ***********************************************************************************************
TEST(ObjAnywhereTest, GOLD_setThenGetIt)
{
    ObjAnywhere::init();
    auto p1 = std::make_shared<int>(1234);
    ObjAnywhere::set<int>(p1);               // req: normal set
    EXPECT_EQ(p1, ObjAnywhere::get<int>());  // req: get p1 itself
    ObjAnywhere::deinit();
}
TEST(ObjAnywhereTest, get_replacement)
{
    ObjAnywhere::init();
    auto p1 = std::make_shared<int>(1);
    ObjAnywhere::set<int>(p1);
    auto p2 = std::make_shared<int>(2);
    ObjAnywhere::set<int>(p2);               // req: replace set
    EXPECT_EQ(p2, ObjAnywhere::get<int>());  // req: get p2 itself
    ObjAnywhere::deinit();
}
TEST(ObjAnywhereTest, noSet_getNull)
{
    ObjAnywhere::init();
    EXPECT_FALSE(ObjAnywhere::get<int>());   // req: get null
    ObjAnywhere::deinit();
}
TEST(ObjAnywhereTest, deinit_getNull)
{
    EXPECT_FALSE(ObjAnywhere::get<int>());   // req: get null
}
TEST(ObjAnywhereTest, deinitThenSet_getNull)
{
    ObjAnywhere::set<int>(std::make_shared<int>(1234));
    EXPECT_FALSE(ObjAnywhere::get<int>());   // req: get null
}

#define CORRECT_DESTRUCT
// ***********************************************************************************************
struct TestObj                       // req: ObjAnywhere need not include Obj.hpp
{
    bool& isDestructed_;
    explicit TestObj(bool& aExtFlag) : isDestructed_(aExtFlag) { isDestructed_ = false; }
    ~TestObj() { isDestructed_ = true; }
};
TEST(ObjAnywhereTest, GOLD_destructCorrectly)
{
    bool isDestructed;
    ObjAnywhere::init();
    ObjAnywhere::set<TestObj>(std::make_shared<TestObj>(isDestructed));
    std::weak_ptr<TestObj> weak = ObjAnywhere::get<TestObj>();
    EXPECT_EQ(1, weak.use_count());  // req: own TestObj
    EXPECT_FALSE(isDestructed);

    ObjAnywhere::deinit();
    EXPECT_EQ(0, weak.use_count());  // req: destruct TestObj
    EXPECT_TRUE(isDestructed);       // req: destruct correctly
}
TEST(ObjAnywhereTest, destructBySetNull)
{
    bool isDestructed;
    ObjAnywhere::init();
    ObjAnywhere::set<TestObj>(std::make_shared<TestObj>(isDestructed));
    ObjAnywhere::set<TestObj>(std::shared_ptr<TestObj>());  // set null
    EXPECT_FALSE(ObjAnywhere::get<TestObj>());              // req: destruct TestObj
    EXPECT_TRUE(isDestructed);                              // req: destruct correctly
    ObjAnywhere::deinit();
}

#define INIT_DEINIT
// ***********************************************************************************************
TEST(ObjAnywhereTest, GOLD_init_deinit)
{
    ObjAnywhere::init();
    EXPECT_TRUE(ObjAnywhere::isInit());

    ObjAnywhere::deinit();
    EXPECT_FALSE(ObjAnywhere::isInit());
}
TEST(ObjAnywhereTest, dup_init_deinit)
{
    ObjAnywhere::init();
    ObjAnywhere::init();
    EXPECT_TRUE(ObjAnywhere::isInit());

    ObjAnywhere::deinit();
    ObjAnywhere::deinit();
    EXPECT_FALSE(ObjAnywhere::isInit());
}
TEST(ObjAnywhereTest, ignore_dup_init)
{
    ObjAnywhere::init();
    auto pChar = std::make_shared<char>('a');
    ObjAnywhere::set<char>(pChar);
    ObjAnywhere::init();  // req: ignore dup init
    EXPECT_EQ(pChar, ObjAnywhere::get<char>());
    ObjAnywhere::deinit();
}
TEST(ObjAnywhereTest, default_is_deinit)
{
    EXPECT_FALSE(ObjAnywhere::isInit());
}
}  // namespace
