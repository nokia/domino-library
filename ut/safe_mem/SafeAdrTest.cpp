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
#define CREATE
// ***********************************************************************************************
TEST(SafeAdrTest, GOLD_safe_create)
{
    auto v = SafeAdr<>();
    EXPECT_EQ(nullptr, v.get()) << "REQ: create default, null is legal address";

    auto c = SafeAdr<char>(nullptr);
    EXPECT_EQ(nullptr, c.get()) << "REQ: create null";

    auto i = make_safe<int>(42);
    auto content = i.get();
    EXPECT_EQ(42, *content) << "REQ: valid construct & get";

    *content = 43;
    EXPECT_EQ(43, *i.get()) << "REQ: valid update";

    content.reset();
    EXPECT_EQ(nullptr, content) << "REQ: reset OK";
    EXPECT_EQ(43, *i.get()) << "REQ: outside reset not impact SafeAdr";
}

#define COPY_CAST
// ***********************************************************************************************
TEST(SafeAdrTest, GOLD_safe_cp_sameType)
{
    auto one = make_safe<int>(42);
    {
        auto two(one);
        EXPECT_EQ(42, *two.get()) << "REQ: valid cp & get";

        *one.get() = 43;
        EXPECT_EQ(43, *two.get()) << "REQ: valid update via sharing";
    }
    EXPECT_EQ(43, *one.get()) << "REQ: 1 del not impact another";
}

struct Base                   { virtual int value() const  { return 0; } };
struct Derive : public Base   { int value() const override { return 1; } };
struct D2     : public Derive { int value() const override { return 2; } };
TEST(SafeAdrTest, GOLD_safe_cp_normal)
{
//  EXPECT_EQ(0,              SafeAdr<char>(make_safe<int>())) << "REQ: cp diff type - compile-err";
//  EXPECT_EQ(0, dynamic_pointer_cast<char>(make_safe<int>())) << "REQ: cast - also compile-err";

    EXPECT_EQ(1,              SafeAdr<Base>(make_safe<Derive>()).get()->value()) << "REQ: cp to base";
    EXPECT_EQ(1, dynamic_pointer_cast<Base>(make_safe<Derive>()).get()->value()) << "REQ: cast";

    EXPECT_NE(nullptr,              SafeAdr<void>(make_safe<Derive>()).get()) << "REQ: cp to void OK";
    EXPECT_NE(nullptr, dynamic_pointer_cast<void>(make_safe<Derive>()).get()) << "REQ: cast OK";
}
TEST(SafeAdrTest, safe_cp_extended)
{
//  EXPECT_EQ(1,              SafeAdr<Derive>(SafeAdr<Base>(make_safe<Derive>())).get()->value()) << "REQ: cp to derived - compile-err";
    EXPECT_EQ(1, dynamic_pointer_cast<Derive>(SafeAdr<Base>(make_safe<Derive>())).get()->value()) << "REQ: cast OK";
    EXPECT_EQ(nullptr, dynamic_pointer_cast<Derive>(make_safe<Base>()).get()) << "REQ: cast NOK so has to check null before use";

//  EXPECT_EQ(1,              SafeAdr<Derive>(SafeAdr<void>(SafeAdr<Base>(make_safe<Derive>()))).get()->value()) << "REQ: cp - compile-err";
    EXPECT_EQ(1, dynamic_pointer_cast<Derive>(SafeAdr<void>(SafeAdr<Base>(make_safe<Derive>()))).get()->value()) << "REQ: void to realType_";
    EXPECT_EQ(1, dynamic_pointer_cast<Base  >(SafeAdr<void>(SafeAdr<Base>(make_safe<Derive>()))).get()->value()) << "REQ: void to diffType_";
    EXPECT_EQ(nullptr, dynamic_pointer_cast<D2>(SafeAdr<void>(SafeAdr<Base>(make_safe<Derive>()))).get()) << "REQ: void to unknown type";
}
TEST(SafeAdrTest, safe_cast_bugFix)
{
    SafeAdr<Base> b = make_safe<D2>();  // realType_ is D2
    SafeAdr<void> v = b;  // diffType_ is Base
    auto vv = dynamic_pointer_cast<void>(v);  // bug fix for multi-void
    EXPECT_EQ(2, static_pointer_cast<Base>(vv).get()->value()) << "REQ: can cast D2->Base->void->void->Base";
}
TEST(SafeAdrTest, GOLD_const_and_back)
{
    struct D
    {
        int value()       { return 100; }
        int value() const { return 0; }
    };

    auto safe_d  = make_safe<D>();
    auto share_d = make_shared<D>();

    EXPECT_EQ(100, safe_d ->value()) << "REQ: call non-const";
    EXPECT_EQ(100, share_d->value()) << "req: call non-const";

    SafeAdr   <const D> safe_const_d  = safe_d;
    shared_ptr<const D> share_const_d = share_d;

    EXPECT_EQ(0, safe_const_d ->value()) << "REQ: cp succ & call const)";
    EXPECT_EQ(0, share_const_d->value()) << "req: cp succ & call const)";

//  SafeAdr<D>    safe_dd  = safe_const_d;   // REQ: compile err to cp from const to non
//  shared_ptr<D> share_dd = share_const_d;  // REQ: compile err to cp from const to non

    const SafeAdr   <D> const_safe_d  = safe_d;
    const shared_ptr<D> const_share_d = share_d;

    EXPECT_EQ(100, const_safe_d ->value()) << "REQ: cp succ & call NON-const (same as shared_ptr)";
    EXPECT_EQ(100, const_share_d->value()) << "call NON-const since all members are NON-const except 'this'";
}

