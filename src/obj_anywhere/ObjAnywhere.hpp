/**
 * Copyright 2018 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - ISSUE:
//   . how easily share obj within a process? like cout
//
// - how:
//   . eg ObjAnywhere::emplaceObjOK<Obj>(): create/register/store Obj
//   . eg ObjAnywhere::getObj<Obj>(): get Obj
//
// - more value:
//   . ObjAnywhere not include any Obj.hpp so no cross-include conflict
//   * ObjAnywhere stores SafePtr<Obj> - real store, correct destruct, lifespan management
//
// - core: name_obj_S_
//
// - mem-safe: true (when use SafePtr instead of shared_ptr)
// - MT safe: false (only use in main-thread)
// ***********************************************************************************************
#pragma once

#include <string>

#include "DataStore.hpp"
#include "UniLog.hpp"
#include "UniPtr.hpp"

namespace rlib
{
using ObjName = std::string;

// ***********************************************************************************************
class ObjAnywhere
{
public:
    static void init(UniLog& = UniLog::defaultUniLog_) noexcept;
    static void deinit() noexcept;  // rm name_obj_S_
    static bool isInit() noexcept { return name_obj_S_ != nullptr; }
    static size_t nObj() noexcept { return name_obj_S_ ? name_obj_S_->nData() : 0; }

    // @brief: store an obj
    // @param SafePtr<aObjType>: an obj to be stored
    // @param UniLog           : log
    // @param ObjName          : key of the obj; default is typeid(aObjType).name()
    template<typename aObjType> static bool emplaceObjOK(S_PTR<aObjType>,
        UniLog& = UniLog::defaultUniLog_, const ObjName& = typeid(aObjType).name()) noexcept;

    // @brief: get an obj
    // @param ObjName: key of the obj when stored; default is typeid(aObjType).name()
    // @ret: ok or nullptr
    template<typename aObjType> static
    S_PTR<aObjType> getObj(const ObjName& = typeid(aObjType).name()) noexcept;

private:
    // -------------------------------------------------------------------------------------------
    static std::shared_ptr<DataStore<ObjName>> name_obj_S_;  // store aObj w/o include aObj.hpp; shared_ptr is safe here
};

// ***********************************************************************************************
template<typename aObjType>
S_PTR<aObjType> ObjAnywhere::getObj(const ObjName& aObjName) noexcept
{
    return isInit()
        ? name_obj_S_->get<aObjType>(aObjName)
        : nullptr;
}

// ***********************************************************************************************
template<typename aObjType>
bool ObjAnywhere::emplaceObjOK(S_PTR<aObjType> aObj, UniLog& oneLog, const ObjName& aObjName) noexcept
{
    if (isInit())
        return name_obj_S_->emplaceOK(aObjName, aObj);

    ERR("(ObjAnywhere) !!! Failed, pls call ObjAnywhere::init() beforehand.");
    return false;
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
// 2025-02-13  CSZ       - support both SafePtr & shared_ptr
// 2025-03-28  CSZ       9)tolerate exception
// ***********************************************************************************************
