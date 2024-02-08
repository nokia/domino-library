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
#include <type_traits>

using namespace std;

namespace RLib
{
// ***********************************************************************************************
template<class T> class SafePtr
{
public:
    // create
    template<class U, class... Args> friend SafePtr<U> make_safe(Args&&... aArgs);
    SafePtr() = default;

    // any <-> void
    template<class From> SafePtr(const SafePtr<From>& aSafeFrom)
        : pT_(aSafeFrom.template get<T>())
    {
        if (! is_same<T, void>::value)  // not -> void
            return;
        if (! pT_)  // invalid pT_
            return;
        if (! is_same<From, void>::value)  // not from void ->
            voidToType_ = &typeid(From);
    }

    template<typename To> shared_ptr<To> get() const
    {
        if (is_convertible<T*, To*>::value)  // Derive -> Base
            return static_pointer_cast<To>(pT_);
        if (is_same<To, void>::value)  // any -> void (for storing same type= SafePtr<void>)
            return static_pointer_cast<To>(pT_);
        if (is_same<T, void>::value && &typeid(To) == voidToType_)  // void -> back
            return static_pointer_cast<To>(pT_);
        return nullptr;
    }

private:
    // -------------------------------------------------------------------------------------------
    std::shared_ptr<T>  pT_;
    const type_info*    voidToType_ = nullptr;  // cast void back
};

// ***********************************************************************************************
template<class U, class... Args> SafePtr<U> make_safe(Args&&... aArgs)
{
    SafePtr<U> sptr;
    sptr.pT_ = make_shared<U>(forward<Args>(aArgs)...);
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
