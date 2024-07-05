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
#define DOMINO      (ObjAnywhere::getObj<MaxDom>      ().get())
#define NO_FREE_DOM (ObjAnywhere::getObj<MaxNofreeDom>().get())
#define PARA_DOM    (ObjAnywhere::getObj<TypeParam>   ().get())

using namespace std;
using namespace testing;

namespace RLib
{
using MinHdlrDom  =                 HdlrDomino<Domino>;
using MinMhdlrDom = MultiHdlrDomino<MinHdlrDom>;
using MinPriDom   =       PriDomino<MinHdlrDom>;
using MinFreeDom  =  FreeHdlrDomino<MinHdlrDom>;

using MinDatDom =                                                              DataDomino<Domino>;
using MinWbasicDatDom =                                           WbasicDatDom<MinDatDom>;
using MaxNofreeDom = RmEvDom<PriDomino<MultiHdlrDomino<HdlrDomino<MinWbasicDatDom>>>>;  // diff order

using MinRmEvDom =                                                                         RmEvDom<Domino>;
using MaxDom = WbasicDatDom<MultiHdlrDomino<DataDomino<FreeHdlrDomino<PriDomino<HdlrDomino<MinRmEvDom>>>>>>;


// ***********************************************************************************************
struct UtInitObjAnywhere : public UniLog, public Test
{
    UtInitObjAnywhere() : UniLog(UnitTest::GetInstance()->current_test_info()->name())
    {
        ObjAnywhere::init(*this);

        ObjAnywhere::emplaceObjOK(make_safe<MsgSelf>(uniLogName()), *this);

        ObjAnywhere::emplaceObjOK(make_safe<Domino>         (uniLogName()), *this);
        ObjAnywhere::emplaceObjOK(make_safe<MinDatDom>      (uniLogName()), *this);
        ObjAnywhere::emplaceObjOK(make_safe<MinWbasicDatDom>(uniLogName()), *this);
        ObjAnywhere::emplaceObjOK(make_safe<MinHdlrDom>     (uniLogName()), *this);
        ObjAnywhere::emplaceObjOK(make_safe<MinMhdlrDom>    (uniLogName()), *this);
        ObjAnywhere::emplaceObjOK(make_safe<MinPriDom>      (uniLogName()), *this);
        ObjAnywhere::emplaceObjOK(make_safe<MinFreeDom>     (uniLogName()), *this);
        ObjAnywhere::emplaceObjOK(make_safe<MinRmEvDom>     (uniLogName()), *this);

        ObjAnywhere::emplaceObjOK(make_safe<MaxDom>         (uniLogName()), *this);
        ObjAnywhere::emplaceObjOK(make_safe<MaxNofreeDom>   (uniLogName()), *this);

        // - example how main() callback MsgSelf to handle all msgs
        // - this lambda hides all impl details but a common interface = function<void()>
        pongMsgSelf_ = [msgSelf = MSG_SELF]{ msgSelf->handleAllMsg(); };
    }
    ~UtInitObjAnywhere()
    {
        ObjAnywhere::deinit();
        GTEST_LOG_FAIL
    }

    // -------------------------------------------------------------------------------------------
    MsgCB pongMsgSelf_;
};

}  // namespace
