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
    EXPECT_EQ(42, *(one.cast_get<int>())) << "REQ: valid construct & get";

    *(one.cast_get<int>()) = 43;
    EXPECT_EQ(43, *(one.cast_get<int>())) << "REQ: valid update";

    EXPECT_EQ(one.cast_get<void>(), one.cast_get<int>()) << "REQ: valid get (void)";
    // one.cast_get<unsigned>();  // REQ: invalid cast_get() will fail compile
}
TEST(SafePtrTest, GOLD_cp_get)
{
    auto one = make_safe<int>(42);
    auto two(one);
    EXPECT_EQ(42, *(two.cast_get<int>())) << "REQ: valid cp & get";

    *(one.cast_get<int>()) = 43;
    EXPECT_EQ(43, *(two.cast_get<int>())) << "REQ: valid update via sharing";
}
TEST(SafePtrTest, GOLD_assign_get)
{
    SafePtr<int> one;
    EXPECT_EQ(nullptr, one.cast_get<int>()) << "REQ: construct null";

    auto two = make_safe<int>(42);
    one = two;
    EXPECT_EQ(42, *(one.cast_get<int>()))  << "REQ: valid assign & get";

    two = SafePtr<int>();
    EXPECT_EQ(42,    *(one.cast_get<int>())) << "REQ: valid get after shared is reset";
    EXPECT_EQ(nullptr, two.cast_get<int>())  << "REQ: assign to null";
}
TEST(SafePtrTest, GOLD_mv_get)
{
    SafePtr<int> one;
    auto two = make_safe<int>(42);
    one = move(two);
    EXPECT_EQ(42,    *(one.cast_get<int>())) << "REQ: valid move assignment & get";
    EXPECT_EQ(nullptr, two.cast_get<int>())  << "REQ: move src is null";

    SafePtr<int> three(move(one));
    EXPECT_EQ(42,    *(three.cast_get<int>())) << "REQ: valid move cp & get";
    EXPECT_EQ(nullptr, one.cast_get<int>())    << "REQ: move src is null";
}

#define DERIVE_VOID
// ***********************************************************************************************
struct Base                   { virtual int value()  { return 0; } };
struct Derive : public Base   { int value() override { return 1; } };
struct D2     : public Derive { int value() override { return 2; } };

TEST(SafePtrTest, GOLD_base_get)
{
    SafePtr<Base> b = make_safe<Derive>();
    EXPECT_EQ(1,       b.cast_get<Base>()  ->value()) << "REQ: Base can ptr Derive & get virtual";
    EXPECT_EQ(1,       b.cast_get<Derive>()->value()) << "REQ: get self";
    EXPECT_EQ(nullptr, b.cast_get<D2>())              << "REQ: invalid";
}
TEST(SafePtrTest, castTo_baseDirection)
{
    SafePtr<D2> d2 = make_safe<D2>();
    EXPECT_EQ(2, d2.cast_get<D2>()    ->value()) << "req: get virtual";
    EXPECT_EQ(2, d2.cast_get<Derive>()->value()) << "REQ: safe to Base direction";
    EXPECT_EQ(2, d2.cast_get<Base>()  ->value()) << "req: safe to Base direction";
    EXPECT_EQ(d2.cast_get(), d2.cast_get<D2>())  << "REQ: safe to void";

    SafePtr<Derive> d = d2;
    EXPECT_EQ(2, d.cast_get<Base>()  ->value()) << "req: safe to Base direction";
    EXPECT_EQ(2, d.cast_get<Derive>()->value()) << "req: safe to self";
    EXPECT_EQ(2, d.cast_get<D2>()    ->value()) << "REQ: safe to origin";
    EXPECT_EQ(d.cast_get(), d.cast_get<Base>()) << "req: valid to void";

    SafePtr<Base> b = d2;
    EXPECT_EQ(2,   b.cast_get<Base>()  ->value()) << "req: valid self";
    //EXPECT_EQ(2, b.cast_get<Derive>()->value()) << "safe but not support yet, worth to support?";
    EXPECT_EQ(2,   b.cast_get<D2>()    ->value()) << "req: safe to origin";
    EXPECT_EQ(b.cast_get(), b.cast_get<Base>())   << "req: valid get";

    SafePtr<> v = d;
    EXPECT_EQ(2, v.cast_get<D2>()    ->value()) << "req: safe to origin:  (D2->Derive->)void->D2";
    EXPECT_EQ(2, v.cast_get<Derive>()->value()) << "REQ: safe to preVoid: (D2->Derive->)void->Derive";
}
TEST(SafePtrTest, castTo_origin_preVoid)
{
    SafePtr<> v = make_safe<D2>();
    EXPECT_EQ(&typeid(D2), v.realType());
    EXPECT_EQ(&typeid(D2), v.preVoidType());
    EXPECT_EQ(2, v.cast_get<D2>()->value()) << "REQ: safe to origin/preVoid";

    SafePtr<> vv = v;
    EXPECT_EQ(&typeid(D2), vv.realType());
    EXPECT_EQ(&typeid(D2), vv.preVoidType());
    EXPECT_EQ(2,   vv.cast_get<D2>()    ->value()) << "REQ: void->void shall not lose origin & preVoid";
    //EXPECT_EQ(2, vv.cast_get<Derive>()->value()) << "safe but not support yet, worth to support?";
    //EXPECT_EQ(2, vv.cast_get<Base>()  ->value()) << "safe but not support yet, worth to support?";

    SafePtr<D2> d2 = vv;
    EXPECT_EQ(&typeid(D2), d2.realType());
    EXPECT_EQ(&typeid(D2), d2.preVoidType());
    EXPECT_EQ(2, d2.cast_get<D2>()->value()) << "REQ: D2->void->void->D2 shall not lose origin & preVoid";

    SafePtr<Derive> d = vv;
    EXPECT_EQ(&typeid(Derive), d.realType()) << "lose origin - don't know Derive is base via typeid(D2)";
    EXPECT_EQ(nullptr, d.preVoidType())      << "lose origin so also lose preVoidType_";
    EXPECT_EQ(nullptr, d.cast_get<void>())   << "lose origin so also lose pT_";
}
TEST(SafePtrTest, inc_cov)
{
    SafePtr<Derive> d;
    SafePtr<Base> b = d;
    EXPECT_EQ(nullptr,       b.preVoidType()) << "REQ: type before to void";
    EXPECT_EQ(&typeid(Base), b.realType())    << "REQ: origin type since d is nullptr";
}

#define ARRAY
// ***********************************************************************************************
TEST(SafePtrTest, GOLD_construct_array)
{
    //make_safe<ints[]>(10);
}

}  // namespace
