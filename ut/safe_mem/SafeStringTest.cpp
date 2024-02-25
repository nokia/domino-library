/**
 * Copyright 2024 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <gtest/gtest.h>

#include "SafeString.hpp"

using namespace std;

namespace RLib
{
#define STAR_GET
// ***********************************************************************************************
TEST(SafeStringTest, GOLD_starGet_isMemSafe_afterDelOrigin)
{
    SafeString hc = "hello";
    auto get = *hc;
    EXPECT_EQ(2, get.use_count()) << "REQ: shared";

    hc = SafeString();  // del
    EXPECT_EQ(1, get.use_count()) << "REQ: safe usage after origin deleted";
}
TEST(SafeStringTest, GOLD_starGetNull_isMemSafe)
{
    SafeString null;
    auto get = *null;
    EXPECT_EQ(0, get.use_count()) << "REQ: shared";

    null = SafeString("hello");
    EXPECT_EQ("hello", **null)    << "REQ: re-assign";
    EXPECT_EQ(0, get.use_count()) << "REQ: no impact what got";
}

#define CONTENT_GET
// ***********************************************************************************************
TEST(SafeStringTest, GOLD_contentGet_isMemSafe)
{
    SafeString hc = "hello";
    auto&& get = hc();
    EXPECT_EQ("hello", get) << "REQ: get content";

    hc = SafeString();  // del
    EXPECT_EQ("hello", get) << "REQ: still safe of what got after origin deleted";
}
TEST(SafeStringTest, GOLD_contentGetNull_isMemSafe)
{
    SafeString null;
    auto&& get = null();
    EXPECT_EQ("null shared_ptr<>", get) << "REQ: no crash to get nullptr";

    null = SafeString("hello");
    EXPECT_EQ("hello", **null) << "REQ: re-assign";
    EXPECT_EQ("null shared_ptr<>", get) << "REQ: no impact what got";
}

#define COMPARE
// ***********************************************************************************************
TEST(SafeStringTest, GOLD_compare)
{
    SafeString hc = "hello";  // 1st constructor
    EXPECT_EQ(hc(), hc()) << "REQ: compare self";

    SafeString hc2 = hc;  // cp constructor
    EXPECT_EQ(hc(), hc2()) << "REQ: compare same pointee";

    SafeString hs = string("hello");  // 2nd constructor
    EXPECT_EQ(hc(), hs()) << "REQ: same content";

    SafeString null;  // 3rd constructor
    EXPECT_FALSE(hc()   == null()) << "REQ: != nullptr";
    EXPECT_TRUE (null() != hs()  ) << "REQ: nullptr !=";
}

}  // namespace
