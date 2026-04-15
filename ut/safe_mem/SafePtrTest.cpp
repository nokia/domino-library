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
// ***********************************************************************************************
// isValid(): centralized invariant check for SafePtr
// - invariant: null SafePtr<void> must have lastType=void (never stale)
// - invariant: non-null SafePtr<T≠void> must have lastType=typeid(T)
// - invariant: use_count consistency with bool(ptr)
template<typename T>
testing::AssertionResult isValid(const SafePtr<T>& p)
{
    // bool vs null consistency
    if (bool(p) != (p.get() != nullptr))
        return testing::AssertionFailure() << "REQ: bool(p) shall consist with get()";

    // use_count vs null consistency
    if (bool(p) == (p.use_count() == 0))
        return testing::AssertionFailure() << "REQ: use_count shall consist with bool(p)";

    // lastType & size
    if constexpr(std::is_void_v<T>) {
        EXPECT_EQ(sizeof(SafePtr<T>), sizeof(std::shared_ptr<T>) + sizeof(std::type_index))
            << "REQ: EBO +8B for void only";
        if (!p && p.lastType() != type_index(typeid(void)))
            return testing::AssertionFailure() << "REQ: SafePtr<void>.lastType_ shall be void if nullptr";
    } else {
        EXPECT_EQ(sizeof(SafePtr<T>), sizeof(std::shared_ptr<T>)) << "REQ: EBO zero overhead for non-void";
        if (p.lastType() != type_index(typeid(T)))
            return testing::AssertionFailure() << "REQ: correct SafePtr.lastType_=" << typeid(T).name();
    }

    return testing::AssertionSuccess();
}
// EXPECT_VALID(p): gtest prints line#; #p prints variable name — zero manual cost
#define EXPECT_VALID(p) EXPECT_TRUE(isValid(p)) << #p

#define CREATE
// ***********************************************************************************************
TEST(SafePtrTest, GOLD_safeCreate_normal)
{
    SafePtr<int> i = make_safe<int>(42);
    EXPECT_VALID(i);
    EXPECT_EQ(1, i.use_count()) << "REQ: compatible shared_ptr";

    auto content = i.get();
    EXPECT_EQ(42, *content) << "REQ: valid get";
    EXPECT_EQ(2, i.use_count()) << "REQ: compatible shared_ptr";

    *content = 43;
    EXPECT_EQ(43, *i.get()) << "REQ: valid update";

    content = nullptr;
    EXPECT_EQ(nullptr, content) << "REQ: reset OK";
    EXPECT_EQ(43, *i.get()) << "REQ: outside reset not impact SafePtr";
    EXPECT_EQ(1, i.use_count()) << "REQ: compatible shared_ptr";
    EXPECT_VALID(i);
}
TEST(SafePtrTest, GOLD_unsafeCreate_forbid)
{
    // SafePtr<int> ptr(raw);  // compile-err
    static_assert(!is_constructible<SafePtr<int>, int*>::value,
        "REQ: forbid construct: raw pointer is unsafe");

    // SafePtr<int> ptr(make_shared<int>());  // compile-err
    static_assert(!is_constructible<SafePtr<int>, shared_ptr<int>>::value,
        "REQ: forbid construct: shared_ptr is unsafe - that's why SafePtr exists");
}
TEST(SafePtrTest, safeCreate_default)
{
    SafePtr v;
    EXPECT_VALID(v);

    SafePtr<int> i;
    EXPECT_VALID(i);

    auto ptr = make_safe<int>();
    EXPECT_EQ(0, *ptr.get()) << "REQ: empty make_safe like make_shared";
    EXPECT_EQ(1, ptr.use_count()) << "REQ: compatible shared_ptr";
    EXPECT_VALID(ptr);
}
TEST(SafePtrTest, safeCreate_null)
{
    const SafePtr v = nullptr;  // common
    EXPECT_VALID(v);

    const SafePtr<int> i = nullptr;
    EXPECT_VALID(i);
}

#define CAST
// ***********************************************************************************************
struct Base                 { virtual int value() const  { return 0; } };
struct Derive : public Base { int value() const override { return 1; } };

