/**
 * Copyright 2024 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - REQ: this class is to
//   . store data of any type: eg for ObjAnywhere
//   * store multi-copy of data of the same type: eg for DataDomino
//   . fetch data by unique key(eg string)
//   . ensure mem safe of stored data by SafePtr
//
// - MT safe: NO
// - mem safe: yes
// ***********************************************************************************************
#pragma once

#include <unordered_map>

#include "SafePtr.hpp"  // can't UniPtr.hpp since ut(=req) build-err
#include "UniLog.hpp"   // debug

namespace RLib
{
// ***********************************************************************************************
template<typename aDataKey>
class DataStore
{
public:
    // @brief: get a data
    // @ret: ok or nullptr
    template<typename aDataT> SafePtr<aDataT> get(const aDataKey& aKey) const;

    // @brief: store a new data (ie not replace but add new or rm old by aData=nullptr)
    // @ret: store ok/nok
    bool emplaceOK(const aDataKey& aKey, SafePtr<void> aData);

    // @brief: store a data (add new, replace old, or rm old by aData=nullptr)
    void replace(const aDataKey& aKey, SafePtr<void> aData);

    size_t nData() const { return key_data_S_.size(); }
    ~DataStore() { HID("(DataStore) discard nData=" << nData()); }  // debug

private:
    // -------------------------------------------------------------------------------------------
    std::unordered_map<aDataKey, SafePtr<void>> key_data_S_;
};

// ***********************************************************************************************
template<typename aDataKey>
bool DataStore<aDataKey>::emplaceOK(const aDataKey& aKey, SafePtr<void> aData)
{
    if (! aData)
    {
        HID("(DataStore) erase key=" << aKey);
        key_data_S_.erase(aKey);
        return true;
    }
    else
    {
        auto&& it_ok = key_data_S_.emplace(aKey, aData);
        if (it_ok.second)
            return true;
        else
        {
            HID("(DataStore) ERR!!! not support to replace key=" << aKey);
            return false;
        }
    }
}

// ***********************************************************************************************
template<typename aDataKey>
template<typename aDataT>
SafePtr<aDataT> DataStore<aDataKey>::get(const aDataKey& aKey) const
{
    auto&& key_data = key_data_S_.find(aKey);
    if (key_data == key_data_S_.end())
    {
        HID("(DataStore) can't find key=" << aKey);
        return nullptr;
    }
    return std::dynamic_pointer_cast<aDataT>(key_data->second);
}

// ***********************************************************************************************
template<typename aDataKey>
void DataStore<aDataKey>::replace(const aDataKey& aKey, SafePtr<void> aData)
{
    if (! aData)
    {
        HID("(DataStore) erase key=" << aKey);
        key_data_S_.erase(aKey);
    }
    else
        key_data_S_[aKey] = aData;
}

}  // namespace
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2024-06-05  CSZ       1)create
// ***********************************************************************************************
