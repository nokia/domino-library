/**
 * Copyright 2024 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - ISSUE:
//   . c++ mem bugs are 1 of the most challenge (eg use Rust to replace c++)
// - REQ: this class is to impl
//   . safe create   : null & make_safe only
//   . safe ship     : cp/mv/= base<-derive(s); short->int?
//   . safe store    : cast any<->void
//   . safe use      : get real / before void
//   . safe lifecycle: by shared_ptr
//   . safe ptr array:
// - suggest:
//   . any class ensure mem-safe (like MT safe)
//   . struct ptr/ref member shall be SafePtr
// ***********************************************************************************************
#pragma once

#include <functional>
#include <memory>
#include <type_traits>

#include "UniLog.hpp"

using namespace std;

namespace RLib
{
// ***********************************************************************************************
template<typename T = void> class SafePtr
{
public:
    // create
    template<typename U, typename... Args> friend SafePtr<U> make_safe(Args&&... aArgs);
    SafePtr() = default;

    // any <-> void
    template<typename From> SafePtr(const SafePtr<From>&);

    template<typename To> shared_ptr<To> get() const;
    shared_ptr<void> get() const;

    const type_info* preVoidType() const { return preVoidType_; }
    const type_info* realType() const { return realType_; }

private:
    // -------------------------------------------------------------------------------------------
    std::shared_ptr<T>  pT_;
    const type_info*    preVoidType_ = nullptr;  // that before cast to void, can safely cast back
    const type_info*    realType_ = &typeid(T);  // that pT_ point to, can safely cast to
};

// ***********************************************************************************************
template<typename T>
template<typename From>
SafePtr<T>::SafePtr(const SafePtr<From>& aSafeFrom)
    : pT_(aSafeFrom.template get<T>())
{
    HID(typeid(From).name() << " to " << typeid(T).name());
    if (! pT_)
    {
        HID("pT_ == nullptr");  // HID: ut debug only
        return;
    }
    realType_ = aSafeFrom.realType();
    if (! is_same<T, void>::value)
    {
        HID("not to void");
        return;
    }
    if (! is_same<From, void>::value)
    {
        HID("not from void");
        preVoidType_ = &typeid(From);
    }
}

// ***********************************************************************************************
template<typename T>
template<typename To>
shared_ptr<To> SafePtr<T>::get() const
{
    if (is_convertible<T*, To*>::value)
    {
        HID("(SafePtr) Derive to Base (include to self)");
        return static_pointer_cast<To>(pT_);
    }
    if (&typeid(To) == realType_)
    {
        HID("(SafePtr) any to origin");
        return static_pointer_cast<To>(pT_);
    }
    return nullptr;
}
template<typename T>
shared_ptr<void> SafePtr<T>::get() const
{
    HID("(SafePtr) any to void (for container to store diff types)");
    return pT_;
}
template<>
template<typename To>
shared_ptr<To> SafePtr<void>::get() const
{
    HID("(SafePtr) back from void");
    return static_pointer_cast<To>(pT_);
}

// ***********************************************************************************************
template<typename U, typename... Args> SafePtr<U> make_safe(Args&&... aArgs)
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
// - Q&A
//   . must replace shared_ptr in DatDom, ObjAnywhere?
//     . SafePtr is simple
//     . freq cast void-any is dangeous
//     . so worth
//
//   . SafePtr for array:
//     . shared_ptr: c++11 start support T[], c++17 enhance, c++20 full support
//     . shared_ptr[out-bound] is NOT safe, so still need SafePtr to support array
//     . g++12 full support T[]
//
//   . T not ref/ptr/const?
//   . SafeRef? or like this?
//   . how about class' this?