TEST(SafePtrTest, GOLD_safeCast)
{
    auto d = make_safe<Derive>();
    EXPECT_VALID(d);
    auto nRef = d.use_count();

    // cast to self
    auto dd = safe_cast<Derive>(d);
    EXPECT_VALID(dd);
    EXPECT_EQ(1, dd->value())        << "REQ: Derive->Derive";
    EXPECT_VALID(d);
    EXPECT_EQ(++nRef, d.use_count()) << "REQ: all are shared";

    auto dbd = safe_cast<Derive>(safe_cast<Base>(d));
    EXPECT_VALID(dbd);
    EXPECT_EQ(1, dbd->value())       << "REQ: Derive->Base->Derive";
    EXPECT_VALID(d);
    EXPECT_EQ(++nRef, d.use_count()) << "REQ: all are shared";

    auto dbvbd = safe_cast<Derive>(safe_cast<Base>(safe_cast<void>(safe_cast<Base>(d))));
    EXPECT_VALID(dbvbd);
    EXPECT_EQ(1, dbvbd->value())     << "REQ: Derive->Base->void->Base->Derive";
    EXPECT_VALID(d);
    EXPECT_EQ(++nRef, d.use_count()) << "REQ: all are shared";

    auto dbdvd = safe_cast<Derive>(safe_cast<void>(safe_cast<Derive>(safe_cast<Base>(d))));
    EXPECT_VALID(dbdvd);
    EXPECT_EQ(1, dbdvd->value())     << "REQ: Derive->Base->Derive->void->Derive";
    EXPECT_VALID(d);
    EXPECT_EQ(++nRef, d.use_count()) << "REQ: all are shared";

    // cast to const
    auto dcb = safe_cast<const Base>(d);
    EXPECT_VALID(dcb);
    EXPECT_EQ(1, dcb->value())       << "REQ: Derive->const Base (virtual dispatch)";
    EXPECT_EQ(++nRef, d.use_count()) << "REQ: all are shared";

    // cast nullptr
    auto n = safe_cast<Base>(SafePtr<Derive>());
    EXPECT_VALID(n);
    EXPECT_EQ(nullptr, n.get())      << "REQ: cast nullptr -> nullptr";

    auto dvb = safe_cast<Base>(safe_cast<void>(d));
    EXPECT_VALID(dvb);
    EXPECT_EQ(nullptr, dvb.get())    << "Derive->void->Base: unsupported since mem & impl cost";
    EXPECT_VALID(d);
    EXPECT_EQ(nRef, d.use_count()) << "REQ: keep shared";

    auto dbvd = safe_cast<Derive>(safe_cast<void>(safe_cast<Base>(d)));
    EXPECT_VALID(dbvd);
    EXPECT_EQ(nullptr, dbvd.get())   << "Derive->Base->void->Derive unsupported";
    EXPECT_VALID(d);
    EXPECT_EQ(nRef, d.use_count()) << "REQ: keep shared";
}
TEST(SafePtrTest, GOLD_unsafeCast)
{
    //EXPECT_EQ(nullptr, safe_cast<char>(make_safe<int>(7)).get());  // unsafe, compile err safer than ret nullptr
    //EXPECT_EQ(nullptr, safe_cast<char>(SafePtr  <int>( )).get());  // unsafe, same as dynamic_pointer_cast
    EXPECT_EQ(nullptr, safe_cast<Derive>(make_safe<Base>() ).get()) << "REQ: unsafe Base->Derive";

    struct D_protect : protected Derive { int value() const override { return 2; } };
    struct D_private : private   Derive { int value() const override { return 3; } };
    //EXPECT_EQ(nullptr, safe_cast<Base>(make_safe<D_private>()).get());  // unsafe, compile err
    //EXPECT_EQ(nullptr, safe_cast<Base>(make_safe<D_protect>()).get());  // unsafe, compile err

    struct B { int i=0; };  // no virtual
    struct D : public B { D(){ i=1; } };
    auto d = safe_cast<B>(make_safe<D>());
    EXPECT_VALID(d);
    EXPECT_EQ(1, d.get()->i) << "D sliced to B but still safe";
    //EXPECT_EQ(nullptr, safe_cast<D>(d).get());  // unsafe, compile err

    auto msgInQ  = SafePtr<void>(SafePtr<Base>(make_safe<Derive>()));
    auto msgOutQ = safe_cast<char>(msgInQ);  // failed cast
    EXPECT_EQ(nullptr                 , msgOutQ.get     ()) << "REQ: failed cast -> dest get nothing";
    EXPECT_EQ(type_index(typeid(char)), msgOutQ.lastType()) << "REQ: failed cast -> dest get nothing";
    EXPECT_VALID(msgInQ);
    EXPECT_VALID(msgOutQ);

    //auto x = safe_cast<Base>(make_safe<const Base>());  // unsafe const->non-const, compile err
}
TEST(SafePtrTest, bugFix_safeCast_multiVoid)
{
    auto dbvvbd = safe_cast<Base>(safe_cast<Derive>(safe_cast<Base>(
        safe_cast<void>(safe_cast<void>(  // multi void
            safe_cast<Base>(make_safe<Derive>()))))));
    EXPECT_EQ(1, dbvvbd->value()) << "REQ: multi void OK";
    EXPECT_VALID(dbvvbd);
}
TEST(SafePtrTest, GOLD_safe_D_B_void_B)
{
    // Multiple inheritance: D has two bases at different offsets
    struct A { virtual int a() { return 1; } };
    struct B { virtual int b() { return 2; } };
    struct D : A, B {
        int a() override { return 11; }
        int b() override { return 22; }
    };

    auto d = make_safe<D>();
    EXPECT_EQ(sizeof(d), sizeof(shared_ptr<D>)) << "REQ: EBO for SafePtr<D>";

    // D->B
    SafePtr<B> b = d;
    EXPECT_VALID(b);
    EXPECT_EQ(22, b->b()) << "REQ: correct B sub-object";
    EXPECT_EQ(sizeof(b), sizeof(shared_ptr<B>)) << "REQ: EBO for SafePtr<B>";

    // B->void
    SafePtr<void> v = b;
    EXPECT_VALID(v);
    EXPECT_EQ(sizeof(v), (sizeof(shared_ptr<B>) + sizeof(std::type_index))) << "REQ: EBO for SafePtr<void>";

    // void→D
    auto invalidD = safe_cast<D>(v);
    EXPECT_VALID(invalidD);
    EXPECT_EQ(nullptr, invalidD.get()) << "REQ: void→D not possible when lastType=B";
    EXPECT_EQ(sizeof(invalidD), sizeof(shared_ptr<D>)) << "REQ: EBO for SafePtr<D>";

    // void->B
    auto backB = safe_cast<B>(v);
    EXPECT_VALID(backB);
    EXPECT_EQ(22, backB->b()) << "REQ: correct B sub-object";
    EXPECT_EQ(sizeof(backB), sizeof(shared_ptr<B>)) << "REQ: EBO for SafePtr<B>";

    // B->D
    auto backD = safe_cast<D>(b);
    EXPECT_VALID(backD);
    EXPECT_EQ(11, backD->a()) << "REQ: correct D sub-object";
    EXPECT_EQ(22, backD->b()) << "REQ: correct D sub-object";
    EXPECT_EQ(sizeof(backD), sizeof(shared_ptr<D>)) << "REQ: EBO for SafePtr<D>";
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
    EXPECT_VALID(i);
    EXPECT_VALID(i2);

    auto a = make_safe<A>();
    SafePtr<const A> ac(a);
    EXPECT_EQ(0, ac->value()) << "REQ: cp to const";
    EXPECT_VALID(a);
    EXPECT_VALID(ac);

    const SafePtr<A> ca(a);
    EXPECT_EQ(1, ca->value()) << "REQ: cp ok & calls non-const ok";
    EXPECT_VALID(a);
    EXPECT_VALID(ca);

    const SafePtr<Base> cb = make_safe<Base>();
    EXPECT_EQ(0, SafePtr<Base>(cb)->value()) << "REQ: safe cp from const to non-const - like shared_ptr";
    EXPECT_VALID(cb);
}
TEST(SafePtrTest, GOLD_safeCp_toVoid)
{
    auto db = SafePtr<Base>(make_safe<Derive>());
    SafePtr<void> dbv(db);
    // check target
    EXPECT_EQ(1,            safe_cast<Base>(dbv)->value()) << "req: cp Derive->Base->void->Base";
    EXPECT_EQ(type_index(typeid(Base)),    dbv.lastType()) << "REQ: cp to void preserves lastType=Base";
    // check src
    EXPECT_EQ(2,                           db.use_count()) << "REQ: cp is shared";
    EXPECT_EQ(1,             safe_cast<Base>(db)->value()) << "REQ: keep src after cp";
    EXPECT_EQ(type_index(typeid(Base)),    db.lastType ()) << "REQ: keep src after cp";

    auto dbd = safe_cast<Derive>(db);
    SafePtr<void> dbdv(dbd);
    // check target
    EXPECT_EQ(1,         safe_cast<Derive>(dbdv)->value()) << "req: cp Derive->Base->Derive->void->Derive";
    EXPECT_EQ(type_index(typeid(Derive)), dbdv.lastType()) << "REQ: cp to void preserves lastType=Derive";
    // check src
    EXPECT_EQ(4,                          dbd.use_count()) << "REQ: cp is shared";
    EXPECT_EQ(1,          safe_cast<Base>(dbd)->value  ()) << "REQ: keep src after cp";
    EXPECT_EQ(type_index(typeid(Derive)), dbd.lastType ()) << "REQ: keep src after cp";
    EXPECT_VALID(dbv);
    EXPECT_VALID(dbdv);
}
TEST(SafePtrTest, GOLD_safeCp_toPolyBase)
{
    auto d = make_safe<Derive>();
    SafePtr<Base> db(d);
    // check target
    EXPECT_EQ(1,                          db->value  ()) << "REQ: cp to Base";
    EXPECT_EQ(type_index(typeid(Base)),   db.lastType()) << "REQ: lastType=Base";
    // check src
    EXPECT_EQ(2,                          d.use_count()) << "REQ: cp is shared";
    EXPECT_EQ(1,                          d->value   ()) << "REQ: keep src after cp";
    EXPECT_EQ(type_index(typeid(Derive)), d.lastType ()) << "REQ: keep src after cp";

    SafePtr<const Base> dbc(d);
    // check target
    EXPECT_EQ(1,                              dbc->value  ()) << "REQ: cp to const Base";
    EXPECT_EQ(type_index(typeid(const Base)), dbc.lastType()) << "REQ: lastType=const Base";
    // check src
    EXPECT_EQ(3,                          d.use_count()) << "REQ: cp is shared";
    EXPECT_EQ(1,                          d->value   ()) << "REQ: keep src after cp";
    EXPECT_EQ(type_index(typeid(Derive)), d.lastType ()) << "req: keep src after cp";

    EXPECT_VALID(d);
    EXPECT_VALID(db);
    EXPECT_VALID(dbc);
}
TEST(SafePtrTest, safeCp_toStaticBase)
{
    struct B { int i=0; };
    struct D : B { D(){ i=1; } };
    auto d = make_safe<D>();
    SafePtr<B> b(d);
    EXPECT_VALID(b);
    EXPECT_EQ(1, b->i) << "REQ: cp to non-virtual base";
    EXPECT_EQ(2, d.use_count()) << "REQ: shared";
}
TEST(SafePtrTest, safeCp_nullptr)
{
    auto d = SafePtr<Derive>();
    SafePtr<void> dv(d);
    EXPECT_EQ(nullptr, dv.get());
    EXPECT_EQ(type_index(typeid(void)), dv.lastType ()) << "REQ: cp nullptr as if cp fail, avoid confuse usr";
    EXPECT_EQ(0,                        dv.use_count()) << "REQ: none";
    EXPECT_EQ(0,                        d .use_count()) << "REQ: none";
    EXPECT_VALID(dv);
}
struct D_private   : private   Base {};
struct D_protected : protected Base {};
TEST(SafePtrTest, unsafeCpMvAssign_compileForbid)
{
    // diff type
    static_assert(!is_constructible<SafePtr<char>, const SafePtr<int>&>::value,
        "REQ: forbid cp diff type");
    static_assert(!is_constructible<SafePtr<char>, SafePtr<int>&&>::value,
        "REQ: forbid mv diff type");

    // private/protected inherit
    static_assert(!is_constructible<SafePtr<Base>, const SafePtr<D_private>&>::value,
        "REQ: forbid cp private-derive to base");
    static_assert(!is_constructible<SafePtr<Base>, SafePtr<D_private>&&>::value,
        "REQ: forbid mv private-derive to base");
    static_assert(!is_constructible<SafePtr<Base>, const SafePtr<D_protected>&>::value,
        "REQ: forbid cp protected-derive to base");
    static_assert(!is_constructible<SafePtr<Base>, SafePtr<D_protected>&&>::value,
        "REQ: forbid mv protected-derive to base");

    // base to derive
    static_assert(!is_constructible<SafePtr<Derive>, const SafePtr<Base>&>::value,
        "REQ: forbid cp base to derive");
    static_assert(!is_constructible<SafePtr<Derive>, SafePtr<Base>&&>::value,
        "REQ: forbid mv base to derive");

    // void to non-void
    static_assert(!is_constructible<SafePtr<Base>, const SafePtr<void>&>::value,
        "REQ: forbid cp void to non-void");
    static_assert(!is_constructible<SafePtr<Base>, SafePtr<void>&&>::value,
        "REQ: forbid mv void to non-void");

    // const to non-const
    static_assert(!is_constructible<SafePtr<Base>, const SafePtr<const Base>&>::value,
        "REQ: forbid cp const to non-const");
    static_assert(!is_constructible<SafePtr<Base>, SafePtr<const Base>&&>::value,
        "REQ: forbid mv const to non-const");

    // assign: same forbids apply (cross-type = uses ctor + mv=)
    static_assert(!is_assignable<SafePtr<char>&, const SafePtr<int>&>::value,
        "REQ: forbid assign diff type");
    static_assert(!is_assignable<SafePtr<Base>&, const SafePtr<D_private>&>::value,
        "REQ: forbid assign private-derive to base");
    static_assert(!is_assignable<SafePtr<Base>&, const SafePtr<D_protected>&>::value,
        "REQ: forbid assign protected-derive to base");
    static_assert(!is_assignable<SafePtr<Derive>&, const SafePtr<Base>&>::value,
        "REQ: forbid assign base to derive");
    static_assert(!is_assignable<SafePtr<Base>&, const SafePtr<void>&>::value,
        "REQ: forbid assign void to non-void");
    static_assert(!is_assignable<SafePtr<Base>&, const SafePtr<const Base>&>::value,
        "REQ: forbid assign const to non-const");
}

