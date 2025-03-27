/**
 * Copyright 2021 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - what:     simple/easy write-protect for DataDomino
// - why:      eg Yang RW para need write-protect
// - core:     wrCtrl_
// - mem-safe: true (when use SafePtr instead of shared_ptr)
// ***********************************************************************************************
#pragma once

#include <vector>

#include "UniLog.hpp"
#include "UniPtr.hpp"

namespace rlib
{
// ***********************************************************************************************
template<typename aDominoType>
class WbasicDatDom : public aDominoType
{
public:
    explicit WbasicDatDom(const LogName& aUniLogName = ULN_DEFAULT) : aDominoType(aUniLogName) {}

    bool isWrCtrl(const Domino::EvName&) const;
    bool wrCtrlOk(const Domino::EvName&, const bool aNewState = true);

    S_PTR<void> getData(const Domino::EvName&) const override;
    S_PTR<void> wbasic_getData(const Domino::EvName&) const;

    bool replaceDataOK(const Domino::EvName&, S_PTR<void> aData = nullptr) noexcept override;
    bool wbasic_replaceDataOK(const Domino::EvName&, S_PTR<void> aData = nullptr) noexcept;

protected:
    void rmEv_(const Domino::Event& aValidEv) override;

private:
    // forbid ouside usage
    using aDominoType::getData;
    using aDominoType::replaceDataOK;
    // -------------------------------------------------------------------------------------------
    std::vector<bool> wrCtrl_;

public:
    using aDominoType::oneLog;
};

// ***********************************************************************************************
template<typename aDominoType>
S_PTR<void> WbasicDatDom<aDominoType>::getData(const Domino::EvName& aEvName) const
{
    if (not isWrCtrl(aEvName))
        return aDominoType::getData(aEvName);

    WRN("(WbasicDatDom) Failed!!! EvName=" << aEvName << " is not write-protect so unavailable via this func!!!");
    return nullptr;
}

// ***********************************************************************************************
template<typename aDominoType>
bool WbasicDatDom<aDominoType>::isWrCtrl(const Domino::EvName& aEvName) const
{
    const auto ev = this->getEventBy(aEvName);
    return ev < wrCtrl_.size() ? wrCtrl_[ev] : false;
}

// ***********************************************************************************************
template<typename aDominoType>
bool WbasicDatDom<aDominoType>::replaceDataOK(const Domino::EvName& aEvName, S_PTR<void> aData) noexcept
{
    if (isWrCtrl(aEvName)) {
        WRN("(WbasicDatDom) Failed!!! EvName=" << aEvName << " is not write-protect so unavailable via this func!!!")
        return false;
    }
    else return aDominoType::replaceDataOK(aEvName, aData);
}

// ***********************************************************************************************
template<typename aDominoType>
void WbasicDatDom<aDominoType>::rmEv_(const Domino::Event& aValidEv)
{
    if (aValidEv < wrCtrl_.size())
        wrCtrl_[aValidEv] = false;

    aDominoType::rmEv_(aValidEv);
}

// ***********************************************************************************************
template<typename aDominoType>
S_PTR<void> WbasicDatDom<aDominoType>::wbasic_getData(const Domino::EvName& aEvName) const
{
    if (isWrCtrl(aEvName))
        return aDominoType::getData(aEvName);

    WRN("(WbasicDatDom) Failed!!! EvName=" << aEvName << " is not write-protect so unavailable via this func!!!");
    return nullptr;
}

// ***********************************************************************************************
template<typename aDominoType>
bool WbasicDatDom<aDominoType>::wbasic_replaceDataOK(const Domino::EvName& aEvName, S_PTR<void> aData) noexcept
{
    if (isWrCtrl(aEvName))
        return aDominoType::replaceDataOK(aEvName, aData);
    else {
        WRN("(WbasicDatDom) Failed!!! EvName=" << aEvName << " is not write-protect so unavailable via this func!!!")
        return false;
    }
}

// ***********************************************************************************************
template<typename aDominoType>
bool WbasicDatDom<aDominoType>::wrCtrlOk(const Domino::EvName& aEvName, const bool aNewState)
{
    if (aDominoType::getData(aEvName).get() != nullptr)
    {
        WRN("(WbasicDatDom) !!! Failed to change wrCtrl when aleady own data(out-of-ctrl), EvName=" << aEvName);
        return false;
    }

    const auto ev = this->newEvent(aEvName);
    if (ev >= wrCtrl_.size())
        wrCtrl_.resize(ev + 1);  // resize() can inc size()
    wrCtrl_[ev] = aNewState;
    HID("(WbasicDatDom) Succeed, EvName=" << aEvName << ", new wrCtrl=" << aNewState
        << ", nWr=" << wrCtrl_.size() << ", cap=" << wrCtrl_.capacity());
    return true;
}



#define EXTEND_INTERFACE_FOR_WBASIC_DATA_DOMINO  // more friendly than min WbasicDatDom interface
// ***********************************************************************************************
// - getValue() is NOT mem-safe when aEvName not found
//   . so getData() instead of getValue()
// - this func cast type so more convenient than WbasicDatDom's
template<typename aDataDominoType, typename aDataType>
S_PTR<aDataType> wbasic_getData(aDataDominoType& aDom, const Domino::EvName& aEvName)
{
    return STATIC_PTR_CAST<aDataType>(aDom.wbasic_getData(aEvName));  // mem safe: yes SafePtr, no shared_ptr
}

// ***********************************************************************************************
template<typename aDataDominoType, typename aDataType>
bool wbasic_setValueOK(aDataDominoType& aDom, const Domino::EvName& aEvName, const aDataType& aData) noexcept
{
    return aDom.wbasic_replaceDataOK(aEvName, MAKE_PTR<aDataType>(aData));
}

}  // namespace
// ***********************************************************************************************
// - why not wbasic_setValueOK() auto call wrCtrlOk()?
//   . auto is more convient & efficient
//   . but impact wbasic_setValueOK(), wbasic_getData(), setValue(), getData(), logic complex
//   . user may mis-use setValue/getData(write-ctrl-data)
// - why not throw when getData/setValue/wbasic_getData/wbasic_setValueOK fail?
//   . fit for larger scope that can't accept throw
//   . achieve basic req but may mislead user
// - why also read ctrl?
//   . SIMPLY separate interface: 1)get/set legacy 2)get/set write-protected
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2021-09-18  CSZ       1)create
// 2022-01-02  PJ & CSZ  - formal log & naming
// 2022-03-26  CSZ       - ut's PARA_DOM include self class & ALL its base class(es)
// 2022-03-27  CSZ       - if ut case can test base class, never specify derive
// 2022-08-18  CSZ       - replace CppLog by UniLog
// 2022-12-03  CSZ       - simple & natural
// 2022-12-31  CSZ       - rm data
// 2024-02-12  CSZ       2)use SafePtr (mem-safe); shared_ptr is not mem-safe
// 2025-02-13  CSZ       - support both SafePtr & shared_ptr
// ***********************************************************************************************