#define MOVE
// ***********************************************************************************************
TEST(SafeAdrTest, GOLD_mtQ_req_mv)
{
    auto msg = make_safe<string>("received msg");
    SafeAdr<void> msgInQ = move(msg);
    EXPECT_EQ(nullptr, msg.get()) << "REQ: giveup";
    EXPECT_EQ("received msg", *dynamic_pointer_cast<string>(msgInQ).get()) << "REQ: takeover msg";
}
TEST(SafeAdrTest, mv_fail)
{
    auto i = make_safe<int>(7);
    auto src = SafeAdr<void>(i);
    EXPECT_EQ(&typeid(int), src.realType());
    EXPECT_EQ(nullptr, src.diffType());

    auto dst = dynamic_pointer_cast<char>(move(src));
    EXPECT_NE(nullptr,       src.get()        ) << "REQ: keep content";
    EXPECT_EQ(&typeid(int),  src.realType()   ) << "REQ: keep origin type";
    EXPECT_EQ(nullptr,       src.diffType()   ) << "REQ: keep last type";
    EXPECT_EQ(nullptr,       dst.get()        ) << "REQ: fail to takeover content";
    EXPECT_EQ(&typeid(char), dst.realType()   ) << "REQ: fail to takeover origin type";
    EXPECT_EQ(nullptr,       dst.diffType()   ) << "REQ: fail to takeover last type";

    SafeAdr<Base> b = SafeAdr<Derive>();
    EXPECT_EQ(&typeid(Base), b.realType()) << "REQ/cov: mv-nothing=fail so origin is Base instead of Derive";
}

#define ASSIGN
// ***********************************************************************************************
TEST(SafeAdrTest, safe_assign)  // operator=() is auto-gen, just simple test 1 case
{
    SafeAdr<int> one;
    auto two = make_safe<int>(42);
    one = two;
    EXPECT_EQ(42, *(one.get())) << "REQ: valid assign & get";

    two = SafeAdr<int>();
    EXPECT_EQ(nullptr, two.get()) << "REQ: assign to null";
    EXPECT_EQ(42, *(one.get())) << "REQ: valid get after assigner is reset";
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
