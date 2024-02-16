/**
 * Copyright 2021 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - what:     simple/easy write-protect for DataDomino
// - why:      eg Yang RW para need write-protect
// - core:     wrCtrl_
// - mem-safe: true
// ***********************************************************************************************
#pragma once

#include "UniLog.hpp"

using namespace std;

namespace RLib
{
// ***********************************************************************************************
template<typename aDominoType>
class WbasicDatDom : public aDominoType
{
public:
    explicit WbasicDatDom(const UniLogName& aUniLogName) : aDominoType(aUniLogName) {}

    bool isWrCtrl(const Domino::EvName&) const;
    bool wrCtrlOk(const Domino::EvName&, const bool aNewState = true);

    UniPtr getData(const Domino::EvName&) const override;
    UniPtr wbasic_getData(const Domino::EvName&) const;

    void replaceData(const Domino::EvName&, UniPtr aUniPtr = nullptr) override;
    void wbasic_replaceData(const Domino::EvName&, UniPtr aUniPtr = nullptr);

protected:
    void innerRmEv(const Domino::Event) override;

private:
    // forbid ouside usage
    using aDominoType::getData;
    using aDominoType::replaceData;
    // -------------------------------------------------------------------------------------------
    vector<bool> wrCtrl_;

public:
    using aDominoType::oneLog;
};

// ***********************************************************************************************
template<typename aDominoType>
UniPtr WbasicDatDom<aDominoType>::getData(const Domino::EvName& aEvName) const
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
void WbasicDatDom<aDominoType>::replaceData(const Domino::EvName& aEvName, UniPtr aUniPtr)
{
    if (isWrCtrl(aEvName))
        WRN("(WbasicDatDom) Failed!!! EvName=" << aEvName << " is not write-protect so unavailable via this func!!!")
    else aDominoType::replaceData(aEvName, aUniPtr);
}

// ***********************************************************************************************
template<typename aDominoType>
UniPtr WbasicDatDom<aDominoType>::wbasic_getData(const Domino::EvName& aEvName) const
{
    if (isWrCtrl(aEvName))
        return aDominoType::getData(aEvName);

    WRN("(WbasicDatDom) Failed!!! EvName=" << aEvName << " is not write-protect so unavailable via this func!!!");
    return nullptr;
}

// ***********************************************************************************************
template<typename aDominoType>
void WbasicDatDom<aDominoType>::wbasic_replaceData(const Domino::EvName& aEvName, UniPtr aUniPtr)
{
    if (isWrCtrl(aEvName))
        aDominoType::replaceData(aEvName, aUniPtr);
    else
        WRN("(WbasicDatDom) Failed!!! EvName=" << aEvName << " is not write-protect so unavailable via this func!!!")
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
//   . so getData() instead of getValue
// - this getData() cast type so convenient
template<typename aDataDominoType, typename aDataType>
auto wbasic_getData(aDataDominoType& aDom, const Domino::EvName& aEvName)
{
    return static_pointer_cast<aDataType>(aDom.wbasic_getData(aEvName));
}

// ***********************************************************************************************
template<typename aDataDominoType, typename aDataType>
void wbasic_setValue(aDataDominoType& aDom, const Domino::EvName& aEvName, const aDataType& aData)
{
    aDom.wbasic_replaceData(aEvName, MAKE_PTR<aDataType>(aData));
}


// ***********************************************************************************************
// place at the end to avoud gcovr/gcov bug on cov
template<typename aDominoType>
void WbasicDatDom<aDominoType>::innerRmEv(const Domino::Event aEv)
{
    if (aEv < wrCtrl_.size())
        wrCtrl_[aEv] = false;

    aDominoType::innerRmEv(aEv);
}

}  // namespace
// ***********************************************************************************************
// - why not wbasic_setValue() auto call wrCtrlOk()?
//   . auto is more convient & efficient
//   . but impact wbasic_setValue(), wbasic_getData(), setValue(), getData(), logic complex
//   . user may mis-use setValue/getData(write-ctrl-data)
// - why not throw when getData/setValue/wbasic_getData/wbasic_setValue fail?
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
// ***********************************************************************************************
