/**
 * Copyright 2018 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - ISSUE:
//   . how easily share obj within a process? like cout sharing
// - how:
//   . eg ObjAnywhere::set<Obj>(): register/store Obj
//   . eg ObjAnywhere::get<Obj>(): get Obj
// - more value:
//   . ObjAnywhere not include any Obj.hpp so no cross-include conflict
//   * ObjAnywhere stores shared_ptr<Obj>, not shared_ptr<void> (real store, correct destruct, lifespan mgr)
// - core: objStore_
// - note:
//   . Obj can be destructed by its own destructor when shared_ptr<Obj>.use_count()==0
//   . it's possible after deinit() that Obj still exists since its use_count()>0
//   . if req to store same Obj multiply, use DataDomino
// - can DataDomino replace ObjAnywhere?
//   . somewhat yes
//   . but ObjAnywhere will be more complex since ObjIndex is replaced by EvName
//   . and DataDomino will be more complex to manage sharing/static store_
//   . = merge DataDomino & ObjAnywhere, so now giveup
// ***********************************************************************************************
#ifndef SERVICE_ANYWHERE_HPP_
#define SERVICE_ANYWHERE_HPP_

#include <unordered_map>
#include <memory>    // for shared_ptr
#include <typeinfo>  // typeid()

#include "UniLog.hpp"

using namespace std;

namespace RLib
{
// ***********************************************************************************************
class ObjAnywhere
{
public:
    using ObjIndex = size_t;
    using ObjStore = unordered_map<ObjIndex, shared_ptr<void> >;

    static void init(UniLog& = UniLog::defaultUniLog());    // init objStore_
    static void deinit(UniLog& = UniLog::defaultUniLog());  // rm objStore_
    static bool isInit() { return objStore_ != nullptr; }   // init objStore_?

    // -------------------------------------------------------------------------------------------
    // - save aObjType into objStore_
    // -------------------------------------------------------------------------------------------
    template<typename aObjType>
    static void set(shared_ptr<aObjType> aSharedObj, UniLog& = UniLog::defaultUniLog());

    // -------------------------------------------------------------------------------------------
    // - get a "Obj" from objStore_
    // - template operator[] not easier in usage
    // -------------------------------------------------------------------------------------------
    template<typename aObjType> static shared_ptr<aObjType> get(UniLog& = UniLog::defaultUniLog());

private:
    // -------------------------------------------------------------------------------------------
    static shared_ptr<ObjStore> objStore_;
};

// ***********************************************************************************************
template<typename aObjType>
shared_ptr<aObjType> ObjAnywhere::get(UniLog& oneLog)
{
    if (not isInit()) return shared_ptr<aObjType>();

    auto&& found = objStore_->find(typeid(aObjType).hash_code());
    if (found != objStore_->end()) return static_pointer_cast<aObjType>(found->second);

    INF("(ObjAnywhere) !!! Failed, unavailable obj=" << typeid(aObjType).name() << " in ObjAnywhere.");
    return shared_ptr<aObjType>();
}

// ***********************************************************************************************
template<typename aObjType>
void ObjAnywhere::set(shared_ptr<aObjType> aSharedObj, UniLog& oneLog)
{
    if (not isInit())
    {
        WRN("(ObjAnywhere) !!! Failed, ObjAnywhere is not initialized yet.");
        return;
    }

    auto&& objIndex = typeid(aObjType).hash_code();
    if (not aSharedObj)
    {
        objStore_->erase(objIndex);  // natural expectation
        INF("(ObjAnywhere) Removed obj=" << typeid(aObjType).name() << " from ObjAnywhere.");
        return;
    }

    auto&& found = objStore_->find(objIndex);
    if (found == objStore_->end())
        INF("(ObjAnywhere) Set obj=" << typeid(aObjType).name() << " into ObjAnywhere.")
    else
        INF("(ObjAnywhere) !!!Replace obj=" << typeid(aObjType).name() << " in ObjAnywhere.");
    (*objStore_)[objIndex] = aSharedObj;
}
}  // namespace
#endif  // SERVICE_ANYWHERE_HPP_
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2018-04-08  CSZ       1)create
// 2018-05-07  CSZ       - not self ObjIndex but typeid.hash_code()
// 2020-07-24  CSZ       2)can store object w/o including object.hpp
// 2020-08-14  CSZ       - log compitable to CMT
// 2020-09-28  CSZ       3)SvcCloud->ObjAnywhere: Anywhere is core value, Cloud is not so accurate
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
// ***********************************************************************************************