#define MOVE
// ***********************************************************************************************
TEST(SafePtrTest, GOLD_safeMv_toSameType)
{
    auto i = make_safe<int>(42);
    SafePtr<int> i2(move(i));
    EXPECT_VALID(i);
    EXPECT_VALID(i2);
    EXPECT_EQ(42, *i2.get())     << "REQ: mv to self";
    EXPECT_EQ(1, i2.use_count()) << "REQ: mv ownership";
    EXPECT_EQ(nullptr, i.get())  << "REQ: source is empty after move";

    auto a = make_safe<A>();
    SafePtr<const A> ac(move(a));
    EXPECT_VALID(a);
    EXPECT_VALID(ac);
    EXPECT_EQ(0, ac->value()) << "REQ: mv to const";
    EXPECT_EQ(nullptr, a.get())  << "REQ: source is empty after mv";
}
TEST(SafePtrTest, GOLD_safeMv_toVoid)
{
    auto db = SafePtr<Base>(make_safe<Derive>());
    SafePtr<void> dbv(move(db));
    EXPECT_VALID(db);
    EXPECT_VALID(dbv);
    // check target
    EXPECT_EQ(1,         safe_cast<Base>(dbv)->value()) << "req: mv Derive->Base->void->Base";
    EXPECT_EQ(type_index(typeid(Base)), dbv.lastType()) << "REQ: mv to void preserves lastType=Base";
    // check src
    EXPECT_EQ(nullptr,                  db.get      ()) << "REQ: reset src after mv";
    EXPECT_EQ(type_index(typeid(Base)), db.lastType ()) << "REQ: reset src after mv";

    auto dbd = safe_cast<Derive>(SafePtr<Base>(make_safe<Derive>()));
    SafePtr<void> dbdv(move(dbd));
    EXPECT_VALID(dbd);
    EXPECT_VALID(dbdv);
    // check target
    EXPECT_EQ(1,         safe_cast<Derive>(dbdv)->value()) << "req: mv Derive->Base->Derive->void->Derive";
    EXPECT_EQ(type_index(typeid(Derive)), dbdv.lastType()) << "REQ: mv to void preserves lastType=Derive";
    // check src
    EXPECT_EQ(nullptr,                    dbd.get      ()) << "REQ: reset src after mv";
    EXPECT_EQ(type_index(typeid(Derive)), dbd.lastType ()) << "REQ: reset src after mv";
}
TEST(SafePtrTest, GOLD_safeMv_toPolyBase)
{
    auto d = make_safe<Derive>();
    SafePtr<Base> db(move(d));
    EXPECT_VALID(d);
    EXPECT_VALID(db);
    // check target
    EXPECT_EQ(1,             SafePtr<Base>(db)->value()) << "REQ: mv to Base";
    EXPECT_EQ(type_index(typeid(Base  )), db.lastType()) << "REQ: lastType=Base";
    // check src
    EXPECT_EQ(nullptr,                    d.get      ()) << "REQ: reset src after mv";
    EXPECT_EQ(type_index(typeid(Derive)), d.lastType ()) << "REQ: reset src after mv";

    SafePtr<const Base> dbc(move(db));
    EXPECT_VALID(db);
    EXPECT_VALID(dbc);
    // check target
    EXPECT_EQ(1,                              dbc->value  ()) << "REQ: mv to const Base";
    EXPECT_EQ(type_index(typeid(const Base)), dbc.lastType()) << "REQ: lastType=const Base";
    // check src
    EXPECT_EQ(nullptr,                  db.get      ()) << "req: reset src after mv";
    EXPECT_EQ(type_index(typeid(Base)), db.lastType ()) << "REQ: reset src after mv";
}
TEST(SafePtrTest, safeMv_toStaticBase)
{
    struct B { int i=0; };
    struct D : B { D(){ i=1; } };
    auto d = make_safe<D>();
    SafePtr<B> b(move(d));
    EXPECT_VALID(d);
    EXPECT_VALID(b);
    EXPECT_EQ(1, b->i) << "REQ: mv to non-virtual base";
    EXPECT_EQ(nullptr, d.get()) << "REQ: src empty after mv";
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
    EXPECT_VALID(one);
    EXPECT_VALID(two);

    // same-type mv=
    auto three = make_safe<int>(99);
    SafePtr<int> four;
    four = move(three);
    EXPECT_VALID(three);
    EXPECT_VALID(four);
    EXPECT_EQ(99, *four.get()) << "REQ: mv= transfers value";
    EXPECT_EQ(nullptr, three.get()) << "REQ: src empty after mv=";
}
TEST(SafePtrTest, safeAssign_void)
{
    SafePtr<void> v1(make_safe<int>(42));
    SafePtr<void> v2;
    v2 = v1;  // void cp=
    EXPECT_VALID(v1);
    EXPECT_VALID(v2);
    EXPECT_EQ(type_index(typeid(int)), v2.lastType()) << "REQ: void cp= preserves lastType";
    EXPECT_EQ(v1.get(), v2.get()) << "REQ: shared ownership";
}
TEST(SafePtrTest, safeAssign_crossType)
{
    auto d = make_safe<Derive>();
    SafePtr<Base> b;
    b = d;  // Derive→Base assign
    EXPECT_VALID(b);
    EXPECT_EQ(1, b->value()) << "REQ: cross-type assign";
    EXPECT_EQ(2, d.use_count()) << "REQ: shared";

    SafePtr<void> v;
    v = SafePtr<Base>(d);  // Base→void assign
    EXPECT_VALID(v);
    EXPECT_EQ(type_index(typeid(Base)), v.lastType()) << "REQ: assign to void preserves lastType";
}
TEST(SafePtrTest, GOLD_safeReset_byAssign)
{
    auto msgInQ = SafePtr<void>(SafePtr<Base>(make_safe<Derive>()));
    EXPECT_NE(nullptr                 , msgInQ.get      ()) << "REQ: init ok";
    EXPECT_EQ(type_index(typeid(Base)), msgInQ.lastType ()) << "REQ: lastType=Base";

    msgInQ = nullptr;  // reset
    EXPECT_EQ(nullptr                 , msgInQ.get      ()) << "REQ: reset ok";
    EXPECT_EQ(type_index(typeid(void)), msgInQ.lastType ()) << "REQ: reset ok";
    EXPECT_VALID(msgInQ);
}

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
    EXPECT_VALID(test);
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
    EXPECT_VALID(test);
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
TEST(SafePtrTest, GOLD_voidEqual_identity_not_castPath)
{
    // == answers "same object?" (identity), not "same cast path?" (view)
    // MI first-base: D* == A* (same addr) → same identity → ==
    // user checks lastType() explicitly when cast-path distinction is needed
    struct A { virtual ~A() = default; int a = 1; };
    struct B { virtual ~B() = default; int b = 2; };
    struct D : A, B { D() { a = 11; b = 22; } };

    auto d = make_safe<D>();
    SafePtr<void> vD = d;              // lastType=D, raw=D*
    SafePtr<void> vA = SafePtr<A>(d);  // lastType=A, raw=A*=D* (first base)
    EXPECT_VALID(vD);
    EXPECT_VALID(vA);

    EXPECT_EQ(vD.get().get(), vA.get().get()) << "REQ: first-base addr == D addr";
    EXPECT_EQ(vD, vA) << "REQ: same ptr → same object → == (identity)";
    EXPECT_NE(vD.lastType(), vA.lastType()) << "REQ: lastType differs — user checks explicitly";
}
TEST(SafePtrTest, GOLD_voidEqual_diffObj)
{
    // MI second-base: B* ≠ D* → different raw addrs → !=
    struct B1 { virtual ~B1() = default; int b1 = 10; };
    struct B2 { virtual ~B2() = default; int b2 = 20; };
    struct D : B1, B2 { D() { b1 = 11; b2 = 22; } };

    auto d = make_safe<D>();
    SafePtr<void> v1 = SafePtr<B1>(d);
    SafePtr<void> v2 = SafePtr<B2>(d);
    EXPECT_VALID(v1);
    EXPECT_VALID(v2);
    EXPECT_NE(v1.get().get(), v2.get().get()) << "REQ: MI second-base has diff addr";
    EXPECT_NE(v1, v2) << "REQ: diff addr → !=";
}
TEST(SafePtrTest, voidEqual_samePath)
{
    auto d = make_safe<int>(42);
    SafePtr<void> v1(d);
    SafePtr<void> v2(d);
    EXPECT_VALID(v1);
    EXPECT_VALID(v2);
    EXPECT_EQ(v1, v2) << "REQ: same ptr → ==";
    EXPECT_FALSE(v1 != v2) << "REQ: != consistent";
    EXPECT_FALSE(v1 < v2) << "REQ: < consistent with ==";
    EXPECT_FALSE(v2 < v1) << "REQ: < consistent with ==";
}
TEST(SafePtrTest, voidEqual_null)
{
    SafePtr<void> n1, n2;
    EXPECT_VALID(n1);
    EXPECT_EQ(n1, n2) << "REQ: null == null";
    EXPECT_FALSE(n1 != n2) << "REQ: != consistent";
    EXPECT_FALSE(n1 < n2) << "REQ: < consistent";
}
TEST(SafePtrTest, GOLD_voidMap_sameObj_oneKey)
{
    // same object via different cast paths → same key in map (identity, not view)
    struct A { virtual ~A() = default; };
    struct B { virtual ~B() = default; };
    struct D : A, B {};

    auto d = make_safe<D>();
    SafePtr<void> vD = d;
    SafePtr<void> vA = SafePtr<A>(d);
    EXPECT_VALID(vD);
    EXPECT_VALID(vA);
    EXPECT_EQ(vD, vA) << "precondition: same identity";

    // ordered map: second insert overwrites first (same key)
    map<SafePtr<void>, string> m;
    m[vD] = "D";
    m[vA] = "A";
    EXPECT_EQ(1, m.size()) << "REQ: same object → one key";
    EXPECT_EQ("A", m[vD]) << "REQ: last write wins";

    // unordered map: same
    unordered_map<SafePtr<void>, string> um;
    um[vD] = "D";
    um[vA] = "A";
    EXPECT_EQ(1, um.size()) << "REQ: same object → one key";
    EXPECT_EQ("A", um[vD]) << "REQ: last write wins";
}

