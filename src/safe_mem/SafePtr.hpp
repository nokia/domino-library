/**
 * Copyright 2024 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - ISSUE:
//   . c++ mem bugs are 1 of the most challenge (eg use Rust to replace c++)
// - REQ: this class is to impl
//   . valid ptr: by construct
//   . valid cp/mv ptr: between valid type (same type, base-derive, any<->void)
//   . valid ptr lifecycle: by shared_ptr
//   . valid ptr array
//   . suggest any class ensure mem-safe (like MT safe)
// ***********************************************************************************************
#pragma once

#include <functional>
#include <memory>
#include <iostream>

using namespace std;

namespace RLib
{
// ***********************************************************************************************
class SafePtr
{
public:
    template<typename To = void> To* get()
    {
        return (&typeid(To) == originType_ || is_same<To, void>::value)
            ? static_pointer_cast<To>(pT_).get()
            : nullptr;
    }

    const type_info* originType() const { return originType_; }
private:
    // -------------------------------------------------------------------------------------------
    std::shared_ptr<void>  pT_;
    const type_info*       originType_ = nullptr;  // can't static since derived from T

    template<class T, class... Args> friend SafePtr make_safe(Args&&... aArgs);
};

// ***********************************************************************************************
template<class T, class... Args> SafePtr make_safe(Args&&... aArgs)
{
    SafePtr sptr;
    sptr.pT_ = static_pointer_cast<void>(make_shared<T>(forward<Args>(aArgs)...));
    sptr.originType_ = &typeid(T);
    return sptr;
}

}  // namespace
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2024-01-30  CSZ       1)create
// ***********************************************************************************************
// - T not ref/ptr/const?
// - higher perf
// - SafeRef? or like this?
// - why
//   . safe construct
//   . safe copy/=
//   . safe array
//   . struct ptr/ref member shall be SafePtr
//   . safe owner (avoid wild ptr, use-after-free)
// . how about class' this?
//   . worth?
// - all classes shall mem-safe
//   . type_info* instead of hash_code?
