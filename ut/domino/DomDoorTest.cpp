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
    DomDoor domDoor_;
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
    this->domDoor_.subTree("/A",   PARA_DOM);     // req: can support any type domino
    this->domDoor_.subTree("/A/C", this->dom2_);  // simplify (shall also be any type domino)
    this->domDoor_.subTree("/a",   this->dom3_);  // req: case sensitive; & inc code-cov
    ASSERT_NE(PARA_DOM, this->dom2_);

    EXPECT_EQ(PARA_DOM,    this->domDoor_.template subTree<TypeParam>("/A",     *this));  // req: exact match
    EXPECT_EQ(PARA_DOM,    this->domDoor_.template subTree<TypeParam>("/A/B",   *this));  // req: most match
    EXPECT_EQ(this->dom2_, this->domDoor_.template subTree<TypeParam>("/A/C",   *this));  // req: exact match
    EXPECT_EQ(this->dom3_, this->domDoor_.template subTree<TypeParam>("/a/C",   *this));  // req: exact match
    EXPECT_EQ(this->dom2_, this->domDoor_.template subTree<TypeParam>("/A/C/D", *this));  // req: most match

    EXPECT_FALSE(this->domDoor_.template subTree<TypeParam>("/E"));  // req: no match
    EXPECT_FALSE(this->domDoor_.template subTree<TypeParam>("/"));   // req: no match
    EXPECT_FALSE(this->domDoor_.template subTree<TypeParam>(""));    // req: no match
}

// ***********************************************************************************************
REGISTER_TYPED_TEST_SUITE_P(DomDoorTest
    , GOLD_most_match
);
using AnyDatDom = Types<Domino, MinDatDom, MinWbasicDatDom, MinHdlrDom, MinMhdlrDom,
    MinPriDom, MinFreeDom, MaxNofreeDom, MaxDom>;
INSTANTIATE_TYPED_TEST_SUITE_P(PARA, DomDoorTest, AnyDatDom);
}  // namespace
