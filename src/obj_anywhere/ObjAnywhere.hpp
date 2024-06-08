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
//   . eg ObjAnywhere::setObj<Obj>(): register/store Obj
//   . eg ObjAnywhere::getObj<Obj>(): get Obj
//
// - more value:
//   . ObjAnywhere not include any Obj.hpp so no cross-include conflict
//   * ObjAnywhere stores shared_ptr<Obj> - real store, correct destruct, lifespan mgr
//
// - core: name_obj_S_
//
// - mem-safe: true (when use SafePtr instead of shared_ptr)
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

#include "DataStore.hpp"
#include "UniLog.hpp"

using namespace std;

namespace RLib
{
// ***********************************************************************************************
class ObjAnywhere
{
public:
    static void init(UniLog& = UniLog::defaultUniLog_);    // init name_obj_S_
    static void deinit();  // rm name_obj_S_
    static bool isInit() { return name_obj_S_ != nullptr; }  // init name_obj_S_?
    static size_t nObj() { return name_obj_S_ ? name_obj_S_->nData() : 0; }

    // typeid().name() is to compatible with previous interface (w/o aObjName), eg ::get<TypeParam>
    template<typename aObjType> static
    void setObj(SafePtr<aObjType>, UniLog& = UniLog::defaultUniLog_, const DataKey& = typeid(aObjType).name());
    template<typename aObjType> static
    SafePtr<aObjType> getObj(const DataKey& = typeid(aObjType).name());

private:
    // -------------------------------------------------------------------------------------------
    static shared_ptr<DataStore> name_obj_S_;  // store aObj w/o include aObj.hpp
};

// ***********************************************************************************************
template<typename aObjType>
SafePtr<aObjType> ObjAnywhere::getObj(const DataKey& aObjName)
{
    if (isInit())
        return name_obj_S_->get<aObjType>(aObjName);
    return nullptr;
}

// ***********************************************************************************************
template<typename aObjType>
void ObjAnywhere::setObj(SafePtr<aObjType> aObj, UniLog& oneLog, const DataKey& aObjName)
{
    if (isInit())
        name_obj_S_->set(aObjName, aObj);
    else
        ERR("(ObjAnywhere) !!! Failed, pls call ObjAnywhere::init() beforehand.");
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
// 2024-02-15  CSZ       7)use SafePtr (mem-safe); shared_ptr is not mem-safe
// 2024-06-07  CSZ       8)use DataStore instead of map
// ***********************************************************************************************
