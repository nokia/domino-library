/**
 * Copyright 2024 Nokia. All rights reserved.
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include <gtest/gtest.h>

#include "SafePtr.hpp"

namespace RLib
{
#define NEW_CP_MV_GET
// ***********************************************************************************************
TEST(SafePtrTest, GOLD_construct_get)
{
    SafePtr t1(make_shared<int>(42));
    EXPECT_EQ(42, *(t1.get<int>()))        << "REQ: valid construct & get (original type)";
    EXPECT_EQ(t1.get(), t1.get<int>())     << "REQ: valid get (void)";
    EXPECT_EQ(nullptr, t1.get<unsigned>()) << "REQ: invalid get (diff type)";
}
TEST(SafePtrTest, GOLD_cp_get)
{
    SafePtr t1(make_shared<int>(42));
    SafePtr t2 = t1;
    EXPECT_EQ(t1.get(), t2.get<int>()) << "REQ: valid cp & get (original type)";
    EXPECT_EQ(t2.get(), t2.get<int>()) << "REQ: valid get (void)";
    EXPECT_EQ(nullptr, t2.get<bool>()) << "REQ: invalid get (diff type)";
}
TEST(SafePtrTest, GOLD_mv_get)
{
    SafePtr t1;
    EXPECT_EQ(nullptr, t1.get())      << "REQ: construct null & get it";
    EXPECT_EQ(nullptr, t1.get<int>()) << "REQ: invalid get";

    SafePtr t2(make_shared<char>('a'));
    t1 = t2;
    EXPECT_EQ('a', *(t1.get<char>())) << "REQ: valid get (original type)";

    SafePtr t3(make_shared<bool>(true));
    t1 = t3;
    EXPECT_TRUE(*(t1.get<bool>()))    << "REQ: valid get (original type)";
    EXPECT_EQ('a', *(t2.get<char>())) << "REQ: replacement not impact original";

    t3 = SafePtr();
    EXPECT_EQ(nullptr, t3.get())      << "REQ: can reset SafePtr";
    EXPECT_TRUE(*(t1.get<bool>()))    << "REQ: lifecycle valid";
}

}  // namespace
