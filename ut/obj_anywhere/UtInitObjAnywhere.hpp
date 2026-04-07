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

#include "UniLog.hpp"
#include "UniPtr.hpp"
#include "MsgSelf.hpp"
#include "ObjAnywhere.hpp"

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

using namespace testing;

namespace rlib
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

        EXPECT_TRUE(ObjAnywhere::emplaceObjOK(MAKE_PTR<MsgSelf>(uniLogName()), *this))
            << "REQ: init MsgSelf";

        EXPECT_TRUE(ObjAnywhere::emplaceObjOK(MAKE_PTR<Domino>         (uniLogName()), *this))
            << "REQ: init Domino";
        EXPECT_TRUE(ObjAnywhere::emplaceObjOK(MAKE_PTR<MinDatDom>      (uniLogName()), *this))
            << "REQ: init MinDatDom";
        EXPECT_TRUE(ObjAnywhere::emplaceObjOK(MAKE_PTR<MinWbasicDatDom>(uniLogName()), *this))
            << "REQ: init MinWbasicDatDom";
        EXPECT_TRUE(ObjAnywhere::emplaceObjOK(MAKE_PTR<MinHdlrDom>     (uniLogName()), *this))
            << "REQ: init MinHdlrDom";
        EXPECT_TRUE(ObjAnywhere::emplaceObjOK(MAKE_PTR<MinMhdlrDom>    (uniLogName()), *this))
            << "REQ: init MinMhdlrDom";
        EXPECT_TRUE(ObjAnywhere::emplaceObjOK(MAKE_PTR<MinPriDom>      (uniLogName()), *this))
            << "REQ: init MinPriDom";
        EXPECT_TRUE(ObjAnywhere::emplaceObjOK(MAKE_PTR<MinFreeDom>     (uniLogName()), *this))
            << "REQ: init MinFreeDom";
        EXPECT_TRUE(ObjAnywhere::emplaceObjOK(MAKE_PTR<MinRmEvDom>     (uniLogName()), *this))
            << "REQ: init MinRmEvDom";

        EXPECT_TRUE(ObjAnywhere::emplaceObjOK(MAKE_PTR<MaxDom>         (uniLogName()), *this))
            << "REQ: init MaxDom";
        EXPECT_TRUE(ObjAnywhere::emplaceObjOK(MAKE_PTR<MaxNofreeDom>   (uniLogName()), *this))
            << "REQ: init MaxNofreeDom";

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

// ***********************************************************************************************
// measure actual mem/event via RSS (Linux /proc/self/statm)
static size_t rssBytes()
{
    size_t rss = 0;
    FILE* f = fopen("/proc/self/statm", "r");
    if (f) {
        unsigned long dummy, pages;
        if (fscanf(f, "%lu %lu", &dummy, &pages) == 2) rss = pages * 4096;
        fclose(f);
    }
    return rss;
}

}  // namespace
