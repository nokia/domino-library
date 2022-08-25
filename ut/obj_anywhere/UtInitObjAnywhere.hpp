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

namespace RLib
{
using MinDatDom       = DataDomino<Domino>;
using MinWbasicDatDom = WbasicDatDom<MinDatDom>;
using MinHdlrDom      = HdlrDomino<Domino>;
using MinMhdlrDom     = MultiHdlrDomino<MinHdlrDom>;
using MinPriDom       = PriDomino<MinHdlrDom>;
using MinFreeDom      = FreeHdlrDomino<MinHdlrDom>;

// natural order
using MaxDom = WbasicDatDom<MultiHdlrDomino<DataDomino<FreeHdlrDomino<PriDomino<HdlrDomino<Domino> > > > > >;
// diff order
using MaxNofreeDom = PriDomino<MultiHdlrDomino<HdlrDomino<WbasicDatDom<DataDomino<Domino> > > > >;

// ***********************************************************************************************
struct UtInitObjAnywhere : public UniLog
{
    UtInitObjAnywhere(const UniLogName& aCellName = ULN_DEFAULT) : UniLog(aCellName)
    {
        ObjAnywhere::init(*this);

        ObjAnywhere::set<MsgSelf>(std::make_shared<MsgSelf>(
            [](LoopBackFUNC aFunc){ aFunc(); }, cellName()), *this);

        ObjAnywhere::set<Domino>(std::make_shared<Domino>(cellName()), *this);
        ObjAnywhere::set<MinDatDom>(std::make_shared<MinDatDom>(cellName()), *this);
        ObjAnywhere::set<MinWbasicDatDom>(std::make_shared<MinWbasicDatDom>(cellName()), *this);
        ObjAnywhere::set<MinHdlrDom>(std::make_shared<MinHdlrDom>(cellName()), *this);
        ObjAnywhere::set<MinMhdlrDom>(std::make_shared<MinMhdlrDom>(cellName()), *this);
        ObjAnywhere::set<MinPriDom>(std::make_shared<MinPriDom>(cellName()), *this);
        ObjAnywhere::set<MinFreeDom>(std::make_shared<MinFreeDom>(cellName()), *this);

        ObjAnywhere::set<MaxDom>(std::make_shared<MaxDom>(cellName()), *this);
        ObjAnywhere::set<MaxNofreeDom>(std::make_shared<MaxNofreeDom>(cellName()), *this);
    }
    ~UtInitObjAnywhere() { ObjAnywhere::deinit(*this); }
};
}  // namespace
#endif  // SWM_UT_INIT_SVC_CLOUD_HPP_