#define WEAK
// ***********************************************************************************************
// EBO: SafeWeak<T≠void> = sizeof(weak_ptr), SafeWeak<void> = weak_ptr + type_index
static_assert(sizeof(SafeWeak<int>) == sizeof(std::weak_ptr<int>), "EBO: zero overhead for non-void SafeWeak");
static_assert(sizeof(SafeWeak<void>) == sizeof(std::weak_ptr<void>) + sizeof(std::type_index), "EBO: +8B for void SafeWeak only");

TEST(SafePtrTest, GOLD_convert_SafePtr_WeakPtr)
{
    // create SafeWeak
    auto d = make_safe<Derive>();
    SafeWeak<Derive> w = d;
    EXPECT_EQ(1, d.use_count()) << "REQ: SafeWeak not inc use_count.";
    EXPECT_FALSE(w.expired()) << "REQ: SafeWeak flag.";

    // SafeWeak -> SafePtr
    auto dd = w.lock();
    EXPECT_VALID(dd);
    EXPECT_EQ(d, dd) << "REQ: same";
    EXPECT_EQ(d.lastType(), dd.lastType()) << "REQ: same";
    EXPECT_EQ(2, d.use_count()) << "REQ: SafeWeak not inc use_count.";
    EXPECT_FALSE(w.expired()) << "REQ: SafeWeak flag.";

    // del SafePtr
    d = nullptr;
    EXPECT_VALID(d);
    EXPECT_EQ(1, dd.use_count()) << "REQ: SafeWeak not inc use_count.";
    EXPECT_FALSE(w.expired()) << "REQ: SafeWeak flag.";

    dd = nullptr;
    EXPECT_VALID(dd);
    EXPECT_TRUE(w.expired()) << "REQ: SafeWeak flag.";
    EXPECT_EQ(nullptr, w.lock().get()) << "REQ: no more.";
}
TEST(SafePtrTest, safeWeak_void)
{
    auto d = make_safe<Derive>();
    SafePtr<void> dbv = SafePtr<Base>(d);  // Derive->Base->void
    EXPECT_VALID(dbv);
    SafeWeak<void> dbvw = dbv;
    EXPECT_FALSE(dbvw.expired()) << "REQ: SafeWeak<void> not expired.";
    EXPECT_EQ(type_index(typeid(Base)), dbv.lastType()) << "REQ: lastType=Base";

    // lock & verify round-trip
    auto dbv2 = dbvw.lock();
    EXPECT_VALID(dbv2);
    EXPECT_NE(nullptr, dbv2.get()) << "REQ: lock ok";
    EXPECT_EQ(type_index(typeid(Base)), dbv2.lastType()) << "REQ: lock preserves lastType";
    EXPECT_EQ(1, safe_cast<Base>(dbv2)->value()) << "REQ: round-trip void->Base ok";

    // expire
    d = nullptr;
    EXPECT_VALID(d);
    dbv = nullptr;
    EXPECT_VALID(dbv);
    dbv2 = nullptr;
    EXPECT_VALID(dbv2);
    EXPECT_TRUE(dbvw.expired()) << "REQ: SafeWeak<void> expired.";
    EXPECT_EQ(nullptr, dbvw.lock().get()) << "REQ: lock after expire -> nullptr.";
}
TEST(SafePtrTest, safeWeak_nullptr)
{
    SafeWeak<int> w = SafePtr<int>();
    EXPECT_TRUE(w.expired()) << "REQ: weak from nullptr is expired";
    EXPECT_EQ(nullptr, w.lock().get()) << "REQ: lock nullptr weak → null";

    SafeWeak<void> wv = SafePtr<void>();
    EXPECT_TRUE(wv.expired()) << "REQ: void weak from nullptr is expired";
    EXPECT_EQ(nullptr, wv.lock().get()) << "REQ: lock nullptr void weak → null";
}

