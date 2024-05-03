/**
 * Copyright 2018 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - ISSUE:
//   . how easily share obj within a process? like cout sharing
//
// - how:
//   . eg ObjAnywhere::set<Obj>(): register/store Obj
//   . eg ObjAnywhere::get<Obj>(): get Obj
//
// - more value:
//   . ObjAnywhere not include any Obj.hpp so no cross-include conflict
//   * ObjAnywhere stores shared_ptr<Obj> - real store, correct destruct, lifespan mgr
//
// - core: idx_obj_S_
//
// - mem-safe: true (when use SafeAdr instead of shared_ptr)
//
// - note:
//   . Obj can be destructed by its own destructor when shared_ptr<Obj>.use_count()==0
//   . it's possible after deinit() that Obj still exists since its use_count()>0
//   . if req to store same Obj multiply, use DataDomino
//
// - can DataDomino replace ObjAnywhere?
//   . somewhat yes
//   . but ObjAnywhere will be more complex since ObjIndex is replaced by EvName
//   . and DataDomino will be more complex to manage sharing/static store_
//   . = merge DataDomino & ObjAnywhere, so now giveup
// ***********************************************************************************************
#pragma once

#include <unordered_map>
#include <typeinfo>  // typeid()

#include "UniLog.hpp"
#include "UniPtr.hpp"

using namespace std;

namespace RLib
{
// ***********************************************************************************************
class ObjAnywhere
{
public:
    using ObjIndex = const type_info*;
    using ObjStore = unordered_map<ObjIndex, UniPtr>;

    static void init(UniLog& = UniLog::defaultUniLog_);    // init idx_obj_S_
    static void deinit(UniLog& = UniLog::defaultUniLog_);  // rm idx_obj_S_
    static bool isInit() { return idx_obj_S_ != nullptr; }  // init idx_obj_S_?
    static size_t nObj() { return idx_obj_S_ ? idx_obj_S_->size() : 0; }

    // -------------------------------------------------------------------------------------------
    // - save aObjType into idx_obj_S_
    // -------------------------------------------------------------------------------------------
    template<typename aObjType> static void set(PTR<aObjType> aSharedObj, UniLog& = UniLog::defaultUniLog_);

    // -------------------------------------------------------------------------------------------
    // - get a "Obj" from idx_obj_S_
    // - template operator[] not easier in usage
    // -------------------------------------------------------------------------------------------
    template<typename aObjType> static PTR<aObjType> get(UniLog& = UniLog::defaultUniLog_);

private:
    // -------------------------------------------------------------------------------------------
    static shared_ptr<ObjStore> idx_obj_S_;  // store shared_ptr<aSvc> w/o include aObj.hpp
};

// ***********************************************************************************************
template<typename aObjType>
PTR<aObjType> ObjAnywhere::get(UniLog& oneLog)
{
    if (not isInit())
        return nullptr;

    auto&& idx_obj = idx_obj_S_->find(&typeid(aObjType));
    if (idx_obj != idx_obj_S_->end())
        return static_pointer_cast<aObjType>(idx_obj->second);

    INF("(ObjAnywhere) !!! Failed, unavailable obj=" << typeid(aObjType).name() << " in ObjAnywhere.");
    return nullptr;
}

// ***********************************************************************************************
template<typename aObjType>
void ObjAnywhere::set(PTR<aObjType> aSharedObj, UniLog& oneLog)
{
    if (not isInit())
    {
        ERR("(ObjAnywhere) !!! Failed, pls call ObjAnywhere::init() beforehand.");
        return;
    }

    auto objIndex = &typeid(aObjType);
    if (aSharedObj.get() == nullptr)
    {
        idx_obj_S_->erase(objIndex);  // natural expectation
        INF("(ObjAnywhere) Removed obj=" << typeid(aObjType).name() << " from ObjAnywhere.");
        return;
    }

    if (idx_obj_S_->find(objIndex) == idx_obj_S_->end())
        HID("(ObjAnywhere) Set obj=" << typeid(aObjType).name() << " into ObjAnywhere.")
    else
        INF("(ObjAnywhere) !!!Replace obj=" << typeid(aObjType).name() << " in ObjAnywhere.");
    (*idx_obj_S_)[objIndex] = aSharedObj;
}

}  // namespace
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2018-04-08  CSZ       1)create
// 2018-05-07  CSZ       - not self ObjIndex but typeid.hash_code()
// 2020-07-24  CSZ       2)can store object w/o including object.hpp
// 2020-08-14  CSZ       - log compitable to CMT
// 2020-09-28  CSZ       3)SvcCloud->SvcAnywhere: Anywhere is core value, Cloud is not so accurate
// 2020-10-06  CSZ       - more robust
// 2021-01-22  CSZ       - set(null) -> rm obj
// 2021-03-01  CSZ       - static all mem func, include init & deinit, so no construct obj in usage
//                       & better understanding this class
// 2021-03-02  CSZ       4)SvcAnywhere->ObjAnywhere (more direct naming)
//                       5)coding req by UT case
// 2021-04-05  CSZ       - coding req
// 2021-04-30  ZQ        6)ZhangQiang told me shared_ptr<void> can del Obj correctly
// 2022-01-01  PJ & CSZ  - formal log & naming
// 2022-08-18  CSZ       - replace CppLog by UniLog
// 2022-12-02  CSZ       - simple & natural
// 2024-02-15  CSZ       7)use SafeAdr (mem-safe); shared_ptr is not mem-safe
// ***********************************************************************************************
