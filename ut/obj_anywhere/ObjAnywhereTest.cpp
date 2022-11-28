/**
 * Copyright 2018-2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include <gtest/gtest.h>
#include <memory>  // for shared_ptr

#include "UniLog.hpp"
#include "MsgSelf.hpp"
#include "ObjAnywhere.hpp"

using namespace testing;

namespace RLib
{
// ***********************************************************************************************
struct ObjAnywhereTest : public Test, public UniLog
{
    ObjAnywhereTest() : UniLog(UnitTest::GetInstance()->current_test_info()->name()) {}
    ~ObjAnywhereTest() { GTEST_LOG_FAIL }
};

#define GET_SET
// ***********************************************************************************************
TEST_F(ObjAnywhereTest, GOLD_setThenGetIt)
{
    ObjAnywhere::init(*this);
    auto p1 = make_shared<int>(1234);
    ObjAnywhere::set<int>(p1, *this);             // req: normal set
    EXPECT_EQ(p1, ObjAnywhere::get<int>(*this));  // req: get p1 itself
    ObjAnywhere::deinit(*this);
}
TEST_F(ObjAnywhereTest, get_replacement)
{
    ObjAnywhere::init(*this);
    auto p1 = make_shared<int>(1);
    ObjAnywhere::set<int>(p1, *this);
    auto p2 = make_shared<int>(2);
    ObjAnywhere::set<int>(p2, *this);             // req: replace set
    EXPECT_EQ(p2, ObjAnywhere::get<int>(*this));  // req: get p2 itself
    ObjAnywhere::deinit(*this);
}
TEST_F(ObjAnywhereTest, noSet_getNull)
{
    ObjAnywhere::init(*this);
    EXPECT_FALSE(ObjAnywhere::get<int>(*this));   // req: get null
    ObjAnywhere::deinit(*this);
}
TEST_F(ObjAnywhereTest, deinit_getNull)
{
    EXPECT_FALSE(ObjAnywhere::get<int>(*this));   // req: get null
}
TEST_F(ObjAnywhereTest, deinitThenSet_getNull)
{
    ObjAnywhere::set<int>(make_shared<int>(1234), *this);
    EXPECT_FALSE(ObjAnywhere::get<int>(*this));   // req: get null
}

#define CORRECT_DESTRUCT
// ***********************************************************************************************
struct TestObj                       // req: ObjAnywhere need not include Obj.hpp
{
    bool& isDestructed_;
    explicit TestObj(bool& aExtFlag) : isDestructed_(aExtFlag) { isDestructed_ = false; }
    ~TestObj() { isDestructed_ = true; }
};
TEST_F(ObjAnywhereTest, GOLD_destructCorrectly)
{
    bool isDestructed;
    ObjAnywhere::init(*this);
    ObjAnywhere::set<TestObj>(make_shared<TestObj>(isDestructed), *this);
    weak_ptr<TestObj> weak = ObjAnywhere::get<TestObj>(*this);
    EXPECT_EQ(1, weak.use_count());  // req: own TestObj
    EXPECT_FALSE(isDestructed);

    ObjAnywhere::deinit(*this);
    EXPECT_EQ(0, weak.use_count());  // req: destruct TestObj
    EXPECT_TRUE(isDestructed);       // req: destruct correctly
}
TEST_F(ObjAnywhereTest, destructBySetNull)
{
    bool isDestructed;
    ObjAnywhere::init(*this);
    ObjAnywhere::set<TestObj>(make_shared<TestObj>(isDestructed), *this);
    ObjAnywhere::set<TestObj>(shared_ptr<TestObj>(), *this);  // set null
    EXPECT_TRUE(isDestructed);                                // req: destruct correctly
    EXPECT_FALSE(ObjAnywhere::get<TestObj>(*this));           // req: destruct TestObj
    ObjAnywhere::deinit(*this);
}

#define INIT_DEINIT
// ***********************************************************************************************
TEST_F(ObjAnywhereTest, GOLD_init_deinit)
{
    ObjAnywhere::init(*this);
    EXPECT_TRUE(ObjAnywhere::isInit());

    ObjAnywhere::deinit(*this);
    EXPECT_FALSE(ObjAnywhere::isInit());
}
TEST_F(ObjAnywhereTest, dup_init_deinit)
{
    ObjAnywhere::init(*this);
    ObjAnywhere::init(*this);
    EXPECT_TRUE(ObjAnywhere::isInit());

    ObjAnywhere::deinit(*this);
    ObjAnywhere::deinit(*this);
    EXPECT_FALSE(ObjAnywhere::isInit());
}
TEST_F(ObjAnywhereTest, ignore_dup_init)
{
    ObjAnywhere::init(*this);
    auto pChar = make_shared<char>('a');
    ObjAnywhere::set<char>(pChar, *this);
    ObjAnywhere::init(*this);  // req: ignore dup init
    EXPECT_EQ(pChar, ObjAnywhere::get<char>(*this));
    ObjAnywhere::deinit(*this);
}
TEST_F(ObjAnywhereTest, default_is_deinit)
{
    EXPECT_FALSE(ObjAnywhere::isInit());
}
}  // namespace
