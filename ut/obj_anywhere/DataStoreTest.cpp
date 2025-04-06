/**
 * Copyright 2024 Nokia. All rights reserved.
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
*/
// ***********************************************************************************************
#include <gtest/gtest.h>
#include <string>
#include <type_traits>

#include "DataStore.hpp"
#include "UniPtr.hpp"

// ***********************************************************************************************
namespace std
{
    struct Key {
        int except_ = 1;
        Key() = default;
        Key(const Key& aSrc) : except_(aSrc.except_) {  // cp constructor
            if (except_ == 1)
                throw runtime_error("REQ: simulate except from Key constructor");
        }
        bool operator==(const Key& other) const { return except_ == other.except_; }
    };
    template<> struct hash<Key> {
        size_t operator()(const Key& aK) const {
            if (aK.except_ == 1)
                throw runtime_error("REQ: simulate except from Key's hash");
            return std::hash<int>()(aK.except_);
        }
    };
}

// ***********************************************************************************************
using namespace std;
using namespace testing;

namespace rlib
{
struct DataStoreTest : public Test
{
    DataStore<string> dataStore_;
};

#define STORE
// ***********************************************************************************************
TEST_F(DataStoreTest, GOLD_store_diffType)
{
    // init
    EXPECT_EQ(0u, dataStore_.nData()) << "REQ: empty";
    EXPECT_EQ(nullptr, dataStore_.get<int>("int").get()) << "REQ: ret null if no set";

    // 1 inner type
    EXPECT_TRUE(dataStore_.emplaceOK("int", MAKE_PTR<int>(7))) << "REQ: succeed";
    EXPECT_EQ(1u, dataStore_.nData()) << "REQ: insert OK";
    EXPECT_EQ(7, *(dataStore_.get<int>("int").get())) << "REQ: get OK";

    // 1 class type
    EXPECT_TRUE(dataStore_.emplaceOK("string", MAKE_PTR<string>("hello")));
    EXPECT_EQ(2u, dataStore_.nData()) << "req: insert OK";
    EXPECT_EQ("hello", *(dataStore_.get<string>("string").get())) << "req: get OK";
}
TEST_F(DataStoreTest, GOLD_store_sameType)
{
    EXPECT_TRUE(dataStore_.emplaceOK("string", MAKE_PTR<string>("hello")));
    EXPECT_EQ(1u, dataStore_.nData()) << "req: insert OK";
    EXPECT_EQ("hello", *(dataStore_.get<string>("string").get())) << "req: get OK";

    EXPECT_TRUE(dataStore_.emplaceOK("s2", MAKE_PTR<string>("world")));
    EXPECT_EQ(2u, dataStore_.nData()) << "REQ: insert OK for same type with diff DataName";
    EXPECT_EQ("world", *(dataStore_.get<string>("s2").get())) << "req: get OK";
}
TEST_F(DataStoreTest, noNeedToStoreNull)
{
    EXPECT_FALSE(dataStore_.emplaceOK("string", nullptr)) << "REQ: erase NOK";
    EXPECT_EQ(0u, dataStore_.nData()) << "REQ: no need to store null";

    EXPECT_TRUE(dataStore_.emplaceOK("string", MAKE_PTR<string>("hello")));
    EXPECT_EQ(1u, dataStore_.nData()) << "req: insert OK";

    EXPECT_TRUE(dataStore_.emplaceOK("string", nullptr)) << "REQ: erase OK";
    EXPECT_EQ(0u, dataStore_.nData()) << "REQ: erase OK";
}
TEST_F(DataStoreTest, GOLD_replace)
{
    EXPECT_TRUE (dataStore_.emplaceOK("string", MAKE_PTR<string>("hello")));
    EXPECT_FALSE(dataStore_.emplaceOK("string", MAKE_PTR<string>("world"))) << "REQ: failed";
    EXPECT_EQ("hello", *(dataStore_.get<string>("string").get())) << "req: emplaceOK can't replace";

    EXPECT_TRUE(dataStore_.replaceOK("string", MAKE_PTR<string>("world"))) << "REQ: replace OK";
    EXPECT_EQ("world", *(dataStore_.get<string>("string").get())) << "req: explicit rm then set ok";

    EXPECT_TRUE(dataStore_.replaceOK("string", nullptr)) << "REQ: replace/rm OK";
    EXPECT_EQ(nullptr, dataStore_.get<string>("string").get()) << "req: replace/rm OK";
    EXPECT_FALSE(dataStore_.replaceOK("string", nullptr)) << "REQ: replace/rm again NOK";
}

#define SAFE
// ***********************************************************************************************
TEST_F(DataStoreTest, GOLD_safe_lifecycle)
{
    dataStore_.emplaceOK("int", MAKE_PTR<int>(7));
    auto i = dataStore_.get<int>("int");
    *(i.get()) = 8;
    EXPECT_EQ(8, *(dataStore_.get<int>("int").get())) << "REQ: shared";
    EXPECT_EQ(2u, i.use_count()) << "REQ: 2 users";

    dataStore_.emplaceOK("int", nullptr);
    EXPECT_EQ(8, *(i.get())) << "REQ: lifecycle mgmt - not del until no usr";
    EXPECT_EQ(1u, i.use_count()) << "REQ: 1 user";
}
TEST_F(DataStoreTest, GOLD_safe_cast)
{
    dataStore_.emplaceOK("int", MAKE_PTR<int>(7));
    EXPECT_EQ(7, *(dataStore_.get<int>("int").get())) << "REQ: get origin OK";
    EXPECT_EQ(nullptr, dataStore_.get<char>("int").get()) << "REQ: ret null for invalid type (shared_ptr failed here)";

    struct Base                   { virtual int value() const  { return 0; } };
    struct Derive : public Base   { int value() const override { return 1; } };
    struct D2     : public Derive { int value() const override { return 2; } };

    S_PTR<Base> b = MAKE_PTR<Derive>();
    dataStore_.emplaceOK("Base", b);
    EXPECT_EQ(1      , dataStore_.get<Base  >("Base")->value()) << "REQ: get real type";
    EXPECT_EQ(1      , dataStore_.get<Derive>("Base")->value()) << "REQ: get real type";
    EXPECT_EQ(nullptr, dataStore_.get<D2    >("Base").get   ()) << "REQ: get invalid type (shared_ptr failed here)";
}
TEST_F(DataStoreTest, GOLD_safe_destruct)
{
    struct TestBase
    {
        bool& isBaseOver_;
        explicit TestBase(bool& aExtFlag) : isBaseOver_(aExtFlag) { isBaseOver_ = false; }
        virtual ~TestBase() { isBaseOver_ = true; }
    };
    struct TestDerive : public TestBase
    {
        bool& isDeriveOver_;
        explicit TestDerive(bool& aBaseFlag, bool& aDeriveFlag)
            : TestBase(aBaseFlag)
            , isDeriveOver_(aDeriveFlag)
        { isDeriveOver_ = false; }
        ~TestDerive() { isDeriveOver_ = true; }
    };

    bool isBaseOver;
    bool isDeriveOver;
    dataStore_.emplaceOK("Base", MAKE_PTR<TestDerive>(isBaseOver, isDeriveOver));
    EXPECT_FALSE(isBaseOver);
    EXPECT_FALSE(isDeriveOver);

    dataStore_.emplaceOK("Base", nullptr);
    EXPECT_TRUE(isBaseOver);
    EXPECT_TRUE(isDeriveOver);
}
TEST_F(DataStoreTest, except)
{
    DataStore<Key> ds;
    EXPECT_FALSE(ds.emplaceOK(Key(), MAKE_PTR<int>())) << "REQ: emplaceOK() except->fail";  // 1 except as example
    EXPECT_EQ(0u, ds.nData()) << "REQ: except->not stored";

    Key k;
    k.except_ = 0;
    EXPECT_TRUE(ds.emplaceOK(k, MAKE_PTR<int>(0)));
    EXPECT_EQ(1u, ds.nData()) << "REQ: emplace OK";
    k.except_ = 1;
    EXPECT_EQ(nullptr, ds.get<int>(k).get())  << "REQ: get<>() except->nullptr";
    k.except_ = 0;
    EXPECT_EQ(0,     *(ds.get<int>(k).get())) << "REQ: get<>() noexcept->value";

    EXPECT_FALSE(ds.replaceOK(Key(), MAKE_PTR<int>(100))) << "REQ: replaceOK() except->fail";
    EXPECT_EQ(0, *(ds.get<int>(k).get())) << "REQ: no change";
    EXPECT_EQ(1u, ds.nData()) << "REQ: no change";
}

}  // namespace
