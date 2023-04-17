/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - why: simplify multi UT cases' init
// ***********************************************************************************************
#ifndef SWM_UT_INIT_SVC_CLOUD_HPP_
#define SWM_UT_INIT_SVC_CLOUD_HPP_

#include <gtest/gtest.h>
#include <memory>  // shared_ptr

#include "UniLog.hpp"
#include "Domino.hpp"
#include "DataDomino.hpp"
#include "WbasicDatDom.hpp"
#include "HdlrDomino.hpp"
#include "MultiHdlrDomino.hpp"
#include "PriDomino.hpp"
#include "FreeHdlrDomino.hpp"
#include "MsgSelf.hpp"
#include "ObjAnywhere.hpp"

// ***********************************************************************************************
// UT req: combined domino shall pass all UT
#define DOMINO      (ObjAnywhere::get<MaxDom>(*this))
#define NO_FREE_DOM (ObjAnywhere::get<MaxNofreeDom>(*this))
#define PARA_DOM    (ObjAnywhere::get<TypeParam>(*this))

using namespace std;
using namespace testing;

namespace RLib
{
using MinDatDom       = DataDomino<Domino>;
using MinWbasicDatDom = WbasicDatDom<MinDatDom>;
using MinHdlrDom      = HdlrDomino<Domino>;
using MinMhdlrDom     = MultiHdlrDomino<MinHdlrDom>;
using MinPriDom       = PriDomino<MinHdlrDom>;
using MinFreeDom      = FreeHdlrDomino<MinHdlrDom>;

// natural order
using MaxDom = WbasicDatDom<MultiHdlrDomino<DataDomino<FreeHdlrDomino<MinPriDom > > > >;
// diff order
using MaxNofreeDom = PriDomino<MultiHdlrDomino<HdlrDomino<MinWbasicDatDom > > >;

// ***********************************************************************************************
struct UtInitObjAnywhere : public UniLog, public Test
{
    UtInitObjAnywhere() : UniLog(UnitTest::GetInstance()->current_test_info()->name())
    {
        ObjAnywhere::init(*this);

        ObjAnywhere::set(make_shared<MsgSelf>(
            [](const FromMainFN& aHandleAllMsgFn){ aHandleAllMsgFn(); }, uniLogName()), *this);

        ObjAnywhere::set(make_shared<Domino>         (uniLogName()), *this);
        ObjAnywhere::set(make_shared<MinDatDom>      (uniLogName()), *this);
        ObjAnywhere::set(make_shared<MinWbasicDatDom>(uniLogName()), *this);
        ObjAnywhere::set(make_shared<MinHdlrDom>     (uniLogName()), *this);
        ObjAnywhere::set(make_shared<MinMhdlrDom>    (uniLogName()), *this);
        ObjAnywhere::set(make_shared<MinPriDom>      (uniLogName()), *this);
        ObjAnywhere::set(make_shared<MinFreeDom>     (uniLogName()), *this);

        ObjAnywhere::set(make_shared<MaxDom>         (uniLogName()), *this);
        ObjAnywhere::set(make_shared<MaxNofreeDom>   (uniLogName()), *this);
    }
    ~UtInitObjAnywhere()
    {
        ObjAnywhere::deinit(*this);
        GTEST_LOG_FAIL
    }
};
}  // namespace
#endif  // SWM_UT_INIT_SVC_CLOUD_HPP_