#define EXCEPT
// ***********************************************************************************************
TEST(SafePtrTest, except_safe)
{
    struct E {
        E() { throw runtime_error("construct except"); }
    };
    EXPECT_EQ(nullptr, make_safe<E>().get()) << "REQ: except->nullptr";
}

#define SECURITY
// ***********************************************************************************************
// - vs shared_ptr vulns: type confusion, stale state, self-move, MI addr mismatch
TEST(SafePtrTest, GOLD_voidMv_lastType_consistent)
{
    // mv ctor
    SafePtr<void> v1(make_safe<int>(42));
    EXPECT_VALID(v1);
    SafePtr<void> v2(std::move(v1));
    EXPECT_VALID(v2);
    EXPECT_EQ(type_index(typeid(int)), v2.lastType()) << "REQ: target preserves lastType";
    EXPECT_EQ(42, *safe_cast<int>(v2).get()) << "REQ: target data intact";
    EXPECT_VALID(v1);
    EXPECT_EQ(nullptr, v1.get()) << "REQ: source null after move";

    // mv assign
    SafePtr<void> v3(make_safe<float>(3.14f));
    EXPECT_VALID(v3);
    SafePtr<void> v4;
    v4 = std::move(v3);
    EXPECT_VALID(v4);
    EXPECT_EQ(type_index(typeid(float)), v4.lastType()) << "REQ: target preserves lastType";
    EXPECT_VALID(v3);
    EXPECT_EQ(nullptr, v3.get()) << "REQ: source null after move-assign";
}
TEST(SafePtrTest, selfAssign_void_safe)
{
    // self-move-assign
    SafePtr<void> v(make_safe<int>(42));
    EXPECT_VALID(v);
    auto* pv = &v;
    *pv = std::move(v);
    EXPECT_VALID(v);

    // self-copy-assign
    v = SafePtr<void>(make_safe<int>(42));  // reset to valid
    EXPECT_VALID(v);
    pv = &v;
    *pv = v;
    EXPECT_VALID(v);
    EXPECT_EQ(42, *safe_cast<int>(v).get()) << "REQ: data intact after self-cp=";
}

