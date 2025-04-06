/**
 * Copyright 2018-2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include <gtest/gtest.h>

#include "UniLog.hpp"
#include "ObjAnywhere.hpp"

using namespace testing;

namespace rlib
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
    auto p1 = MAKE_PTR<int>(1234);
    EXPECT_TRUE(ObjAnywhere::emplaceObjOK(p1, *this)) << "REQ: normal set succ";
    EXPECT_EQ(1234, *(ObjAnywhere::getObj<int>().get())) << "REQ: get p1 itself";

    EXPECT_TRUE(ObjAnywhere::emplaceObjOK(MAKE_PTR<int>(5678), *this, "i2"));
    EXPECT_EQ(5678,  *(ObjAnywhere::getObj<int >("i2").get())) << "REQ: ok to store same type";
    EXPECT_EQ(nullptr, ObjAnywhere::getObj<bool>("i2").get())  << "REQ: get invalid type -> ret null (shared_ptr failed here)";

    EXPECT_TRUE(ObjAnywhere::emplaceObjOK<int>(nullptr, *this)) << "REQ: set null succ";
    EXPECT_EQ(nullptr, ObjAnywhere::getObj<int>().get()) << "REQ: get null";
    ObjAnywhere::deinit();
}
TEST_F(ObjAnywhereTest, get_replacement)
{
    ObjAnywhere::init(*this);
    auto p1 = MAKE_PTR<int>(1);
    EXPECT_TRUE(ObjAnywhere::emplaceObjOK(p1, *this));
    auto p2 = MAKE_PTR<int>(2);
    EXPECT_FALSE(ObjAnywhere::emplaceObjOK(p2, *this)) << "REQ: replace failed";
    EXPECT_EQ(p1.get(), ObjAnywhere::getObj<int>().get()) << "REQ: refuse replacement";

    EXPECT_TRUE(ObjAnywhere::emplaceObjOK<int>(nullptr, *this));
    EXPECT_TRUE(ObjAnywhere::emplaceObjOK(p2, *this));
    EXPECT_EQ(p2.get(), ObjAnywhere::getObj<int>().get()) << "REQ: explicit rm then set ok";
    ObjAnywhere::deinit();
}
TEST_F(ObjAnywhereTest, noSet_getNull)
{
    ObjAnywhere::init(*this);
    EXPECT_EQ(nullptr, ObjAnywhere::getObj<int>().get()) << "REQ: get null";
    ObjAnywhere::deinit();
}
TEST_F(ObjAnywhereTest, deinit_getNull)
{
    EXPECT_EQ(nullptr, ObjAnywhere::getObj<int>().get()) << "REQ: get null";
}
TEST_F(ObjAnywhereTest, deinitThenSet_getNull)
{
    EXPECT_FALSE(ObjAnywhere::emplaceObjOK(MAKE_PTR<int>(1234), *this)) << "REQ: no init so set fail";
    EXPECT_EQ(nullptr, ObjAnywhere::getObj<int>().get()) << "REQ: get null";
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
    EXPECT_TRUE(ObjAnywhere::emplaceObjOK(MAKE_PTR<TestObj>(isDestructed), *this));
    EXPECT_FALSE(isDestructed);

    ObjAnywhere::deinit();
    EXPECT_TRUE(isDestructed) << "REQ: destruct correctly";
}
TEST_F(ObjAnywhereTest, destructBySetNull)
{
    bool isDestructed;
    ObjAnywhere::init(*this);

    EXPECT_TRUE(ObjAnywhere::emplaceObjOK(MAKE_PTR<TestObj>(isDestructed), *this));
    EXPECT_TRUE(ObjAnywhere::emplaceObjOK(S_PTR<TestObj>(), *this));  // set null
    EXPECT_TRUE(isDestructed) << "REQ: destruct correctly";
    EXPECT_EQ(nullptr, ObjAnywhere::getObj<TestObj>().get()) << "REQ: destruct TestObj";

    EXPECT_FALSE(ObjAnywhere::emplaceObjOK(S_PTR<TestObj>(), *this)) << "REQ: rm again NOK";
    EXPECT_EQ(0u, ObjAnywhere::nObj());

    ObjAnywhere::deinit();
}

#define INIT_DEINIT
// ***********************************************************************************************
TEST_F(ObjAnywhereTest, GOLD_init_deinit)
{
    ObjAnywhere::init(*this);
    EXPECT_TRUE(ObjAnywhere::isInit());

    ObjAnywhere::deinit();
    EXPECT_FALSE(ObjAnywhere::isInit());
}
TEST_F(ObjAnywhereTest, dup_init_deinit)
{
    ObjAnywhere::init(*this);
    ObjAnywhere::init(*this);
    EXPECT_TRUE(ObjAnywhere::isInit());

    ObjAnywhere::deinit();
    ObjAnywhere::deinit();
    EXPECT_FALSE(ObjAnywhere::isInit());
}
TEST_F(ObjAnywhereTest, ignore_dup_init)
{
    ObjAnywhere::init(*this);
    auto pChar = MAKE_PTR<char>('a');
    ObjAnywhere::emplaceObjOK(pChar, *this);
    ObjAnywhere::init(*this);  // req: ignore dup init
    EXPECT_EQ(pChar.get(), ObjAnywhere::getObj<char>().get());
    ObjAnywhere::deinit();
}
TEST_F(ObjAnywhereTest, default_is_deinit)
{
    EXPECT_FALSE(ObjAnywhere::isInit());
    EXPECT_EQ(0u, ObjAnywhere::nObj());
}

}  // namespace
