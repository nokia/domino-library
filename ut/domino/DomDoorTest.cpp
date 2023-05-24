/**
 * Copyright 2018-2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include <memory>

#include "DomDoor.hpp"
#include "UtInitObjAnywhere.hpp"

namespace RLib
{
// ***********************************************************************************************
template<class aParaDom>
struct DomDoorTest : public UtInitObjAnywhere
{
    // -------------------------------------------------------------------------------------------
    DomDoor domDoor_ = DomDoor(uniLogName());
    shared_ptr<Domino> dom2_ = make_shared<Domino>(uniLogName());
    shared_ptr<Domino> dom3_ = make_shared<Domino>(uniLogName());
};
TYPED_TEST_SUITE_P(DomDoorTest);

// ***********************************************************************************************
// EvName tree:      A (PARA_DOM)    a (dom3_)
//                  / \               \
//                 B   C (dom2_)       C
//                    /
//                   D
TYPED_TEST_P(DomDoorTest, GOLD_most_match)
{
    EXPECT_TRUE(this->domDoor_.setSubTreeOK("/A",   PARA_DOM));     // req: can support any type domino
    EXPECT_TRUE(this->domDoor_.setSubTreeOK("/A/C", this->dom2_));  // simplify (shall also be any type domino)
    EXPECT_TRUE(this->domDoor_.setSubTreeOK("/a",   this->dom3_));  // req: case sensitive; & inc code-cov
    ASSERT_NE(PARA_DOM, this->dom2_);

    EXPECT_EQ(PARA_DOM,    this->domDoor_.template subTree<TypeParam>("/A"));      // req: exact match
    EXPECT_EQ(PARA_DOM,    this->domDoor_.template subTree<TypeParam>("/A/B"));    // req: most match
    EXPECT_EQ(this->dom2_, this->domDoor_.template subTree<TypeParam>("/A/C"));    // req: exact match
    EXPECT_EQ(this->dom3_, this->domDoor_.template subTree<TypeParam>("/a/C"));    // req: exact match
    EXPECT_EQ(this->dom2_, this->domDoor_.template subTree<TypeParam>("/A/C/D"));  // req: most match

    EXPECT_FALSE(this->domDoor_.template subTree<TypeParam>("/E"));  // req: no match
    EXPECT_FALSE(this->domDoor_.template subTree<TypeParam>("/"));   // req: no match
    EXPECT_FALSE(this->domDoor_.template subTree<TypeParam>(""));    // req: no match

    EXPECT_TRUE(this->domDoor_.setSubTreeOK("/A/C"));  // req: rm dom
    EXPECT_EQ(PARA_DOM, this->domDoor_.template subTree<TypeParam>("/A/C"));    // req: exact match
    EXPECT_EQ(PARA_DOM, this->domDoor_.template subTree<TypeParam>("/A/C/D"));  // req: most match
}
TYPED_TEST_P(DomDoorTest, set_fail)
{
    EXPECT_TRUE(this->domDoor_.setSubTreeOK("/A",   PARA_DOM));
    PARA_DOM->newEvent("/A/C");
    EXPECT_FALSE(this->domDoor_.setSubTreeOK("/A/C", this->dom2_));    // req: fail to avoid "/A/C" in 2 dom
    //EXPECT_FALSE(this->domDoor_.setSubTreeOK("/A/C/D", this->dom2_));  // req: fail to avoid "/A/C/D" in 2 dom
}

// ***********************************************************************************************
REGISTER_TYPED_TEST_SUITE_P(DomDoorTest
    , GOLD_most_match
    , set_fail
);
using AnyDatDom = Types<Domino, MinDatDom, MinWbasicDatDom, MinHdlrDom, MinMhdlrDom,
    MinPriDom, MinFreeDom, MaxNofreeDom, MaxDom>;
INSTANTIATE_TYPED_TEST_SUITE_P(PARA, DomDoorTest, AnyDatDom);
}  // namespace