TEST(SafePtrTest, voidCpFromMovedFrom_safe)
{
    SafePtr<void> v1(make_safe<int>(42));
    SafePtr<void> v2(std::move(v1));
    EXPECT_VALID(v1);
    // v1 is moved-from; copying it must be safe
    SafePtr<void> v3(v1);
    EXPECT_VALID(v3);
    EXPECT_EQ(nullptr, v3.get()) << "REQ: cp from moved-from → null";
    EXPECT_EQ(type_index(typeid(void)), v3.lastType()) << "REQ: cp from moved-from → lastType=void";

    // void→void cp (valid source)
    SafePtr<void> v4(v2);
    EXPECT_VALID(v4);
    EXPECT_EQ(42, *safe_cast<int>(v4).get()) << "REQ: void→void cp preserves data";
    EXPECT_EQ(type_index(typeid(int)), v4.lastType()) << "REQ: void→void cp preserves lastType";
    EXPECT_EQ(v2.get(), v4.get()) << "REQ: shared ownership";
}
TEST(SafePtrTest, safeCastFromMovedFrom_safe)
{
    SafePtr<void> v1(make_safe<int>(42));
    SafePtr<void> v2(std::move(v1));
    EXPECT_VALID(v1);
    // safe_cast from moved-from must not crash
    auto i = safe_cast<int>(v1);
    EXPECT_VALID(i);
    EXPECT_EQ(nullptr, i.get()) << "REQ: cast from moved-from → null";
}

