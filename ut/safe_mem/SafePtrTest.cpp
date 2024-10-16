/**
 * Copyright 2024 Nokia. All rights reserved.
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include <gtest/gtest.h>
#include <map>
#include <type_traits>
#include <typeindex>
#include <unordered_map>

#include "SafePtr.hpp"

using namespace std;

namespace rlib
{
#define CREATE
// ***********************************************************************************************
TEST(SafePtrTest, GOLD_safeCreate_normal)
{
    SafePtr<int> i = make_safe<int>(42);
    EXPECT_EQ(1, i.use_count()) << "REQ: compatible shared_ptr";

    auto content = i.get();
    EXPECT_EQ(42, *content) << "REQ: valid construct & get";
    EXPECT_EQ(2, i.use_count()) << "REQ: compatible shared_ptr";

    *content = 43;
    EXPECT_EQ(43, *i.get()) << "REQ: valid update";

    content = nullptr;
    EXPECT_EQ(nullptr, content) << "REQ: reset OK";
    EXPECT_EQ(43, *i.get()) << "REQ: outside reset not impact SafePtr";
    EXPECT_EQ(1, i.use_count()) << "REQ: compatible shared_ptr";
}
TEST(SafePtrTest, GOLD_unsafeCreate_forbid)
{
    static_assert(! is_convertible_v<bool*, SafePtr<bool>>, "REQ: forbid SafePtr<T>(new T)");
    static_assert(! is_convertible_v<shared_ptr<char>, SafePtr<char>>, "REQ: forbid SafePtr<T>(shared_ptr<T>)");
}
TEST(SafePtrTest, safeCreate_default)
{
    SafePtr v;
    EXPECT_EQ(nullptr, v.get()) << "REQ: create default is empty";
    EXPECT_EQ(type_index(typeid(shared_ptr<void>)), type_index(typeid(v.get()))) << "REQ: default template is void";

    SafePtr<int> i;
    EXPECT_EQ(nullptr, i.get()) << "req: create default is empty";
    EXPECT_EQ(type_index(typeid(shared_ptr<int>)), type_index(typeid(i.get()))) << "REQ: specify template";
}
TEST(SafePtrTest, safeCreate_null)  // convenient usage (convert nullptr to SafePtr)
{
    const SafePtr v(nullptr);
    EXPECT_EQ(nullptr, v.get()) << "REQ: explicit create null to compatible with shared_ptr";
    EXPECT_EQ(type_index(typeid(shared_ptr<void>)), type_index(typeid(v.get()))) << "REQ: default template is void";

    const SafePtr<int> i(nullptr);
    EXPECT_EQ(nullptr, i.get()) << "req: create default is empty";
    EXPECT_EQ(type_index(typeid(shared_ptr<int>)), type_index(typeid(i.get()))) << "REQ: specify template";
}
TEST(SafePtrTest, safeCreate_noexcept_constexpr)
{
    static_assert(is_nothrow_constructible_v<SafePtr<>>, "REQ: noexcept; optional: constexpr");
    static_assert(is_nothrow_constructible_v<SafePtr<int>, nullptr_t>, "REQ: noexcept; optional: constexpr");
}

#define CAST
// ***********************************************************************************************
struct Base                 { virtual int value() const  { return 0; } };
struct Derive : public Base { int value() const override { return 1; } };
TEST(SafePtrTest, GOLD_safeCast_self_base_void_back)
{
    auto d = make_safe<Derive>();
    EXPECT_EQ(1, dynPtrCast<Derive>(d)->value()) << "REQ: cast to self";
    EXPECT_EQ(1, dynPtrCast<Base  >(d)->value()) << "REQ: cast to base";

    EXPECT_EQ(1, dynPtrCast<Derive>(dynPtrCast<Base>(d))->value()) << "REQ: (derived->)base->derived";

    EXPECT_EQ(1, dynPtrCast<Derive>(dynPtrCast<void>(dynPtrCast<Base>(d)))->value()) << "REQ: ->void & void->origin";
    EXPECT_EQ(1, dynPtrCast<Base  >(dynPtrCast<void>(dynPtrCast<Base>(d)))->value()) << "REQ: ->void & void->preVoid";

    //EXPECT_EQ(1, dynPtrCast<Base>(dynPtrCast<void>(make_safe<Derive>())).get()) << "safe cast, but not support";
}
struct D_protect : protected Derive { int value() const override { return 2; } };
struct D_private : private   Derive { int value() const override { return 3; } };
TEST(SafePtrTest, invalidCast_retNull)
{
    EXPECT_EQ(nullptr, dynPtrCast<char  >(make_safe<int >(7)).get()) << "REQ: invalid int ->char";
    EXPECT_EQ(nullptr, dynPtrCast<Derive>(make_safe<Base>() ).get()) << "REQ: invalid base->derived";

    //EXPECT_EQ(nullptr, dynPtrCast<Base>(make_safe<D_private>()).get());  // invalid, not ret null but compile err
    //EXPECT_EQ(nullptr, dynPtrCast<Base>(make_safe<D_protect>()).get());  // invalid, not ret null but compile err

    auto msgInQ = SafePtr<void>(SafePtr<Base>(make_safe<Derive>()));
    auto msgOutQ = dynPtrCast<char>(msgInQ);
    EXPECT_EQ(1, dynPtrCast<Base>(msgInQ)->value ()) << "REQ: failed cast -> keep src";
    EXPECT_EQ(type_index(typeid(Derive)),   msgInQ.realType()) << "REQ: failed cast -> keep src";
    EXPECT_EQ(type_index(typeid(Base  )),   msgInQ.lastType()) << "REQ: failed cast -> keep src";

    EXPECT_EQ(nullptr                 , msgOutQ.get     ()) << "REQ: failed cast -> dest get nothing";
    EXPECT_EQ(type_index(typeid(char)), msgOutQ.realType()) << "REQ: failed cast -> dest get nothing";
    EXPECT_EQ(type_index(typeid(char)), msgOutQ.lastType()) << "REQ: failed cast -> dest get nothing";
}
struct D2 : public Derive { int value() const override { return 2; } };
TEST(SafePtrTest, safe_cast_bugFix)
{
    SafePtr<Base> b = make_safe<D2>();  // realType_ is D2
    SafePtr<void> v = b;  // diffType_ is Base
    auto vv = dynPtrCast<void>(v);  // bug fix for multi-void
    EXPECT_EQ(2, staticPtrCast<Base>(vv)->value()) << "REQ: can cast D2->Base->void->void->Base";
}

#define COPY
// ***********************************************************************************************
TEST(SafePtrTest, GOLD_safe_cp_sameType)
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
TEST(SafePtrTest, GOLD_safeCp_self_base_void)
{
    auto d = make_safe<Derive>();
    EXPECT_EQ(1, SafePtr<Derive>(d)->value()) << "REQ: cp to self";
    EXPECT_EQ(1, SafePtr<Base  >(d)->value()) << "REQ: cp to base";

    EXPECT_EQ(0, dynPtrCast<Base  >(SafePtr<void>(make_safe<Base  >()))->value()) << "REQ: cp any->void";
    EXPECT_EQ(1, dynPtrCast<Derive>(SafePtr<void>(make_safe<Derive>()))->value()) << "req: cp any->void";
}
TEST(SafePtrTest, invalidCp_compileErr)  // cp's compile-err is safer than dynPtrCast that may ret nullptr
{
    //SafePtr<Derive>(SafePtr<Base>(make_safe<Derive>()));  // derived->base->derive: cp compile err, can dynPtrCast instead
    //SafePtr<Derive>(SafePtr<void>(make_safe<Derive>()));  // void->origin: cp compile err, can dynPtrCast instead
    //SafePtr<Base  >(SafePtr<void>(make_safe<Derive>()));  // derive->void->base: cp compile err, dynPtrCast ret nullptr

    //SafePtr<Derive>(make_safe<Base>());  // base->derived: cp compile-err; dynPtrCast ret nullptr

    //SafePtr<char>(make_safe<int>(7));  // int->char: both cp & dynPtrCast will compile err

    //SafePtr<Base>(make_safe<D_private>());  // private->base: both cp & dynPtrCast will compile err
    //SafePtr<Base>(make_safe<D_protect>());  // protect->base: both cp & dynPtrCast will compile err
}
TEST(SafePtrTest, GOLD_const_and_back)
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

    SafePtr   <const D> safe_const_d  = safe_d;
    shared_ptr<const D> share_const_d = share_d;

    EXPECT_EQ(0, safe_const_d ->value()) << "REQ: cp succ & call const)";
    EXPECT_EQ(0, share_const_d->value()) << "req: cp succ & call const)";

//  SafePtr<D>    safe_dd  = safe_const_d;   // REQ: compile err to cp from const to non
//  shared_ptr<D> share_dd = share_const_d;  // REQ: compile err to cp from const to non

    const SafePtr   <D> const_safe_d  = safe_d;
    const shared_ptr<D> const_share_d = share_d;

    EXPECT_EQ(100, const_safe_d ->value()) << "REQ: cp succ & call NON-const (same as shared_ptr)";
    EXPECT_EQ(100, const_share_d->value()) << "call NON-const since all members are NON-const except 'this'";
}

#define MOVE
// ***********************************************************************************************
TEST(SafePtrTest, GOLD_mtQ_req_mv)
{
    auto msg = SafePtr<Base>(make_safe<Derive>());  // Derive->Base
    SafePtr<void> msgInQ = move(msg);
    EXPECT_EQ(nullptr, msg.get()) << "REQ: src giveup";
    EXPECT_EQ(type_index(typeid(Base)), msg.realType()) << "REQ: reset src all";
    EXPECT_EQ(type_index(typeid(Base)), msg.lastType()) << "REQ: reset src all";

    EXPECT_EQ(1                         , dynPtrCast<Base>(msgInQ)->value()  ) << "REQ: takeover msg";
    EXPECT_EQ(type_index(typeid(Derive)), dynPtrCast<Base>(msgInQ).realType()) << "REQ: reset src all";
    EXPECT_EQ(type_index(typeid(Base  )), dynPtrCast<Base>(msgInQ).lastType()) << "REQ: reset src all";
}
TEST(SafePtrTest, nothing_cp_mv_assign)
{
    SafePtr<Base> cp = SafePtr<Derive>();  // cp nullptr
    EXPECT_EQ(type_index(typeid(Base)), cp.realType()) << "REQ/cov: cp-nothing=fail so origin is Base instead of Derive";

    SafePtr<Base> mv = move(SafePtr<Derive>());  // mv nullptr
    EXPECT_EQ(type_index(typeid(Base)), mv.realType()) << "REQ/cov: mv-nothing=fail so origin is Base instead of Derive";

    SafePtr<Base> as;
    EXPECT_EQ(type_index(typeid(Base)), mv.realType());
    as = SafePtr<Derive>();
    EXPECT_EQ(type_index(typeid(Base)), mv.realType()) << "REQ/cov: assign-nothing=fail so keep origin";
}

#define ASSIGN
// ***********************************************************************************************
TEST(SafePtrTest, GOLD_safe_assign)  // operator=() is auto-gen, just simple test 1 case
{
    SafePtr<int> one;
    auto two = make_safe<int>(42);
    one = two;
    EXPECT_EQ(42, *(one.get())) << "REQ: valid assign & get";

    two = SafePtr<int>();
    EXPECT_EQ(nullptr, two.get()) << "REQ: assign to null";
    EXPECT_EQ(42, *(one.get())) << "REQ: valid get after assigner is reset";
}
TEST(SafePtrTest, GOLD_safeReset_byAssign)
{
    auto msgInQ = SafePtr<void>(SafePtr<Base>(make_safe<Derive>()));
    EXPECT_NE(nullptr                   , msgInQ.get     ()) << "REQ: init ok";
    EXPECT_EQ(type_index(typeid(Derive)), msgInQ.realType()) << "REQ: init ok";
    EXPECT_EQ(type_index(typeid(Base  )), msgInQ.lastType()) << "REQ: init ok";

    msgInQ = nullptr;  // reset
    EXPECT_EQ(nullptr                 , msgInQ.get     ()) << "REQ: reset ok";
    EXPECT_EQ(type_index(typeid(void)), msgInQ.realType()) << "REQ: reset ok";
    EXPECT_EQ(type_index(typeid(void)), msgInQ.lastType()) << "REQ: reset ok";
}
// assign/cp/mv failure -> compile err

#define DESTRUCT
// ***********************************************************************************************
struct TestBase
{
    bool& isBaseOver_;
    explicit TestBase(bool& aExtFlag) : isBaseOver_(aExtFlag) { isBaseOver_ = false; }
    virtual ~TestBase() { isBaseOver_ = true; }
};
TEST(SafePtrTest, GOLD_destructByVoid)
{
    bool isBaseOver;
    SafePtr<void> test = make_safe<TestBase>(isBaseOver);
    EXPECT_FALSE(isBaseOver) << "correctly constructed";

    test = nullptr;
    EXPECT_TRUE(isBaseOver) << "REQ: correctly destructed by SafePtr<void>";
}
struct TestDerive : public TestBase
{
    bool& isDeriveOver_;
    explicit TestDerive(bool& aBaseFlag, bool& aDeriveFlag)
        : TestBase(aBaseFlag)
        , isDeriveOver_(aDeriveFlag)
    { isDeriveOver_ = false; }
    ~TestDerive() { isDeriveOver_ = true; }
};
TEST(SafePtrTest, GOLD_destructByBase)
{
    bool isBaseOver;
    bool isDeriveOver;
    SafePtr<TestBase> test = make_safe<TestDerive>(isBaseOver, isDeriveOver);
    EXPECT_FALSE(isDeriveOver) << "correctly constructed";

    test = nullptr;
    EXPECT_TRUE(isDeriveOver) << "REQ: correctly destructed by SafePtr<TestBase>";
}

#define LIKE_SHARED_PTR
// ***********************************************************************************************
TEST(SafePtrTest, get_isMemSafe_afterDelOrigin)
{
    SafePtr<string> safe = make_safe<string>("hello");
    auto get = safe.get();
    EXPECT_EQ("hello", *get) << "get succ";

    safe = nullptr;
    EXPECT_EQ("hello", *get) << "REQ: what got is still OK";

    get = safe.get();
    EXPECT_EQ(nullptr, get) << "REQ: get nullptr OK";
}
TEST(SafePtrTest, getPtr_isMemSafe_afterDelOrigin)
{
    SafePtr<string> safe = make_safe<string>("hello");
    auto get = safe.operator->();  // get ptr
    EXPECT_EQ("hello", *get) << "REQ: get succ";

    safe = nullptr;
    EXPECT_EQ("hello", *get) << "REQ: what got is still OK";

    get = safe.operator->();
    EXPECT_EQ(nullptr, get) << "REQ: get nullptr OK";
}

#define SUPPORT_MAP
// ***********************************************************************************************
TEST(SafePtrTest, GOLD_asKeyOf_unorderedMap)
{
    unordered_map<SafePtr<string>, int> store;
    EXPECT_EQ(0, store.size()) << "REQ: SafePtr can be key";

    store[SafePtr<string>()] = 100;
    EXPECT_EQ(nullptr, SafePtr<string>().get());
    EXPECT_EQ(1, store.size()) << "REQ: key=nullptr is OK";

    store[SafePtr<string>()] = 200;
    EXPECT_EQ(1,   store.size()) << "REQ: no dup key";
    EXPECT_EQ(200, store[SafePtr<string>()]) << "REQ: dup key overwrites value";

    store[SafePtr<string>(make_safe<string>("hello"))] = 300;
    EXPECT_EQ(2, store.size()) << "REQ: diff key";
}
TEST(SafePtrTest, GOLD_asKeyOf_map)
{
    map<SafePtr<string>, int> store;
    EXPECT_EQ(0, store.size()) << "REQ: SafePtr can be key";

    store[SafePtr<string>()] = 100;
    EXPECT_EQ(nullptr, SafePtr<string>().get());
    EXPECT_EQ(1, store.size()) << "REQ: key=nullptr is OK";

    store[SafePtr<string>()] = 200;
    EXPECT_EQ(1,   store.size()) << "REQ: no dup key";
    EXPECT_EQ(200, store[SafePtr<string>()]) << "REQ: dup key overwrites value";

    store[SafePtr<string>(make_safe<string>("hello"))] = 300;
    EXPECT_EQ(2, store.size()) << "REQ: diff key";
}

}  // namespace
