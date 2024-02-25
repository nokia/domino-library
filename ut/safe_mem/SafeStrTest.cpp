/**
 * Copyright 2024 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <gtest/gtest.h>

#include "SafeStr.hpp"

using namespace std;

namespace RLib
{
#define STAR_GET
// ***********************************************************************************************
TEST(SafeStrTest, GOLD_starGet_isMemSafe_afterDelOrigin)
{
    SafeStr hc = "hello";
    auto get = *hc;
    EXPECT_EQ(2, get.use_count()) << "REQ: shared";

    hc = SafeStr();  // del
    EXPECT_EQ(1, get.use_count()) << "REQ: safe usage after origin deleted";
}
TEST(SafeStrTest, GOLD_starGetNull_isMemSafe)
{
    SafeStr null;
    auto get = *null;
    EXPECT_EQ(0, get.use_count()) << "REQ: shared";

    null = SafeStr("hello");
    EXPECT_EQ("hello", **null) << "REQ: re-assign";
    EXPECT_EQ(0, get.use_count()) << "REQ: no impact what got";
}

#define CONTENT_GET
// ***********************************************************************************************
TEST(SafeStrTest, GOLD_contentGet_isMemSafe)
{
    SafeStr hc = "hello";
    auto&& get = hc();
    EXPECT_EQ("hello", get) << "REQ: get content";

    hc = SafeStr();  // del
    EXPECT_EQ("hello", get) << "REQ: still safe of what got after origin deleted";
}
TEST(SafeStrTest, GOLD_contentGetNull_isMemSafe)
{
    SafeStr null;
    auto&& get = null();
    EXPECT_EQ("null shared_ptr<>", get) << "REQ: no crash to get nullptr";

    null = SafeStr("hello");
    EXPECT_EQ("hello", **null) << "REQ: re-assign";
    EXPECT_EQ("null shared_ptr<>", get) << "REQ: no impact what got";
}

#define COMPARE
// ***********************************************************************************************
TEST(SafeStrTest, GOLD_compare)
{
    SafeStr hc = "hello";  // 1st constructor
    EXPECT_EQ(hc(), hc()) << "REQ: compare self";

    SafeStr hc2 = hc;  // cp constructor
    EXPECT_EQ(hc(), hc2()) << "REQ: compare same pointee";

    SafeStr hs = string("hello");  // 2nd constructor
    EXPECT_EQ(hc(), hs()) << "REQ: same content";

    SafeStr null;  // 3rd constructor
    EXPECT_FALSE(hc()   == null()) << "REQ: != nullptr";
    EXPECT_TRUE (null() != hs()  ) << "REQ: nullptr !=";
}

}  // namespace