TEST(SafePtrTest, GOLD_MI_D_B1_void_B2_typeConfusion)
{
    // C1: verify D->B1->void->B2 is BLOCKED (type confusion via MI)
    struct B1 { virtual ~B1() = default; int b1 = 10; };
    struct B2 { virtual ~B2() = default; int b2 = 20; };
    struct D : B1, B2 { D() { b1 = 11; b2 = 22; } };

    auto d = make_safe<D>();
    EXPECT_VALID(d);

    // D→B1→void (lastType_=B1, void* stores B1 sub-object addr)
    SafePtr<void> v = SafePtr<B1>(d);
    EXPECT_VALID(v);
    EXPECT_EQ(type_index(typeid(B1)), v.lastType()) << "REQ: lastType=B1";

    // void→B2 MUST fail: lastType=B1 ≠ B2, addr offset would be wrong
    auto wrongB2 = safe_cast<B2>(v);
    EXPECT_VALID(wrongB2);
    EXPECT_EQ(nullptr, wrongB2.get()) << "REQ: D→B1→void→B2 blocked (type confusion)";

    // void→B1 must succeed
    auto backB1 = safe_cast<B1>(v);
    EXPECT_VALID(backB1);
    ASSERT_NE(nullptr, backB1.get()) << "REQ: D→B1→void→B1 ok";
    EXPECT_EQ(11, backB1->b1) << "REQ: correct B1 value";

    // void→D must fail (lastType=B1, not D)
    auto wrongD = safe_cast<D>(v);
    EXPECT_VALID(wrongD);
    EXPECT_EQ(nullptr, wrongD.get()) << "REQ: D→B1→void→D blocked (lastType≠D)";

    // correct path: D→B1→D→B2
    auto backD = safe_cast<D>(SafePtr<B1>(d));
    EXPECT_VALID(backD);
    if (backD) {
        SafePtr<B2> correctB2(backD);
        EXPECT_VALID(correctB2);
        EXPECT_EQ(22, correctB2->b2) << "REQ: correct path D→B1→D→B2 ok";
    }
}

