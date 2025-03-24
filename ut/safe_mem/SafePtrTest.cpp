/**
 * Copyright 2024 Nokia. All rights reserved.
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include <gtest/gtest.h>
#include <map>
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
    ASSERT_NE(nullptr, content) << "REQ: shared_ptr should not be null";
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
    // SafePtr<int> ptr(raw);  // compile-err
    static_assert(!is_constructible<SafePtr<int>, int*>::value,
        "REQ: forbid unsafe raw pointer construction");

    // SafePtr<int> ptr(make_shared<int>());  // compile-err
    static_assert(!is_constructible<SafePtr<int>, shared_ptr<int>>::value,
        "REQ: forbid unsafe shared_ptr construction");
}
TEST(SafePtrTest, safeCreate_default)
{
    SafePtr v;
    EXPECT_EQ(nullptr, v.get()) << "REQ: create default is empty";
    EXPECT_EQ(type_index(typeid(shared_ptr<void>)), type_index(typeid(v.get()))) << "REQ: default template is void";

    SafePtr<int> i;
    EXPECT_EQ(nullptr, i.get()) << "req: create default is empty";
    EXPECT_EQ(type_index(typeid(shared_ptr<int>)), type_index(typeid(i.get()))) << "REQ: specify template";

    auto ptr = make_safe<int>();
    EXPECT_EQ(0, *ptr.get()) << "REQ: empty make_safe like make_shared";
    EXPECT_EQ(1, ptr.use_count()) << "REQ: compatible shared_ptr";
}
TEST(SafePtrTest, safeCreate_null)
{
    const SafePtr v = nullptr;  // common
    EXPECT_EQ(nullptr, v.get()) << "REQ: explicit create null to compatible with shared_ptr";
    EXPECT_EQ(type_index(typeid(shared_ptr<void>)), type_index(typeid(v.get()))) << "REQ: default template is void";

    const SafePtr<int> i = nullptr;
    EXPECT_EQ(nullptr, i.get()) << "req: create default is empty";
    EXPECT_EQ(type_index(typeid(shared_ptr<int>)), type_index(typeid(i.get()))) << "REQ: specify template";
}

#define CAST
// ***********************************************************************************************
struct Base                 { virtual int value() const  { return 0; } };
struct Derive : public Base { int value() const override { return 1; } };
TEST(SafePtrTest, GOLD_safeCast_self_base_void_back)
{
    auto d = make_safe<Derive>();
    auto nRef = d.use_count();

    // cast to self
    auto dd = safe_cast<Derive>(d);
    EXPECT_EQ(1, dd->value())        << "REQ: Derive->Derive";
    EXPECT_EQ(++nRef, d.use_count()) << "REQ: all are shared";

    auto dbd = safe_cast<Derive>(safe_cast<Base>(d));
    EXPECT_EQ(1, dbd->value())       << "REQ: Derive->Base->Derive";
    EXPECT_EQ(++nRef, d.use_count()) << "REQ: all are shared";

    auto dbvd = safe_cast<Derive>(safe_cast<void>(safe_cast<Base>(d)));
    EXPECT_EQ(1, dbvd->value())      << "REQ: Derive->Base->void->Derive";
    EXPECT_EQ(++nRef, d.use_count()) << "REQ: all are shared";

    /*auto dvbd = safe_cast<Derive>(safe_cast<Base>(safe_cast<void>(d)));
    EXPECT_EQ(1, dvbd->value())      << "REQ: Derive->void->Base->Derive";
    EXPECT_EQ(++nRef, d.use_count()) << "REQ: all are shared";*/

    // cast to Base
    auto db = safe_cast<Base  >(d);
    EXPECT_EQ(1, db->value()) << "REQ: Derive->Base";
    EXPECT_EQ(++nRef, d.use_count()) << "REQ: all are shared";

    /*auto dvb = safe_cast<Base>(safe_cast<void>(d));
    EXPECT_EQ(1, dvb->value())       << "REQ: Derive->void->Base";
    EXPECT_EQ(++nRef, d.use_count()) << "REQ: all are shared";*/

    auto dbvb = safe_cast<Base>(safe_cast<void>(safe_cast<Base>(d)));
    EXPECT_EQ(1, dbvb->value())      << "REQ: Derive->Base->void->Base";
    EXPECT_EQ(++nRef, d.use_count()) << "REQ: all are shared";

    auto dbdvb = safe_cast<Base>(safe_cast<void>(safe_cast<Derive>(safe_cast<Base>(d))));
    EXPECT_EQ(1, dbdvb->value())     << "REQ: Derive->Base->Derive->void->Base";
    EXPECT_EQ(++nRef, d.use_count()) << "REQ: all are shared";
}
TEST(SafePtrTest, invalidCast)
{
    //EXPECT_EQ(nullptr, safe_cast<char  >(make_safe<int >(7)).get());  // invalid, compile err safer than ret nullptr
    //EXPECT_EQ(nullptr, safe_cast<char  >(SafePtr  <int >( )).get());  // invalid, same as dynamic_pointer_cast
    EXPECT_EQ(nullptr, safe_cast<Derive>(make_safe<Base>() ).get()) << "REQ: invalid Base->Derive";

    struct D_protect : protected Derive { int value() const override { return 2; } };
    struct D_private : private   Derive { int value() const override { return 3; } };
    //EXPECT_EQ(nullptr, safe_cast<Base>(make_safe<D_private>()).get());  // invalid, compile err
    //EXPECT_EQ(nullptr, safe_cast<Base>(make_safe<D_protect>()).get());  // invalid, compile err

    struct B { int i=0; };  // no virtual
    struct D : public B { D(){ i=1; } };
    auto d = safe_cast<B>(make_safe<D>());
    EXPECT_EQ(1, d.get()->i) << "D sliced to B but still safe";
    //EXPECT_EQ(nullptr, safe_cast<D>(d).get());  // invalid, compile err

    auto msgInQ  = SafePtr<void>(SafePtr<Base>(make_safe<Derive>()));
    auto msgOutQ = safe_cast<char>(msgInQ);  // failed cast
    EXPECT_EQ(nullptr                 , msgOutQ.get     ()) << "REQ: failed cast -> dest get nothing";
    EXPECT_EQ(type_index(typeid(char)), msgOutQ.realType()) << "REQ: failed cast -> dest get nothing";
    EXPECT_EQ(type_index(typeid(char)), msgOutQ.lastType()) << "REQ: failed cast -> dest get nothing";
}
TEST(SafePtrTest, safe_cast_bugFix)
{
    auto dbvvb = safe_cast<Base>(safe_cast<void>(safe_cast<void>(safe_cast<Base>(make_safe<Derive>()))));
    EXPECT_EQ(1, dbvvb->value()) << "REQ: can cast Derive->Base->void->void->Base (multi-void)";
}

