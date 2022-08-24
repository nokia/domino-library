/**
 * Copyright 2021-2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - what: simple/easy write-protect for DataDomino
// - why:  eg Yang RW para need write-protect
// - core: wrCtrl_
// ***********************************************************************************************
#pragma once

namespace RLib
{
// ***********************************************************************************************
template<typename aDominoType>
class WbasicDatDom : public aDominoType
{
public:
    explicit WbasicDatDom(const CellName& aCellName) : aDominoType(aCellName) {}

    bool isWrCtrl(const Domino::EvName&) const;
    bool wrCtrlOk(const Domino::EvName&, const bool aNewState = true);

    std::shared_ptr<void> getShared(const Domino::EvName& aEvName) override;
    std::shared_ptr<void> wbasic_getShared(const Domino::EvName& aEvName);

    void replaceShared(const Domino::EvName& aEvName, std::shared_ptr<void> aSharedData) override;
    void wbasic_replaceShared(const Domino::EvName& aEvName, std::shared_ptr<void> aSharedData);

private:
    // forbid ouside usage
    using aDominoType::getShared;
    using aDominoType::replaceShared;
    // -------------------------------------------------------------------------------------------
    std::vector<bool> wrCtrl_;

public:
    using aDominoType::oneLog;
};

// ***********************************************************************************************
template<typename aDominoType>
std::shared_ptr<void> WbasicDatDom<aDominoType>::getShared(const Domino::EvName& aEvName)
{
    if (not isWrCtrl(aEvName)) return aDominoType::getShared(aEvName);

    WRN("(WbasicDatDom) Failed!!! EvName=" << aEvName << " is not write-protect so unavailable via this func!!!");
    return std::shared_ptr<void>();
}

// ***********************************************************************************************
template<typename aDominoType>
bool WbasicDatDom<aDominoType>::isWrCtrl(const Domino::EvName& aEvName) const
{
    const auto ev = this->getEventBy(aEvName);
    return ev < wrCtrl_.size() ? wrCtrl_.at(ev) : false;
}

// ***********************************************************************************************
template<typename aDominoType>
void WbasicDatDom<aDominoType>::replaceShared(const Domino::EvName& aEvName, std::shared_ptr<void> aSharedData)
{
    if (isWrCtrl(aEvName))
        WRN("(WbasicDatDom) Failed!!! EvName=" << aEvName << " is not write-protect so unavailable via this func!!!")
    else aDominoType::replaceShared(aEvName, aSharedData);
}

// ***********************************************************************************************
template<typename aDominoType>
std::shared_ptr<void> WbasicDatDom<aDominoType>::wbasic_getShared(const Domino::EvName& aEvName)
{
    if (isWrCtrl(aEvName)) return aDominoType::getShared(aEvName);

    WRN("(WbasicDatDom) Failed!!! EvName=" << aEvName << " is not write-protect so unavailable via this func!!!");
    return std::shared_ptr<void>();
}

// ***********************************************************************************************
template<typename aDominoType>
void WbasicDatDom<aDominoType>::wbasic_replaceShared(const Domino::EvName& aEvName, std::shared_ptr<void> aSharedData)
{
    if (isWrCtrl(aEvName)) aDominoType::replaceShared(aEvName, aSharedData);
    else WRN("(WbasicDatDom) Failed!!! EvName=" << aEvName << " is not write-protect so unavailable via this func!!!")
}

// ***********************************************************************************************
template<typename aDominoType>
bool WbasicDatDom<aDominoType>::wrCtrlOk(const Domino::EvName& aEvName, const bool aNewState)
{
    const auto nShared = this->nShared(aEvName);
    if (nShared != 0)
    {
        WRN("(WbasicDatDom) Failed!!! EvName=" << aEvName
            << ", its nShared=" << nShared << " (must=0 otherwise may out-ctrl!!!)");
        return false;
    }

    const auto ev = this->newEvent(aEvName);
    if (ev >= wrCtrl_.size()) wrCtrl_.resize(ev + 1);
    wrCtrl_[ev] = aNewState;
    HID("(WbasicDatDom) Succeed, EvName=" << aEvName << ", newState=" << aNewState);
    return true;
}



#define EXTEND_INTERFACE_FOR_WBASIC_DATA_DOMINO  // more friendly than min WbasicDatDom interface
// ***********************************************************************************************
template<typename aDataDominoType, typename aDataType>
aDataType wbasic_getValue(aDataDominoType& aDom, const Domino::EvName& aEvName)
{
    auto&& data = std::static_pointer_cast<aDataType>(aDom.wbasic_getShared(aEvName));
    if (data.use_count() > 0) return *data;

    auto&& oneLog = aDom;
    WRN("(WbasicDatDom) Failed!!! EvName=" << aEvName << " not found, return undefined obj!!!");
    return aDataType();
}

// ***********************************************************************************************
template<typename aDataDominoType, typename aDataType>
void wbasic_setValue(aDataDominoType& aDom, const Domino::EvName& aEvName, const aDataType& aData)
{
    auto&& data = std::make_shared<aDataType>(aData);
    aDom.wbasic_replaceShared(aEvName, data);
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
// 2022-08-18  CSZ       - replace CppLog by CellLog
// ***********************************************************************************************
