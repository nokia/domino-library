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
    shared_ptr<Domino> anotherDom_ = make_shared<Domino>(uniLogName());
};
TYPED_TEST_SUITE_P(DomDoorTest);

// ***********************************************************************************************
// EvName tree:      A (PARA_DOM)
//                  / \
//                 B   C (anotherDom_)
//                    /
//                   D
TYPED_TEST_P(DomDoorTest, GOLD_most_match)
{
    this->domDoor_.subTree("/A",   PARA_DOM);           // req: can support any type domino
    this->domDoor_.subTree("/A/C", this->anotherDom_);  // simplify (shall also be any type domino)
    ASSERT_NE(PARA_DOM, this->anotherDom_);

    EXPECT_EQ(PARA_DOM,          this->domDoor_.template subTree<TypeParam>("/A"));      // req: exact match
    EXPECT_EQ(PARA_DOM,          this->domDoor_.template subTree<TypeParam>("/A/B"));    // req: most match
    EXPECT_EQ(this->anotherDom_, this->domDoor_.template subTree<TypeParam>("/A/C"));    // req: exact match
    EXPECT_EQ(this->anotherDom_, this->domDoor_.template subTree<TypeParam>("/A/C/D"));  // req: most match

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
