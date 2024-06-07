/**
 * Copyright 2024 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - REQ: this class is to enhance safety of shared_ptr:
//   . store any type data: eg for ObjAnywhere
//   * store same type data: eg for DataDomino
//   . mem safe by SafePtr
// - Unique Value:
//   . store
//
// - MT safe: NO
// - mem safe: yes
// ***********************************************************************************************
#pragma once

#include <string>
#include <unordered_map>

#include "SafePtr.hpp"  // can't UniPtr.hpp since ut(=req) build-err

using namespace std;

namespace RLib
{
using DataKey = string;

// ***********************************************************************************************
class DataStore
{
public:
    template<typename aDataT> SafePtr<aDataT> get(const DataKey& aKey) const;
    void set(const DataKey& aKey, SafePtr<void> aData);
    size_t nData() const { return key_data_S_.size(); }

private:
    // -------------------------------------------------------------------------------------------
    unordered_map<DataKey, SafePtr<void>> key_data_S_;
};

// ***********************************************************************************************
template<typename aDataT>
SafePtr<aDataT> DataStore::get(const DataKey& aKey) const
{
    auto&& key_data = key_data_S_.find(aKey);
    if (key_data == key_data_S_.end())
        return nullptr;
    return dynamic_pointer_cast<aDataT>(key_data->second);
}

// ***********************************************************************************************
inline
void DataStore::set(const DataKey& aKey, SafePtr<void> aData)
{
    if (aData.get() == nullptr)
        key_data_S_.erase(aKey);
    else
        key_data_S_[aKey] = aData;
}

}  // namespace
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2024-06-05  CSZ       1)create
// ***********************************************************************************************
