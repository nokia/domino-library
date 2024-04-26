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
    EXPECT_EQ(42, *i.get()) << "REQ: valid construct & get";

    *i.get() = 43;
    EXPECT_EQ(43, *i.get()) << "REQ: valid update";

    i.get().reset();
    EXPECT_EQ(43, *i.get()) << "REQ: outside del not impact SafeAdr";
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

struct Base                   { virtual int value()  { return 0; } };
struct Derive : public Base   { int value() override { return 1; } };
struct D2     : public Derive { int value() override { return 2; } };
TEST(SafeAdrTest, GOLD_safe_cp_toBase_OK)
{
    auto d2 = make_safe<D2>();
    EXPECT_EQ(2, d2                 .get()->value()) << "req: cp D2->D2";
    EXPECT_EQ(2, SafeAdr<Derive>(d2).get()->value()) << "req: cp D2->Derive";
    EXPECT_EQ(2, SafeAdr<Base  >(d2).get()->value()) << "REQ: cp D2->Base";
}
TEST(SafeAdrTest, GOLD_safe_cp_toVoid_OK)
{
    auto b = make_safe<Base>();
    EXPECT_EQ(b.get(), SafeAdr             <void>(b).get()) << "REQ: cp (self-defined) Base->void";
    EXPECT_EQ(b.get(), dynamic_pointer_cast<void>(b).get()) << "cov: cast cp";

    auto c = make_safe<char>();
    EXPECT_EQ(c.get(), SafeAdr             <void>(c).get()) << "REQ: cp char->void";
    EXPECT_EQ(c.get(), dynamic_pointer_cast<void>(c).get()) << "cov: cast cp";
}
TEST(SafeAdrTest, safe_cp_toDerive_NOK)
{
    SafeAdr<Base> b = make_safe<Derive>();

//  EXPECT_EQ(1, SafeAdr            <Derive>(b).get()->value()) << "REQ: cp (Derive->)Base->Derive compile err";
    EXPECT_EQ(1, static_pointer_cast<Derive>(b).get()->value()) << "req: cast cp is a workaround";

//  EXPECT_EQ(1,       SafeAdr            <D2>(b).get()) << "req: cp (Derive->)Base->D2 compile err";
    EXPECT_EQ(nullptr, static_pointer_cast<D2>(b).get()) << "REQ: cast cp ret null (compile-err is safer)";
}
TEST(SafeAdrTest, safe_cp_nonVoidToNonVoid)
{
//  SafeAdr<int> i = static_pointer_cast<int>(make_safe<char>('c'));  // REQ: compile err
}
TEST(SafeAdrTest, GOLD_safe_cp_voidToNonVoid)
{
    SafeAdr<Base> b = make_safe<D2>();  // origin is D2
    SafeAdr<void> v = b;  // preVoid is Base

//  EXPECT_EQ(2, SafeAdr            <D2>(v).get()->value()) << "req: cp (D2->Base->)void->D2 compile err";
    EXPECT_EQ(2, static_pointer_cast<D2>(v).get()->value()) << "REQ: cast cp can get origin";

//  EXPECT_EQ(2, SafeAdr            <Base>(v).get()->value()) << "req: cp (D2->Base->)void->Base compile err";
    EXPECT_EQ(2, static_pointer_cast<Base>(v).get()->value()) << "REQ: cast cp can get pre-void-type";

    EXPECT_EQ(nullptr, static_pointer_cast<Derive>(v).get()) << "cast failed since Derive not origin nor preVoid";
}
TEST(SafeAdrTest, safe_cp_complex)
{
    auto v = SafeAdr<void>(static_pointer_cast<D2>(SafeAdr<Derive>(make_safe<D2>())));

//  EXPECT_EQ(2, SafeAdr            <D2>(v).get()->value()) << "REQ: cp (D2->Derive->D2->)void->D2 compile err";
    EXPECT_EQ(2, static_pointer_cast<D2>(v).get()->value()) << "req: cast cp is a workaround";

//  EXPECT_EQ(2, SafeAdr            <Derive>(v).get()->value()) << "req: cp (D2->Derive->D2->)void->Derive compile err";
    EXPECT_EQ(2, static_pointer_cast<Derive>(v).get()->value()) << "REQ: cast cp is a workaround";
}
TEST(SafeAdrTest, safe_cast_bugFix)
{
    SafeAdr<Base> b = make_safe<D2>();  // origin is D2
    SafeAdr<void> v = b;  // preVoid is Base
    auto vv = dynamic_pointer_cast<void>(v);  // bug fix for multi-void
    EXPECT_EQ(2, static_pointer_cast<Base>(vv).get()->value()) << "REQ: can D2->Base->void->void->Base";
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