#define COPY
// ***********************************************************************************************
struct A
{
    int value() { return 1; }
    int value() const { return 0; }
};
TEST(SafePtrTest, GOLD_safeCp_toSameType)
{
    auto i = make_safe<int>(42);
    SafePtr<int> i2(i);  // cp, not mv
    EXPECT_EQ(42, *i2.get())    << "REQ: cp to self";
    EXPECT_EQ(2, i.use_count()) << "REQ: shared ownership";

    auto a = make_safe<A>();
    SafePtr<const A> ac(a);
    EXPECT_EQ(0, ac->value()) << "REQ: cp to const";

    const SafePtr<A> ca(a);
    EXPECT_EQ(1, ca->value()) << "REQ: cp ok & calls non-const ok";

    const SafePtr<Base> cb = make_safe<Base>();
    EXPECT_EQ(0, SafePtr<Base>(cb)->value()) << "REQ: safe cp from const to non-const - like shared_ptr";
}
TEST(SafePtrTest, GOLD_safeCp_toVoid)
{
    auto db = SafePtr<Base>(make_safe<Derive>());
    SafePtr<void> dbv(db);
    // check target
    EXPECT_EQ(1,          safe_cast<Base>(dbv)->value ()) << "req: cp Derive->Base->void";
    EXPECT_EQ(type_index(typeid(Derive)), dbv.realType()) << "REQ: cp to target";
    EXPECT_EQ(type_index(typeid(Base  )), dbv.lastType()) << "REQ: cp to target";
    // check src
    EXPECT_EQ(2,                          db.use_count()) << "REQ: cp is shared";
    EXPECT_EQ(1,          safe_cast<Base>(db)->value  ()) << "REQ: keep src after cp";
    EXPECT_EQ(type_index(typeid(Derive)), db.realType ()) << "REQ: keep src after cp";
    EXPECT_EQ(type_index(typeid(Base  )), db.lastType ()) << "REQ: keep src after cp";

    auto dbd = safe_cast<Derive>(db);
    SafePtr<void> dbdv(dbd);
    // check target
    EXPECT_EQ(1,          safe_cast<Base>(dbdv)->value ()) << "req: cp Derive->Base->Derive->void";
    EXPECT_EQ(type_index(typeid(Derive)), dbdv.realType()) << "REQ: cp to target";
    EXPECT_EQ(type_index(typeid(Base  )), dbdv.lastType()) << "REQ: cp to target";
    // check src
    EXPECT_EQ(4,                          dbd.use_count()) << "REQ: cp is shared";
    EXPECT_EQ(1,          safe_cast<Base>(dbd)->value  ()) << "REQ: keep src after cp";
    EXPECT_EQ(type_index(typeid(Derive)), dbd.realType ()) << "REQ: keep src after cp";
    EXPECT_EQ(type_index(typeid(Base  )), dbd.lastType ()) << "REQ: keep src after cp";
}
TEST(SafePtrTest, GOLD_safeCp_toPolyBase)
{
    auto d = make_safe<Derive>();
    SafePtr<Base> db(d);
    // check target
    EXPECT_EQ(1,                          db->value  ()) << "REQ: cp to Base";
    EXPECT_EQ(type_index(typeid(Derive)), db.realType()) << "REQ: cp to Base";
    EXPECT_EQ(type_index(typeid(Base  )), db.lastType()) << "REQ: keep diff";
    // check src
    EXPECT_EQ(2,                          d.use_count()) << "REQ: cp is shared";
    EXPECT_EQ(1,                          d->value   ()) << "REQ: keep src after cp";
    EXPECT_EQ(type_index(typeid(Derive)), d.realType ()) << "REQ: keep src after cp";
    EXPECT_EQ(type_index(typeid(Derive)), d.lastType ()) << "REQ: keep src after cp";

    SafePtr<const Base> dbc(d);
    // check target
    EXPECT_EQ(1,                              dbc->value  ()) << "REQ: cp to const Base";
    EXPECT_EQ(type_index(typeid(Derive)),     dbc.realType()) << "REQ: cp to const Base";
    EXPECT_EQ(type_index(typeid(const Base)), dbc.lastType()) << "REQ: keep diff";
    // check src
    EXPECT_EQ(3,                          d.use_count()) << "REQ: cp is shared";
    EXPECT_EQ(1,                          d->value   ()) << "REQ: keep src after cp";
    EXPECT_EQ(type_index(typeid(Derive)), d.realType ()) << "req: keep src after cp";
    EXPECT_EQ(type_index(typeid(Derive)), d.lastType ()) << "req: keep src after cp";
}
struct BS             { int i = 10; };
struct DS : public BS { DS() { i = 20; } };
TEST(SafePtrTest, safeCp_toStaticBase)
{
    auto d = make_safe<DS>();
    SafePtr<BS> db(d);
    // check target
    EXPECT_EQ(20,                     db->i        ) << "REQ: cp to static Base";
    EXPECT_EQ(type_index(typeid(DS)), db.realType()) << "REQ: cp to static Base";
    EXPECT_EQ(type_index(typeid(BS)), db.lastType()) << "REQ: diff";
    // check src
    EXPECT_EQ(2,                      d.use_count()) << "REQ: cp is shared";
    EXPECT_EQ(20,                     d->i         ) << "REQ: access same member";
    EXPECT_EQ(type_index(typeid(DS)), d.realType ()) << "req: keep src after cp";
    EXPECT_EQ(type_index(typeid(DS)), d.lastType ()) << "req: keep src after cp";
    SafePtr<const BS> dbc(db);
    EXPECT_EQ(20, dbc->i         ) << "REQ: cp to const static Base";
    EXPECT_EQ(3,  dbc.use_count()) << "REQ: share";
}
TEST(SafePtrTest, safeCp_nullptr)
{
    auto d = SafePtr<Derive>();
    SafePtr<void> dv(d);
    EXPECT_EQ(nullptr, dv.get());
    EXPECT_EQ(type_index(typeid(void)), dv.realType()) << "REQ: cp nullptr as if cp fail, avoid confuse usr";
    EXPECT_EQ(type_index(typeid(void)), dv.lastType()) << "REQ: cp nullptr as if cp fail, avoid confuse usr";
    EXPECT_EQ(0,                        dv.use_count()) << "REQ: none";
    EXPECT_EQ(0,                        d .use_count()) << "REQ: none";
}
TEST(SafePtrTest, unsafeCp_toDiffType)
{
    auto i = make_safe<int>(7);
    //auto c = SafePtr<char>(i);  // REQ: compile err for diff types
}
struct D_private   : private   Base {};
struct D_protected : protected Base {};
TEST(SafePtrTest, unsupportCp_toBase)
{
    auto b_pri = make_safe<D_private  >();
    //auto b = SafePtr<Base>(b_pri);  // c++ not allow
    auto b_pro = make_safe<D_protected>();
    //auto b = SafePtr<Base>(b_pro);  // c++ not allow
}
TEST(SafePtrTest, unsupportCp_toDerive)
{
    auto b = make_safe<Base>();
    //auto bd = SafePtr<Derive>(b);  // REQ: compile err Base->Derive - unsafe

    auto db = SafePtr<Base>(make_safe<Derive>());
    //auto dbd = SafePtr<Derive>(db);  // safe but unsupport since need dynamic_cast that can't bld err if fail
}
TEST(SafePtrTest, unsupportCp_voidToNon)
{
    auto bv = SafePtr<void>(make_safe<Base>());
    //auto bvb = SafePtr<Base>(bv);  // safe but unsupport since need dynamic check that can't bld err if fail
}
TEST(SafePtrTest, unssafeCp_constToNon)
{
    auto bc = make_safe<const Base>();
    //auto b = SafePtr<Base>(bc);  // REQ: unsafe cp, compile err
}