TEST(SafePtrTest, safe_virtual_MI_diamond)
{
    struct VBase { virtual ~VBase() = default; int v = 0; };
    struct Left  : virtual VBase { int left = 1; };
    struct Right : virtual VBase { int right = 2; };
    struct Diamond : Left, Right { Diamond() { v = 10; left = 11; right = 12; } };

    auto d = make_safe<Diamond>();
    EXPECT_VALID(d);

    // Diamond→Left→void→Left round-trip
    SafePtr<void> vLeft = SafePtr<Left>(d);
    EXPECT_VALID(vLeft);
    EXPECT_EQ(type_index(typeid(Left)), vLeft.lastType());
    auto backLeft = safe_cast<Left>(vLeft);
    EXPECT_VALID(backLeft);
    ASSERT_NE(nullptr, backLeft.get()) << "REQ: Diamond→Left→void→Left ok";
    EXPECT_EQ(11, backLeft->left);

    // Left→Diamond via dynamic_cast (virtual inheritance)
    auto backD1 = safe_cast<Diamond>(backLeft);
    EXPECT_VALID(backD1);
    ASSERT_NE(nullptr, backD1.get()) << "REQ: Diamond→Left→void→Left→Diamond ok";
    EXPECT_EQ(10, backD1->v);

    // Diamond→VBase→void→VBase round-trip (virtual base)
    SafePtr<void> vBase = SafePtr<VBase>(d);
    EXPECT_VALID(vBase);
    EXPECT_EQ(type_index(typeid(VBase)), vBase.lastType());
    auto backBase = safe_cast<VBase>(vBase);
    EXPECT_VALID(backBase);
    ASSERT_NE(nullptr, backBase.get()) << "REQ: Diamond→VBase→void→VBase ok";
    EXPECT_EQ(10, backBase->v);

    // VBase→Diamond via dynamic_cast (cross virtual inheritance)
    auto backD2 = safe_cast<Diamond>(backBase);
    EXPECT_VALID(backD2);
    ASSERT_NE(nullptr, backD2.get()) << "REQ: Diamond→VBase→void→VBase→Diamond ok";
    EXPECT_EQ(12, backD2->right);

    // cross-path blocked: Diamond→Left→void→Right
    auto wrongRight = safe_cast<Right>(vLeft);
    EXPECT_VALID(wrongRight);
    EXPECT_EQ(nullptr, wrongRight.get()) << "REQ: Diamond→Left→void→Right blocked";

    // cross-path blocked: Diamond→Left→void→VBase
    auto wrongBase = safe_cast<VBase>(vLeft);
    EXPECT_VALID(wrongBase);
    EXPECT_EQ(nullptr, wrongBase.get()) << "REQ: Diamond→Left→void→VBase blocked";
}

TEST(SafePtrTest, weakFromMovedFrom_safe)
{
    SafePtr<void> v(make_safe<int>(42));
    SafePtr<void> v2(std::move(v));
    EXPECT_VALID(v);
    // create weak from moved-from
    SafeWeak<void> w = v;
    EXPECT_TRUE(w.expired()) << "REQ: weak from moved-from is expired";
    EXPECT_EQ(nullptr, w.lock().get()) << "REQ: lock moved-from weak → null";
}

}  // namespace
