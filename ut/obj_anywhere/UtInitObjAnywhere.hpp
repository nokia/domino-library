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
#define DOMINO      (ObjAnywhere::get<MaxDom>())
#define NO_FREE_DOM (ObjAnywhere::get<MaxNofreeDom>())
#define PARA_DOM    (ObjAnywhere::get<TypeParam>())

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
struct UtInitObjAnywhere
{
    UtInitObjAnywhere()
    {
        ObjAnywhere::init();

        ObjAnywhere::set<MsgSelf>(std::make_shared<MsgSelf>([](LoopBackFUNC aFunc){ aFunc(); }));

        ObjAnywhere::set<Domino>(std::make_shared<Domino>());
        ObjAnywhere::set<MinDatDom>(std::make_shared<MinDatDom>());
        ObjAnywhere::set<MinWbasicDatDom>(std::make_shared<MinWbasicDatDom>());
        ObjAnywhere::set<MinHdlrDom>(std::make_shared<MinHdlrDom>());
        ObjAnywhere::set<MinMhdlrDom>(std::make_shared<MinMhdlrDom>());
        ObjAnywhere::set<MinPriDom>(std::make_shared<MinPriDom>());
        ObjAnywhere::set<MinFreeDom>(std::make_shared<MinFreeDom>());

        ObjAnywhere::set<MaxDom>(std::make_shared<MaxDom>());
        ObjAnywhere::set<MaxNofreeDom>(std::make_shared<MaxNofreeDom>());
    }
    ~UtInitObjAnywhere() { ObjAnywhere::deinit(); }
};
}  // namespace
#endif  // SWM_UT_INIT_SVC_CLOUD_HPP_
