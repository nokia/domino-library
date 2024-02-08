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
// - safe create: null & make_safe only
// - safe ship  : cp/mv/= base<-derive(s); short->int?
// - safe store : cast any<->void
// - safe use   : get self T*
#define CREATE_GET
// ***********************************************************************************************
TEST(SafePtrTest, GOLD_construct_get)
{
    auto one = make_safe<int>(42);
    EXPECT_EQ(42, *(one.get<int>())) << "REQ: valid construct & get";

    *(one.get<int>()) = 43;
    EXPECT_EQ(43, *(one.get<int>())) << "REQ: valid update";

    EXPECT_EQ(one.get<void>(), one.get<int>()) << "REQ: valid get (void)";
    // one.get<unsigned>();  // REQ: invalid get() will fail compile
}
TEST(SafePtrTest, GOLD_cp_get)
{
    auto one = make_safe<int>(42);
    auto two(one);
    EXPECT_EQ(42, *(two.get<int>())) << "REQ: valid cp & get";

    *(one.get<int>()) = 43;
    EXPECT_EQ(43, *(two.get<int>())) << "REQ: valid update via sharing";
}
TEST(SafePtrTest, GOLD_assign_get)
{
    SafePtr<int> one;
    EXPECT_EQ(nullptr, one.get<int>()) << "REQ: construct null";

    auto two = make_safe<int>(42);
    one = two;
    EXPECT_EQ(42, *(one.get<int>())) << "REQ: valid assign & get";

    two = SafePtr<int>();
    EXPECT_EQ(nullptr, two.get<int>()) << "REQ: assign to null";
    EXPECT_EQ(42, *(one.get<int>())) << "REQ: valid get after shared is reset";
}
TEST(SafePtrTest, GOLD_mv_get)
{
    SafePtr<int> one;
    auto two = make_safe<int>(42);
    one = move(two);
    EXPECT_EQ(42, *(one.get<int>())) << "REQ: valid move & get";
    EXPECT_EQ(nullptr, two.get<int>()) << "REQ: move src is null";

    SafePtr<int> three(move(one));
    EXPECT_EQ(42, *(three.get<int>())) << "REQ: valid move & get";
    EXPECT_EQ(nullptr, one.get<int>()) << "REQ: move src is null";
}

#define DERIVE_VOID
// ***********************************************************************************************
struct Base                   { virtual int value()  { return 0; } };
struct Derive : public Base   { int value() override { return 1; } };
struct D2     : public Derive { int value() override { return 2; } };

TEST(SafePtrTest, GOLD_base_get)
{
    SafePtr<Base> b = make_safe<Derive>();
    EXPECT_NE(nullptr, b.get<Base>()) << "REQ: Base can ptr Derive";
    EXPECT_EQ(1, b.get<Base>()->value()) << "REQ: get virtual";
}
TEST(SafePtrTest, derive_to_base)
{
    SafePtr<D2> d2 = make_safe<D2>();

    EXPECT_EQ(2, d2.get<D2>()    ->value()) << "REQ: valid & get virtual";
    EXPECT_EQ(2, d2.get<Derive>()->value()) << "REQ: valid to Base direction";
    EXPECT_EQ(2, d2.get<Base>()  ->value()) << "REQ: valid to Base direction";
    EXPECT_EQ(d2.get<void>(), d2.get<D2>()) << "REQ: valid to void";

    SafePtr<Derive> d = d2;
    EXPECT_EQ(2, d.get<Base>()  ->value())  << "REQ: valid to Base direction";
    EXPECT_EQ(2, d.get<Derive>()->value())  << "REQ: valid to self";
    //           d.get<D2>();                // fail to Derive direction - compile err
    EXPECT_EQ(d.get<void>(), d.get<Base>()) << "REQ: valid to void";

    SafePtr<Base> b = d2;
    EXPECT_EQ(2, b.get<Base>()  ->value())  << "REQ: valid self";
    //           b.get<Derive>();            // fail to Derive direction - compile err
    //           b.get<D2>();                // fail to Derive direction - compile err
    EXPECT_EQ(b.get<void>(), b.get<Base>()) << "REQ: valid get";
}
TEST(SafePtrTest, GOLD_void_back)
{
    SafePtr<D2> d2 = make_safe<D2>();
    SafePtr<void> v = d2;
    EXPECT_NE(nullptr, v.get<void>())        << "REQ: any -> void";
    EXPECT_EQ(2,       v.get<D2>()->value()) << "REQ: void back";
    EXPECT_EQ(nullptr, v.get<Derive>())      << "REQ: void -> unknown";
    EXPECT_EQ(nullptr, v.get<Base>())        << "REQ: void -> unknown";

    SafePtr<Derive> d = d2;
    v = d;
    EXPECT_EQ(nullptr, v.get<D2>())              << "REQ: void -> unknown";
    EXPECT_EQ(2,       v.get<Derive>()->value()) << "REQ: void back";
    EXPECT_EQ(nullptr, v.get<Base>())            << "REQ: void -> unknown";

    SafePtr<void> v2 = v;
    EXPECT_EQ(nullptr, v2.get<D2>())              << "REQ: void -> unknown";
    EXPECT_EQ(2,       v2.get<Derive>()->value()) << "REQ: void back";
    EXPECT_EQ(nullptr, v2.get<Base>())            << "REQ: void -> unknown";
}

#define ARRAY
// ***********************************************************************************************
TEST(SafePtrTest, GOLD_construct_array)
{
    //make_safe<ints[]>(10);
}

}  // namespace
