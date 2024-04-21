/**
 * Copyright 2024 Nokia. All rights reserved.
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include <gtest/gtest.h>
#include <map>
#include <unordered_map>

#include "SafeAdr.hpp"

namespace RLib
{
#define CREATE_GET
// ***********************************************************************************************
TEST(SafeAdrTest, GOLD_create_get_cast)
{
    auto v = SafeAdr<>();
    EXPECT_EQ(nullptr, v.get()) << "REQ: create default=null";
    EXPECT_EQ(&typeid(void), v.realType()) << "REQ: default type=void";

    auto c = SafeAdr<char>(nullptr);
    EXPECT_EQ(nullptr, c.get()) << "REQ: create null";
    EXPECT_EQ(&typeid(char), c.realType()) << "REQ: type=char";

    auto one = make_safe<int>(42);
    EXPECT_EQ(42, *(one.get())) << "REQ: valid construct & get";
    EXPECT_EQ(&typeid(int), one.realType()) << "REQ: type=int";

    *(one.get()) = 43;
    EXPECT_EQ(43, *(one.get())) << "REQ: valid update";
}
TEST(SafeAdrTest, GOLD_cp_get)
{
    auto one = make_safe<int>(42);
    auto two(one);
    EXPECT_EQ(42, *(two.get())) << "REQ: valid cp & get";

    *(one.get()) = 43;
    EXPECT_EQ(43, *(two.get())) << "REQ: valid update via sharing";
}
TEST(SafeAdrTest, GOLD_assign_get)
{
    SafeAdr<int> one;
    auto two = make_safe<int>(42);
    one = two;
    EXPECT_EQ(42, *(one.get())) << "REQ: valid assign & get";

    two = SafeAdr<int>();
    EXPECT_EQ(nullptr, two.get()) << "REQ: assign to null";
    EXPECT_EQ(42, *(one.get())) << "REQ: valid get after assigner is reset";
}
TEST(SafeAdrTest, GOLD_mv_get)
{
    auto one = make_safe<int>(42);
    SafeAdr<int> cp(one);
    SafeAdr<int> mv = move(one);
    EXPECT_EQ(42, *(mv.get())) << "REQ: valid move assignment & get";
    EXPECT_EQ(nullptr, one.get()) << "REQ: src is null";
    EXPECT_EQ(42, *(cp.get())) << "REQ: mv not impact cp";
}

#define DERIVE_VOID
// ***********************************************************************************************
struct Base                   { virtual int value()  { return 0; } };
struct Derive : public Base   { int value() override { return 1; } };
struct D2     : public Derive { int value() override { return 2; } };

TEST(SafeAdrTest, GOLD_base_get)
{
    SafeAdr<Base> b = make_safe<Derive>();
    EXPECT_EQ(1,    b.cast_get<Base>()  ->value()) << "REQ: Base can ptr Derive & get virtual";
    // EXPECT_EQ(1, b.cast_get<Derive>()->value()) << "REQ: compile err to cast Base->Derive, safer than ret null";
    // EXPECT_EQ(nullptr, b.cast_get<D2>())        << "REQ: compile err to cast (Derive->)Base->D2";
}
TEST(SafeAdrTest, castTo_baseDirection)
{
    SafeAdr<D2> d2 = make_safe<D2>();
    EXPECT_EQ(2, d2.cast_get<D2>()    ->value()) << "req: get self";
    EXPECT_EQ(2, d2.cast_get<Derive>()->value()) << "REQ: safe to Base direction";
    EXPECT_EQ(2, d2.cast_get<Base>()  ->value()) << "req: safe to Base direction";
    EXPECT_EQ(d2.get(), d2.cast_get<D2>())  << "REQ: safe to void";

    SafeAdr<Derive> d = d2;
    EXPECT_EQ(2,    d.cast_get<Base>()  ->value()) << "req: safe to Base direction";
    EXPECT_EQ(2,    d.cast_get<Derive>()->value()) << "req: safe to self";
    // EXPECT_EQ(2, d.cast_get<D2>()    ->value()) << "req: compile err to cast (D2->)Derive->D2";
    EXPECT_EQ(d.get(), d.cast_get<Base>()) << "req: valid to void";

    SafeAdr<Base> b = d2;
    EXPECT_EQ(2,    b.cast_get<Base>()  ->value()) << "req: valid self";
    // EXPECT_EQ(2, b.cast_get<Derive>()->value()) << "req: compile err to cast (D2->)Base->Derive";
    // EXPECT_EQ(2, b.cast_get<D2>()    ->value()) << "req: compile err to cast (D2->)Base->D2";
    EXPECT_EQ(b.get(), b.cast_get<Base>())   << "req: valid get";

    SafeAdr<> v = d;
    EXPECT_EQ(2, v.cast_get<D2>()    ->value()) << "req: safe to origin:  (D2->Derive->)void->D2";
    EXPECT_EQ(2, v.cast_get<Derive>()->value()) << "REQ: safe to preVoid: (D2->Derive->)void->Derive";
}
TEST(SafeAdrTest, origin_preVoid)
{
    SafeAdr<> v = make_safe<D2>();
    EXPECT_EQ(&typeid(D2), v.realType   ()) << "REQ: safe to origin";
    EXPECT_EQ(&typeid(D2), v.preVoidType()) << "REQ: safe to preVoid";

    SafeAdr<> vv = v;
    EXPECT_EQ(&typeid(D2), vv.realType   ()) << "REQ: void->void shall not lose origin";
    EXPECT_EQ(&typeid(D2), vv.preVoidType()) << "REQ: void->void shall not lose preVoid";

    SafeAdr<D2> d2 = vv;
    EXPECT_EQ(&typeid(D2), d2.realType   ()) << "REQ: D2->void->void->D2 shall not lose origin";
    EXPECT_EQ(&typeid(D2), d2.preVoidType()) << "REQ: D2->void->void->D2 shall not lose preVoid";

    SafeAdr<Derive> d = vv;  // REQ: compile err to cast (D2->void->)void->Derive
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

#define SAFE
// ***********************************************************************************************
TEST(SafeAdrTest, get_isMemSafe_afterDelOrigin)
{
    SafeAdr<string> safe = make_safe<string>("hello");
    shared_ptr<string> get = safe.get();
    EXPECT_EQ("hello", *get) << "get succ";

    safe = nullptr;
    EXPECT_EQ("hello", *get) << "REQ: what got is still OK";

    get = safe.get();
    EXPECT_EQ(nullptr, get) << "REQ: get nullptr OK";
}
TEST(SafeAdrTest, getPtr_isMemSafe_afterDelOrigin)
{
    SafeAdr<string> safe = make_safe<string>("hello");
    shared_ptr<string> get = safe.operator->();  // diff
    EXPECT_EQ("hello", *get) << "REQ: get succ";

    safe = nullptr;
    EXPECT_EQ("hello", *get) << "REQ: what got is still OK";

    get = safe.operator->();
    EXPECT_EQ(nullptr, get) << "REQ: get nullptr OK";
}

#define SUPPORT_MAP
// ***********************************************************************************************
TEST(SafeAdrTest, GOLD_asKeyOf_unorderedMap)
{
    unordered_map<SafeAdr<string>, int> store;
    EXPECT_EQ(0, store.size()) << "REQ: SafeAdr can be key";

    store[SafeAdr<string>()] = 100;
    EXPECT_EQ(nullptr, SafeAdr<string>().get());
    EXPECT_EQ(1, store.size()) << "REQ: key=nullptr is OK";

    store[SafeAdr<string>()] = 200;
    EXPECT_EQ(1,   store.size()) << "REQ: no dup key";
    EXPECT_EQ(200, store[SafeAdr<string>()]) << "REQ: dup key overwrites value";

    store[SafeAdr<string>(make_safe<string>("hello"))] = 300;
    EXPECT_EQ(2, store.size()) << "REQ: diff key";
}
TEST(SafeAdrTest, GOLD_asKeyOf_map)
{
    map<SafeAdr<string>, int> store;
    EXPECT_EQ(0, store.size()) << "REQ: SafeAdr can be key";

    store[SafeAdr<string>()] = 100;
    EXPECT_EQ(nullptr, SafeAdr<string>().get());
    EXPECT_EQ(1, store.size()) << "REQ: key=nullptr is OK";

    store[SafeAdr<string>()] = 200;
    EXPECT_EQ(1,   store.size()) << "REQ: no dup key";
    EXPECT_EQ(200, store[SafeAdr<string>()]) << "REQ: dup key overwrites value";

    store[SafeAdr<string>(make_safe<string>("hello"))] = 300;
    EXPECT_EQ(2, store.size()) << "REQ: diff key";
}

}  // namespace
