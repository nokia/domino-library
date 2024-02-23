/**
 * Copyright 2024 Nokia. All rights reserved.
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include <gtest/gtest.h>

#include "UniPtr.hpp"

namespace RLib
{
#define CREATE_GET
// ***********************************************************************************************
TEST(SafeAdrTest, GOLD_construct_get)
{
    auto one = make_safe<int>(42);
    EXPECT_EQ(42, *(one.cast_get())) << "REQ: valid construct & get";

    *(one.cast_get()) = 43;
    EXPECT_EQ(43, *(one.cast_get())) << "REQ: valid update";

    EXPECT_EQ(one.cast_get<void>(), one.cast_get()) << "REQ: valid get (void)";
    // one.cast_get<unsigned>();  // REQ: invalid cast_get() will fail compile
}
TEST(SafeAdrTest, GOLD_cp_get)
{
    auto one = make_safe<int>(42);
    auto two(one);
    EXPECT_EQ(42, *(two.cast_get())) << "REQ: valid cp & get";

    *(one.cast_get()) = 43;
    EXPECT_EQ(43, *(two.cast_get())) << "REQ: valid update via sharing";
}
TEST(SafeAdrTest, GOLD_assign_get)
{
    SafeAdr<int> one;
    EXPECT_EQ(nullptr, one.cast_get()) << "REQ: construct null";

    auto two = make_safe<int>(42);
    one = two;
    EXPECT_EQ(42, *(one.cast_get()))  << "REQ: valid assign & get";

    two = SafeAdr<int>();
    EXPECT_EQ(42,    *(one.cast_get())) << "REQ: valid get after shared is reset";
    EXPECT_EQ(nullptr, two.cast_get())  << "REQ: assign to null";
}
TEST(SafeAdrTest, GOLD_mv_get)
{
    SafeAdr<int> one;
    auto two = make_safe<int>(42);
    one = move(two);
    EXPECT_EQ(42,    *(one.cast_get())) << "REQ: valid move assignment & get";
    EXPECT_EQ(nullptr, two.cast_get())  << "REQ: move src is null";

    SafeAdr<int> three(move(one));
    EXPECT_EQ(42,    *(three.cast_get())) << "REQ: valid move cp & get";
    EXPECT_EQ(nullptr, one.cast_get())    << "REQ: move src is null";
}

#define DERIVE_VOID
// ***********************************************************************************************
struct Base                   { virtual int value()  { return 0; } };
struct Derive : public Base   { int value() override { return 1; } };
struct D2     : public Derive { int value() override { return 2; } };

TEST(SafeAdrTest, GOLD_base_get)
{
    SafeAdr<Base> b = make_safe<Derive>();
    EXPECT_EQ(1,       b.cast_get<Base>()  ->value()) << "REQ: Base can ptr Derive & get virtual";
    EXPECT_EQ(1,       b.cast_get<Derive>()->value()) << "REQ: get self";
    EXPECT_EQ(nullptr, b.cast_get<D2>())              << "REQ: invalid";
}
TEST(SafeAdrTest, castTo_baseDirection)
{
    SafeAdr<D2> d2 = make_safe<D2>();
    EXPECT_EQ(2, d2.cast_get<D2>()    ->value()) << "req: get virtual";
    EXPECT_EQ(2, d2.cast_get<Derive>()->value()) << "REQ: safe to Base direction";
    EXPECT_EQ(2, d2.cast_get<Base>()  ->value()) << "req: safe to Base direction";
    EXPECT_EQ(d2.cast_get(), d2.cast_get<D2>())  << "REQ: safe to void";

    SafeAdr<Derive> d = d2;
    EXPECT_EQ(2, d.cast_get<Base>()  ->value()) << "req: safe to Base direction";
    EXPECT_EQ(2, d.cast_get<Derive>()->value()) << "req: safe to self";
    EXPECT_EQ(2, d.cast_get<D2>()    ->value()) << "REQ: safe to origin";
    EXPECT_EQ(d.cast_get(), d.cast_get<Base>()) << "req: valid to void";

    SafeAdr<Base> b = d2;
    EXPECT_EQ(2,   b.cast_get<Base>()  ->value()) << "req: valid self";
    //EXPECT_EQ(2, b.cast_get<Derive>()->value()) << "safe but not support yet, worth to support?";
    EXPECT_EQ(2,   b.cast_get<D2>()    ->value()) << "req: safe to origin";
    EXPECT_EQ(b.cast_get(), b.cast_get<Base>())   << "req: valid get";

    SafeAdr<> v = d;
    EXPECT_EQ(2, v.cast_get<D2>()    ->value()) << "req: safe to origin:  (D2->Derive->)void->D2";
    EXPECT_EQ(2, v.cast_get<Derive>()->value()) << "REQ: safe to preVoid: (D2->Derive->)void->Derive";
}
TEST(SafeAdrTest, castTo_origin_preVoid)
{
    SafeAdr<> v = make_safe<D2>();
    EXPECT_EQ(&typeid(D2), v.realType());
    EXPECT_EQ(&typeid(D2), v.preVoidType());
    EXPECT_EQ(2, v.cast_get<D2>()->value()) << "REQ: safe to origin/preVoid";

    SafeAdr<> vv = v;
    EXPECT_EQ(&typeid(D2), vv.realType());
    EXPECT_EQ(&typeid(D2), vv.preVoidType());
    EXPECT_EQ(2,   vv.cast_get<D2>()    ->value()) << "REQ: void->void shall not lose origin & preVoid";
    //EXPECT_EQ(2, vv.cast_get<Derive>()->value()) << "safe but not support yet, worth to support?";
    //EXPECT_EQ(2, vv.cast_get<Base>()  ->value()) << "safe but not support yet, worth to support?";

    SafeAdr<D2> d2 = vv;
    EXPECT_EQ(&typeid(D2), d2.realType());
    EXPECT_EQ(&typeid(D2), d2.preVoidType());
    EXPECT_EQ(2, d2.cast_get<D2>()->value()) << "REQ: D2->void->void->D2 shall not lose origin & preVoid";

    SafeAdr<Derive> d = vv;
    EXPECT_EQ(&typeid(Derive), d.realType()) << "lose origin - don't know Derive is base via typeid(D2)";
    EXPECT_EQ(nullptr, d.preVoidType())      << "lose origin so also lose preVoidType_";
    EXPECT_EQ(nullptr, d.cast_get<void>())   << "lose origin so also lose pT_";
}
TEST(SafeAdrTest, inc_cov)
{
    SafeAdr<Derive> d;
    SafeAdr<Base> b = d;
    EXPECT_EQ(nullptr,       b.preVoidType()) << "REQ: type before to void";
    EXPECT_EQ(&typeid(Base), b.realType())    << "REQ: origin type since d is nullptr";
}

#define CONST_AND_BACK
// ***********************************************************************************************
struct D
{
    int value()       { return 100; }
    int value() const { return 0; }
};

TEST(SafeAdrTest, GOLD_const_and_back)
{
    auto safe_d  = make_safe<D>();
    auto share_d = make_shared<D>();

    EXPECT_EQ(100, safe_d ->value()) << "REQ: call non-const";
    EXPECT_EQ(100, share_d->value()) << "REQ: call non-const";

    SafeAdr   <const D> safe_const_d  = safe_d;
    shared_ptr<const D> share_const_d = share_d;

    EXPECT_EQ(0, safe_const_d ->value()) << "REQ: cp succ & call const)";
    EXPECT_EQ(0, share_const_d->value()) << "REQ: cp succ & call const)";

    // SafeAdr<D>    safe_dd  = safe_const_d;   // REQ: compile err to cp from const to non
    // shared_ptr<D> share_dd = share_const_d;  // REQ: compile err to cp from const to non

    const SafeAdr   <D> const_safe_d  = safe_d;
    const shared_ptr<D> const_share_d = share_d;

    EXPECT_EQ(100, const_safe_d ->value()) << "REQ: cp succ & call NON-const (same as shared_ptr)";
    EXPECT_EQ(100, const_share_d->value()) << "call NON-const since all members are NON-const except 'this'";
}

#define ARRAY
// ***********************************************************************************************
TEST(SafeAdrTest, GOLD_construct_array)
{
    //make_safe<ints[]>(10);
}

}  // namespace
