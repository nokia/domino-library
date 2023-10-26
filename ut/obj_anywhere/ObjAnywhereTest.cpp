/**
 * Copyright 2018-2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include <gtest/gtest.h>
#include <memory>  // for shared_ptr

#include "UniLog.hpp"
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
    ObjAnywhere::set(p1, *this);  // req: normal set
    EXPECT_EQ(p1, ObjAnywhere::get<int>(*this)) << "REQ: get p1 itself" << endl;

    ObjAnywhere::set<int>(nullptr, *this);  // req: set null
    EXPECT_EQ(nullptr, ObjAnywhere::get<int>(*this)) << "REQ: get null" << endl;
    ObjAnywhere::deinit(*this);
}
TEST_F(ObjAnywhereTest, get_replacement)
{
    ObjAnywhere::init(*this);
    auto p1 = make_shared<int>(1);
    ObjAnywhere::set(p1, *this);
    auto p2 = make_shared<int>(2);
    ObjAnywhere::set(p2, *this);  // req: replace set
    EXPECT_EQ(p2, ObjAnywhere::get<int>(*this)) << "REQ: get p2 itself" << endl;
    ObjAnywhere::deinit(*this);
}
TEST_F(ObjAnywhereTest, noSet_getNull)
{
    ObjAnywhere::init(*this);
    EXPECT_EQ(nullptr, ObjAnywhere::get<int>(*this)) << "REQ: get null" << endl;
    ObjAnywhere::deinit(*this);
}
TEST_F(ObjAnywhereTest, deinit_getNull)
{
    EXPECT_EQ(nullptr, ObjAnywhere::get<int>(*this)) << "REQ: get null" << endl;
}
TEST_F(ObjAnywhereTest, deinitThenSet_getNull)
{
    ObjAnywhere::set(make_shared<int>(1234), *this);
    EXPECT_EQ(nullptr, ObjAnywhere::get<int>(*this)) << "REQ: get null" << endl;
}

#define CORRECT_DESTRUCT
// ***********************************************************************************************
struct TestObj  // req: ObjAnywhere need not include Obj.hpp
{
    bool& isDestructed_;
    explicit TestObj(bool& aExtFlag) : isDestructed_(aExtFlag) { isDestructed_ = false; }
    ~TestObj() { isDestructed_ = true; }
};
TEST_F(ObjAnywhereTest, GOLD_destructCorrectly)
{
    bool isDestructed;
    ObjAnywhere::init(*this);
    ObjAnywhere::set(make_shared<TestObj>(isDestructed), *this);
    weak_ptr<TestObj> weak = ObjAnywhere::get<TestObj>(*this);
    EXPECT_EQ(1, weak.use_count()) << "REQ: own TestObj" << endl;
    EXPECT_FALSE(isDestructed);

    ObjAnywhere::deinit(*this);
    EXPECT_EQ(0, weak.use_count()) << "REQ: destruct TestObj" << endl;
    EXPECT_TRUE(isDestructed) << "REQ: destruct correctly" << endl;
}
TEST_F(ObjAnywhereTest, destructBySetNull)
{
    bool isDestructed;
    ObjAnywhere::init(*this);

    ObjAnywhere::set(make_shared<TestObj>(isDestructed), *this);
    ObjAnywhere::set(shared_ptr<TestObj>(), *this);  // set null
    EXPECT_TRUE(isDestructed) << "REQ: destruct correctly" << endl;
    EXPECT_FALSE(ObjAnywhere::get<TestObj>(*this)) << "REQ: destruct TestObj" << endl;

    ObjAnywhere::set(shared_ptr<TestObj>(), *this);  // REQ: rm unexist
    EXPECT_EQ(0u, ObjAnywhere::nObj());

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
    ObjAnywhere::set(pChar, *this);
    ObjAnywhere::init(*this);  // req: ignore dup init
    EXPECT_EQ(pChar, ObjAnywhere::get<char>(*this));
    ObjAnywhere::deinit(*this);
}
TEST_F(ObjAnywhereTest, default_is_deinit)
{
    EXPECT_FALSE(ObjAnywhere::isInit());
    EXPECT_EQ(0u, ObjAnywhere::nObj());
}
}  // namespace
