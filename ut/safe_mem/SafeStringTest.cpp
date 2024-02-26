/**
 * Copyright 2024 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <gtest/gtest.h>
#include <unordered_map>

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

#define UNORDERED_MAP_ETC
// ***********************************************************************************************
TEST(SafeStringTest, GOLD_asKeyOf_unorderedMap)
{
    unordered_map<SafeString, int> store;
    EXPECT_EQ(0, store.size()) << "REQ: SafeString can be key";

    store[SafeString()] = 100;
    EXPECT_EQ(nullptr, *SafeString());
    EXPECT_EQ(1, store.size()) << "REQ: key=nullptr is OK";

    store[SafeString()] = 200;
    EXPECT_EQ(1,   store.size())        << "REQ: no dup key";
    EXPECT_EQ(200, store[SafeString()]) << "REQ: dup key overwrites value";

    store[SafeString("hello")] = 300;
    EXPECT_EQ(2,   store.size())               << "REQ: store diff keys";
    EXPECT_EQ(300, store[SafeString("hello")]) << "REQ: get normal key's value";
}

}  // namespace
