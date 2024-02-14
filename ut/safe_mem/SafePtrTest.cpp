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
#define CREATE_GET
// ***********************************************************************************************
TEST(SafePtrTest, GOLD_construct_get)
{
    auto one = make_safe<int>(42);
    EXPECT_EQ(42, *(one.cast<int>()))  << "REQ: valid construct & get";

    *(one.cast<int>()) = 43;
    EXPECT_EQ(43, *(one.cast<int>())) << "REQ: valid update";

    EXPECT_EQ(one.cast<void>(), one.cast<int>()) << "REQ: valid get (void)";
    // one.cast<unsigned>();  // REQ: invalid cast() will fail compile
}
TEST(SafePtrTest, GOLD_cp_get)
{
    auto one = make_safe<int>(42);
    auto two(one);
    EXPECT_EQ(42, *(two.cast<int>()))  << "REQ: valid cp & get";

    *(one.cast<int>()) = 43;
    EXPECT_EQ(43, *(two.cast<int>())) << "REQ: valid update via sharing";
}
TEST(SafePtrTest, GOLD_assign_get)
{
    SafePtr<int> one;
    EXPECT_EQ(nullptr, one.cast<int>())   << "REQ: construct null";

    auto two = make_safe<int>(42);
    one = two;
    EXPECT_EQ(42, *(one.cast<int>()))  << "REQ: valid assign & get";

    two = SafePtr<int>();
    EXPECT_EQ(42,    *(one.cast<int>())) << "REQ: valid get after shared is reset";
    EXPECT_EQ(nullptr, two.cast<int>())  << "REQ: assign to null";
}
TEST(SafePtrTest, GOLD_mv_get)
{
    SafePtr<int> one;
    auto two = make_safe<int>(42);
    one = move(two);
    EXPECT_EQ(42,    *(one.cast<int>())) << "REQ: valid move assignment & get";
    EXPECT_EQ(nullptr, two.cast<int>())  << "REQ: move src is null";

    SafePtr<int> three(move(one));
    EXPECT_EQ(42,    *(three.cast<int>())) << "REQ: valid move cp & get";
    EXPECT_EQ(nullptr, one.cast<int>())    << "REQ: move src is null";
}

#define DERIVE_VOID
// ***********************************************************************************************
struct Base                   { virtual int value()  { return 0; } };
struct Derive : public Base   { int value() override { return 1; } };
struct D2     : public Derive { int value() override { return 2; } };

TEST(SafePtrTest, GOLD_base_get)
{
    SafePtr<Base> b = make_safe<Derive>();
    EXPECT_NE(nullptr, b.cast<Base>())    << "REQ: Base can ptr Derive";
    EXPECT_EQ(1, b.cast<Base>()->value()) << "REQ: get virtual";

    EXPECT_EQ(nullptr, b.cast<D2>()) << "REQ: invalid";
}
TEST(SafePtrTest, GOLD_safeConvert_base_derive)
{
    SafePtr<D2> d2 = make_safe<D2>();
    EXPECT_EQ(nullptr,     d2.preVoidType()) << "REQ: type before to void";
    EXPECT_EQ(&typeid(D2), d2.realType())    << "REQ: origin type";

    EXPECT_EQ(2, d2.cast<D2>()    ->value()) << "req: get virtual";
    EXPECT_EQ(2, d2.cast<Derive>()->value()) << "REQ: safe to Base direction";
    EXPECT_EQ(2, d2.cast<Base>()  ->value()) << "req: safe to Base direction";
    EXPECT_EQ(d2.cast(), d2.cast<D2>()) << "REQ: safe to void";

    SafePtr<Derive> d = d2;
    EXPECT_EQ(nullptr,     d.preVoidType()) << "req: type before to void";
    EXPECT_EQ(&typeid(D2), d.realType())    << "req: origin type";

    EXPECT_EQ(2, d.cast<Base>()  ->value())  << "req: safe to Base direction";
    EXPECT_EQ(2, d.cast<Derive>()->value())  << "req: safe to self";
    EXPECT_EQ(2, d.cast<D2>()    ->value())  << "REQ: safe to origin";
    EXPECT_EQ(d.cast(), d.cast<Base>()) << "req: valid to void";

    SafePtr<Base> b = d2;
    EXPECT_EQ(nullptr,     b.preVoidType()) << "req: type before to void";
    EXPECT_EQ(&typeid(D2), b.realType())    << "req: origin type";

    EXPECT_EQ(2,   b.cast<Base>()  ->value()) << "req: valid self";
    //EXPECT_EQ(2, b.cast<Derive>()->value()) << "safe but not support yet, worth to support?";
    EXPECT_EQ(2,   b.cast<D2>()    ->value()) << "req: safe to origin";
    EXPECT_EQ(b.cast(), b.cast<Base>()) << "req: valid get";
}
TEST(SafePtrTest, GOLD_void_back)
{
    SafePtr<D2> d2 = make_safe<D2>();
    SafePtr<> v = d2;
    EXPECT_EQ(&typeid(D2), v.preVoidType()) << "REQ: type before to void";
    EXPECT_EQ(&typeid(D2), v.realType())    << "req: origin type";

    EXPECT_NE(nullptr, v.cast())                  << "req: safe any to void";
    EXPECT_EQ(2,       v.cast<D2>()->value())     << "REQ: safe void back";
    //EXPECT_EQ(2,     v.cast<Derive>()->value()) << "safe but not support yet, worth to support?";
    //EXPECT_EQ(2,     v.cast<Base>()->value())   << "safe but not support yet, worth to support?";

    SafePtr<Derive> d = d2;
    EXPECT_EQ(nullptr, d.preVoidType())  << "req: type before to void";
    EXPECT_EQ(&typeid(D2), d.realType()) << "req: origin type";
    v = d;
    EXPECT_EQ(&typeid(Derive), v.preVoidType()) << "REQ: type before to void";
    EXPECT_EQ(&typeid(D2),     v.realType())    << "req: origin type";

    EXPECT_EQ(2,   v.cast<D2>()->value())     << "REQ: safe to origin";
    EXPECT_EQ(2,   v.cast<Derive>()->value()) << "REQ: safe (D2 to Derive to) void to Derive";
    //EXPECT_EQ(2, v.cast<Base>()->value())   << "safe but not support yet, worth to support?";

    SafePtr<> v2 = v;
    EXPECT_EQ(&typeid(Derive), v2.preVoidType()) << "REQ: type before to void";
    EXPECT_EQ(&typeid(D2),     v2.realType())    << "req: origin type";

    EXPECT_EQ(2,   v2.cast<D2>()->value())     << "req: safe to origin";
    EXPECT_EQ(2,   v2.cast<Derive>()->value()) << "REQ: safe (D2 to Derive to void to) void(AGAIN) to Derive";
    //EXPECT_EQ(2, v2.cast<Base>()->value())   << "safe but not support yet, worth to support?";

    SafePtr<D2> d2_2 = v;
    EXPECT_EQ(nullptr,     d2_2.preVoidType()) << "REQ: type before to void";
    EXPECT_EQ(&typeid(D2), d2_2.realType())    << "req: origin type";
}
TEST(SafePtrTest, inc_cov)
{
    SafePtr<Derive> d;
    SafePtr<Base> b = d;
    EXPECT_EQ(nullptr,       b.preVoidType()) << "REQ: type before to void";
    EXPECT_EQ(&typeid(Base), b.realType())    << "REQ: origin type";
}

#define ARRAY
// ***********************************************************************************************
TEST(SafePtrTest, GOLD_construct_array)
{
    //make_safe<ints[]>(10);
}

}  // namespace
