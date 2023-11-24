/**
 * Copyright 2021 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - what: simple/easy write-protect for DataDomino
// - why:  eg Yang RW para need write-protect
// - core: wrCtrl_
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

    shared_ptr<void> getShared(const Domino::EvName&) const override;
    shared_ptr<void> wbasic_getShared(const Domino::EvName&) const;

    void replaceShared(const Domino::EvName&, shared_ptr<void> aSharedData = nullptr) override;
    void wbasic_replaceShared(const Domino::EvName&, shared_ptr<void> aSharedData = nullptr);

protected:
    void innerRmEvOK(const Domino::Event) override;

private:
    // forbid ouside usage
    using aDominoType::getShared;
    using aDominoType::replaceShared;
    // -------------------------------------------------------------------------------------------
    vector<bool> wrCtrl_;

public:
    using aDominoType::oneLog;
};

// ***********************************************************************************************
template<typename aDominoType>
shared_ptr<void> WbasicDatDom<aDominoType>::getShared(const Domino::EvName& aEvName) const
{
    if (not isWrCtrl(aEvName))
        return aDominoType::getShared(aEvName);

    WRN("(WbasicDatDom) Failed!!! EvName=" << aEvName << " is not write-protect so unavailable via this func!!!");
    return shared_ptr<void>();
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
void WbasicDatDom<aDominoType>::replaceShared(const Domino::EvName& aEvName, shared_ptr<void> aSharedData)
{
    if (isWrCtrl(aEvName))
        WRN("(WbasicDatDom) Failed!!! EvName=" << aEvName << " is not write-protect so unavailable via this func!!!")
    else aDominoType::replaceShared(aEvName, aSharedData);
}

// ***********************************************************************************************
template<typename aDominoType>
shared_ptr<void> WbasicDatDom<aDominoType>::wbasic_getShared(const Domino::EvName& aEvName) const
{
    if (isWrCtrl(aEvName))
        return aDominoType::getShared(aEvName);

    WRN("(WbasicDatDom) Failed!!! EvName=" << aEvName << " is not write-protect so unavailable via this func!!!");
    return shared_ptr<void>();
}

// ***********************************************************************************************
template<typename aDominoType>
void WbasicDatDom<aDominoType>::wbasic_replaceShared(const Domino::EvName& aEvName, shared_ptr<void> aSharedData)
{
    if (isWrCtrl(aEvName))
        aDominoType::replaceShared(aEvName, aSharedData);
    else
        WRN("(WbasicDatDom) Failed!!! EvName=" << aEvName << " is not write-protect so unavailable via this func!!!")
}

// ***********************************************************************************************
template<typename aDominoType>
bool WbasicDatDom<aDominoType>::wrCtrlOk(const Domino::EvName& aEvName, const bool aNewState)
{
    if (aDominoType::getShared(aEvName) != nullptr)
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
template<typename aDataDominoType, typename aDataType>
aDataType wbasic_getValue(aDataDominoType& aDom, const Domino::EvName& aEvName)
{
    auto&& data = static_pointer_cast<aDataType>(aDom.wbasic_getShared(aEvName));
    if (data != nullptr)
        return *data;

    auto&& oneLog = aDom;
    WRN("(WbasicDatDom) Failed!!! EvName=" << aEvName << " not found, return undefined obj!!!");
    return aDataType();
}

// ***********************************************************************************************
template<typename aDataDominoType, typename aDataType>
void wbasic_setValue(aDataDominoType& aDom, const Domino::EvName& aEvName, const aDataType& aData)
{
    aDom.wbasic_replaceShared(aEvName, make_shared<aDataType>(aData));
}


// ***********************************************************************************************
// place at the end to avoud gcovr/gcov bug on cov
template<typename aDominoType>
void WbasicDatDom<aDominoType>::innerRmEvOK(const Domino::Event aEv)
{
    if (aEv < wrCtrl_.size())
        wrCtrl_[aEv] = false;

    aDominoType::innerRmEvOK(aEv);
}

}  // namespace
// ***********************************************************************************************
// - why not wbasic_setValue() auto call wrCtrlOk()?
//   . auto is more convient & efficient
//   . but impact wbasic_setValue(), wbasic_getValue(), setValue(), getValue(), logic complex
//   . user may mis-use setValue/getValue(write-ctrl-data)
// - why not throw when getValue/setValue/wbasic_getValue/wbasic_setValue fail?
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
// ***********************************************************************************************
