/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - why: simplify multi UT cases' init
// ***********************************************************************************************
#pragma once

#include <gtest/gtest.h>
#include <memory>  // shared_ptr

#include "MsgSelf.hpp"
#include "ObjAnywhere.hpp"
#include "UniLog.hpp"

#include "Domino.hpp"
#include "DataDomino.hpp"
#include "WbasicDatDom.hpp"
#include "HdlrDomino.hpp"
#include "MultiHdlrDomino.hpp"
#include "PriDomino.hpp"
#include "FreeHdlrDomino.hpp"
#include "RmEvDom.hpp"

// ***********************************************************************************************
// UT req: combined domino shall pass all UT
#define DOMINO      (ObjAnywhere::get<MaxDom>      (*this).get())
#define NO_FREE_DOM (ObjAnywhere::get<MaxNofreeDom>(*this).get())
#define PARA_DOM    (ObjAnywhere::get<TypeParam>   (*this).get())

using namespace std;
using namespace testing;

namespace RLib
{
using MinDatDom       =              DataDomino<Domino>;
using MinWbasicDatDom = WbasicDatDom<MinDatDom>;

using MinHdlrDom      =                 HdlrDomino<Domino>;
using MinMhdlrDom     = MultiHdlrDomino<MinHdlrDom>;
using MinPriDom       =       PriDomino<MinHdlrDom>;
using MinFreeDom      =  FreeHdlrDomino<MinHdlrDom>;

using MinRmEvDom      = RmEvDom<Domino>;

// natural order
using MaxDom = WbasicDatDom<MultiHdlrDomino<DataDomino<FreeHdlrDomino<PriDomino<HdlrDomino<RmEvDom<Domino>>>>>>>;
// diff order
using MaxNofreeDom = RmEvDom<PriDomino<MultiHdlrDomino<HdlrDomino<WbasicDatDom<DataDomino<Domino>>>>>>;

// ***********************************************************************************************
struct UtInitObjAnywhere : public UniLog, public Test
{
    UtInitObjAnywhere() : UniLog(UnitTest::GetInstance()->current_test_info()->name())
    {
        ObjAnywhere::init(*this);

        ObjAnywhere::set(MAKE_PTR<MsgSelf>(uniLogName()), *this);

        ObjAnywhere::set(MAKE_PTR<Domino>         (uniLogName()), *this);
        ObjAnywhere::set(MAKE_PTR<MinDatDom>      (uniLogName()), *this);
        ObjAnywhere::set(MAKE_PTR<MinWbasicDatDom>(uniLogName()), *this);
        ObjAnywhere::set(MAKE_PTR<MinHdlrDom>     (uniLogName()), *this);
        ObjAnywhere::set(MAKE_PTR<MinMhdlrDom>    (uniLogName()), *this);
        ObjAnywhere::set(MAKE_PTR<MinPriDom>      (uniLogName()), *this);
        ObjAnywhere::set(MAKE_PTR<MinFreeDom>     (uniLogName()), *this);
        ObjAnywhere::set(MAKE_PTR<MinRmEvDom>     (uniLogName()), *this);

        ObjAnywhere::set(MAKE_PTR<MaxDom>         (uniLogName()), *this);
        ObjAnywhere::set(MAKE_PTR<MaxNofreeDom>   (uniLogName()), *this);
    }
    ~UtInitObjAnywhere()
    {
        ObjAnywhere::deinit(*this);
        GTEST_LOG_FAIL
    }
};

}  // namespace