#define MOVE
// ***********************************************************************************************
TEST(SafePtrTest, GOLD_safeMv_toSameType)
{
    auto i = make_safe<int>(42);
    SafePtr<int> i2(move(i));
    EXPECT_EQ(42, *i2.get())     << "REQ: mv to self";
    EXPECT_EQ(1, i2.use_count()) << "REQ: mv ownership";
    EXPECT_EQ(nullptr, i.get())  << "REQ: source is empty after move";

    auto a = make_safe<A>();
    SafePtr<const A> ac(move(a));
    EXPECT_EQ(0, ac->value()) << "REQ: mv to const";
    EXPECT_EQ(nullptr, a.get())  << "REQ: source is empty after mv";
}
TEST(SafePtrTest, GOLD_safeMv_toVoid)
{
    auto db = SafePtr<Base>(make_safe<Derive>());
    SafePtr<void> dbv(move(db));
    // check target
    EXPECT_EQ(1,          safe_cast<Base>(dbv)->value ()) << "req: mv Derive->Base->void";
    EXPECT_EQ(type_index(typeid(Derive)), dbv.realType()) << "REQ: mv to target";
    EXPECT_EQ(type_index(typeid(Base  )), dbv.lastType()) << "REQ: mv to target";
    // check src
    EXPECT_EQ(nullptr,                  db.get     ()) << "REQ: reset src after mv";
    EXPECT_EQ(type_index(typeid(Base)), db.realType()) << "REQ: reset src after mv";
    EXPECT_EQ(type_index(typeid(Base)), db.lastType()) << "REQ: reset src after mv";

    auto dbd = safe_cast<Derive>(SafePtr<Base>(make_safe<Derive>()));
    SafePtr<void> dbdv(move(dbd));
    // check target
    EXPECT_EQ(1,          safe_cast<Base>(dbdv)->value ()) << "req: mv Derive->Base->Derive->void";
    EXPECT_EQ(type_index(typeid(Derive)), dbdv.realType()) << "REQ: mv to target";
    EXPECT_EQ(type_index(typeid(Base  )), dbdv.lastType()) << "REQ: mv to target";
    // check src
    EXPECT_EQ(nullptr,                    dbd.get     ()) << "REQ: reset src after mv";
    EXPECT_EQ(type_index(typeid(Derive)), dbd.realType()) << "REQ: reset src after mv";
    EXPECT_EQ(type_index(typeid(Derive)), dbd.lastType()) << "REQ: reset src after mv";
}
TEST(SafePtrTest, GOLD_safeMv_toPolyBase)
{
    auto d = make_safe<Derive>();
    SafePtr<Base> db(move(d));
    // check target
    EXPECT_EQ(1,            SafePtr<Base>(db)->value ()) << "REQ: mv to Base";
    EXPECT_EQ(type_index(typeid(Derive)), db.realType()) << "REQ: mv to Base";
    EXPECT_EQ(type_index(typeid(Base  )), db.lastType()) << "REQ: keep diff";
    // check src
    EXPECT_EQ(nullptr,                    d.get     ()) << "REQ: reset src after mv";
    EXPECT_EQ(type_index(typeid(Derive)), d.realType()) << "REQ: reset src after mv";
    EXPECT_EQ(type_index(typeid(Derive)), d.lastType()) << "REQ: reset src after mv";

    SafePtr<const Base> dbc(move(db));
    // check target
    EXPECT_EQ(1,                              dbc->value  ()) << "REQ: mv to const Base";
    EXPECT_EQ(type_index(typeid(Derive    )), dbc.realType()) << "REQ: mv to const Base";
    EXPECT_EQ(type_index(typeid(const Base)), dbc.lastType()) << "REQ: keep diff";
    // check src
    EXPECT_EQ(nullptr,                  db.get     ()) << "req: reset src after mv";
    EXPECT_EQ(type_index(typeid(Base)), db.realType()) << "REQ: reset src after mv";
    EXPECT_EQ(type_index(typeid(Base)), db.lastType()) << "REQ: reset src after mv";
}
TEST(SafePtrTest, safeMv_toStaticBase)
{
    auto d = make_safe<DS>();
    SafePtr<BS> db(move(d));
    // check target
    EXPECT_EQ(20,                     db->i        ) << "REQ: mv to static Base";
    EXPECT_EQ(type_index(typeid(DS)), db.realType()) << "REQ: mv to static Base";
    EXPECT_EQ(type_index(typeid(BS)), db.lastType()) << "REQ: diff";
    // check src
    EXPECT_EQ(nullptr,                d.get     ()) << "REQ: reset src after mv";
    EXPECT_EQ(type_index(typeid(DS)), d.realType()) << "req: reset src after mv";
    EXPECT_EQ(type_index(typeid(DS)), d.lastType()) << "req: reset src after mv";

    SafePtr<const BS> dbc(move(db));
    EXPECT_EQ(20, dbc->i         ) << "REQ: mv to const static Base";
    EXPECT_EQ(1,  dbc.use_count()) << "REQ: mv ownership";
}
TEST(SafePtrTest, safeMv_nullptr)
{
    auto d = SafePtr<Derive>();
    SafePtr<void> dv(move(d));
    EXPECT_EQ(nullptr, dv.get());
    EXPECT_EQ(type_index(typeid(void)), dv.realType ()) << "REQ: mv nullptr as if mv fail, avoid confuse usr";
    EXPECT_EQ(type_index(typeid(void)), dv.lastType ()) << "REQ: mv nullptr as if mv fail, avoid confuse usr";
    EXPECT_EQ(0,                        dv.use_count()) << "REQ: none";
    EXPECT_EQ(0,                        d .use_count()) << "REQ: none";
}
TEST(SafePtrTest, unsafeMv_toDiffType)
{
    auto i = make_safe<int>(7);
    //auto c = SafePtr<char>(move(i));  // REQ: compile err for diff types
}
TEST(SafePtrTest, unsupportMv_toBase)
{
    auto b_pri = make_safe<D_private  >();
    //auto b = SafePtr<Base>(move(b_pri));  // c++ not allow
    auto b_pro = make_safe<D_protected>();
    //auto b = SafePtr<Base>(move(b_pro));  // c++ not allow
}
TEST(SafePtrTest, unsupportMv_toDerive)
{
    auto b = make_safe<Base>();
    //auto bd = SafePtr<Derive>(move(b));  // REQ: compile err Base->Derive - unsafe

    auto db = SafePtr<Base>(make_safe<Derive>());
    //auto dbd = SafePtr<Derive>(move(db));  // safe but unsupport since need dynamic_cast that can't bld err if fail
}
TEST(SafePtrTest, unsupportMv_voidToNon)
{
    auto bv = SafePtr<void>(make_safe<Base>());
    //auto bvb = SafePtr<Base>(move(bv));  // safe but unsupport since need dynamic check that can't bld err if fail
}
TEST(SafePtrTest, unssafeMv_constToNon)
{
    auto bc = make_safe<const Base>();
    //auto b = SafePtr<Base>(move(bc));  // REQ: unsafe cp, compile err
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

#define WEAK
// ***********************************************************************************************
TEST(SafePtrTest, GOLD_convert_SafePtr_WeakPtr)
{
    // create SafeWeak
    auto d = make_safe<Derive>();
    SafeWeak<Derive> w = d;
    EXPECT_EQ(1, d.use_count()) << "REQ: SafeWeak not inc use_count.";
    EXPECT_FALSE(w.expired()) << "REQ: SafeWeak flag.";

    // SafeWeak -> SafePtr
    auto dd = w.lock();
    EXPECT_EQ(d, dd) << "REQ: same";
    EXPECT_EQ(d.realType(), dd.realType()) << "REQ: same";
    EXPECT_EQ(d.lastType(), dd.lastType()) << "REQ: same";
    EXPECT_EQ(2, d.use_count()) << "REQ: SafeWeak not inc use_count.";
    EXPECT_FALSE(w.expired()) << "REQ: SafeWeak flag.";

    // del SafePtr
    d = nullptr;
    EXPECT_EQ(1, dd.use_count()) << "REQ: SafeWeak not inc use_count.";
    EXPECT_FALSE(w.expired()) << "REQ: SafeWeak flag.";

    dd = nullptr;
    EXPECT_TRUE(w.expired()) << "REQ: SafeWeak flag.";
    EXPECT_EQ(nullptr, w.lock().get()) << "REQ: no more.";
}

#define WEAK
// ***********************************************************************************************
TEST(SafePtrTest, except_safe)
{
    struct E {
        E() { throw runtime_error("construct except"); }
    };
    EXPECT_EQ(nullptr, make_safe<E>().get()) << "REQ: except->nullptr";
}

}  // namespace
