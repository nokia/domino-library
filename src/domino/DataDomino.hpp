/**
 * Copyright 2020 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - ISSUE:    eg C&M translator store any type data from RU/BBU/HLAPI, like IM [MUST-HAVE!]
// - Req:      Domino to store any type of data
// - core:     dataStore_
// - scope:    provide min interface (& extend by non-member func)
// - mem-safe: true (when use SafePtr instead of shared_ptr)
// ***********************************************************************************************
#pragma once

#include <unordered_map>
#include <memory>  // make_shared

#include "UniLog.hpp"
#include "UniPtr.hpp"

using namespace std;

namespace RLib
{
// ***********************************************************************************************
template<typename aDominoType>
class DataDomino : public aDominoType
{
    // -------------------------------------------------------------------------------------------
    // Extend Tile record:
    // - data: now Tile can store any type of data besides state, optional
    // -------------------------------------------------------------------------------------------

public:
    explicit DataDomino(const LogName& aUniLogName = ULN_DEFAULT) : aDominoType(aUniLogName) {}

    // -------------------------------------------------------------------------------------------
    // - for read/write data
    // - if aEvName/data invalid, return null
    // - not template<aDataType> so can virtual for WrDatDom
    // . & let DataDomino has little idea of read-write ctrl, simpler
    virtual UniPtr getData(const Domino::EvName&) const;

    // -------------------------------------------------------------------------------------------
    // - replace old data by new=aData if old != new
    // - for aDataType w/o default constructor!!!
    virtual void replaceData(const Domino::EvName&, UniPtr = nullptr);

protected:
    void rmEv_(const Domino::Event& aValidEv) override;

private:
    // -------------------------------------------------------------------------------------------
    unordered_map<Domino::Event, UniPtr> dataStore_;  // [event]=UniPtr
};

// ***********************************************************************************************
template<typename aDominoType>
UniPtr DataDomino<aDominoType>::getData(const Domino::EvName& aEvName) const
{
    auto&& found = dataStore_.find(this->getEventBy(aEvName));
    return (found == dataStore_.end()) ? nullptr : found->second;
}

// ***********************************************************************************************
template<typename aDominoType>
void DataDomino<aDominoType>::replaceData(const Domino::EvName& aEvName, UniPtr aData)
{
    if (aData.get() == nullptr)
        dataStore_.erase(this->getEventBy(aEvName));  // avoid keep inc dataStore_
    else
        dataStore_[this->newEvent(aEvName)] = aData;
}

// ***********************************************************************************************
template<typename aDominoType>
void DataDomino<aDominoType>::rmEv_(const Domino::Event& aValidEv)
{
    dataStore_.erase(aValidEv);
    aDominoType::rmEv_(aValidEv);
}


#define EXTEND_INTERFACE_FOR_DATA_DOMINO  // more friendly than min DataDomino interface
// ***********************************************************************************************
// - getValue() is NOT mem-safe when aEvName not found
//   . so getData() instead of getValue
// - this getData() cast type so convenient
template<typename aDataDominoType, typename aDataType>
auto getData(aDataDominoType& aDom, const Domino::EvName& aEvName)
{
    return static_pointer_cast<aDataType>(aDom.getData(aEvName));
}

// ***********************************************************************************************
template<typename aDataDominoType, typename aDataType>
void setValue(aDataDominoType& aDom, const Domino::EvName& aEvName, const aDataType& aData)
{
    aDom.replaceData(aEvName, MAKE_PTR<aDataType>(aData));
}
}  // namespace
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2020-07-27  CSZ       1)create
// 2021-04-01  CSZ       - coding req
// 2021-04-30  ZQ & CSZ  2)ZhangQiang told CSZ shared_ptr<void> can del Data correctly
// 2021-09-13  CSZ       - new interface to read/write value directly
// 2021-09-14  CSZ       3)writable ctrl eg for Yang rw & ro para
// 2021-12-31  PJ & CSZ  - formal log & naming
// 2022-03-26  CSZ       - ut's PARA_DOM include self class & ALL its base class(es)
// 2022-03-27  CSZ       - if ut case can test base class, never specify derive
// 2022-08-18  CSZ       - replace CppLog by UniLog
// 2022-12-03  CSZ       - simple & natural
// 2022-12-30  CSZ       - rm data
// 2024-02-12  CSZ       4)use SafePtr (mem-safe); shared_ptr is not mem-safe
// ***********************************************************************************************
