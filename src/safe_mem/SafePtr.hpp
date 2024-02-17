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
//   . safe use      : cast to real / before void
//   . safe lifecycle: by shared_ptr
//   . safe ptr array:
// - mem-safe: true
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
template<typename T = void>
class SafePtr
{
public:
    // safe-only creation (vs shared_ptr, eg shared_ptr(U*) is not safe)
    template<typename U, typename... Args> friend SafePtr<U> make_safe(Args&&... aArgs);
    constexpr SafePtr() noexcept = default;      // must explicit since below converter constructor
    constexpr SafePtr(nullptr_t) : SafePtr() {}  // implicit nullptr -> SafePtr()

    // safe-only cast (vs shared_ptr, eg static_pointer_cast<any> is not safe)
    template<typename From> SafePtr(const SafePtr<From>&) noexcept;
    template<typename To> shared_ptr<To> cast_get() const noexcept;
    shared_ptr<T> cast_get() const noexcept;

    // use: safe, compatible & min (vs shared_ptr)
    // - get() etc doesn't break SafePtr's safety though caller may abuse T*
    // - T shall ensure itself safety (forbid abuse)
    T* get() const noexcept { return pT_.get(); }  // same interface as shared_ptr
    auto operator->() const noexcept { return pT_; }  // same interface as shared_ptr
    auto use_count() const noexcept { return pT_.use_count(); }  // same interface as shared_ptr

    // most for debug
    const type_info* preVoidType() const noexcept { return preVoidType_; }
    const type_info* realType() const noexcept { return realType_; }

private:
    // -------------------------------------------------------------------------------------------
    shared_ptr<T>    pT_;
    const type_info* preVoidType_ = nullptr;  // that before cast to void, can safely cast back
    const type_info* realType_ = &typeid(T);  // that pT_ point to, can safely cast to
};

// ***********************************************************************************************
template<typename T>
template<typename From>
SafePtr<T>::SafePtr(const SafePtr<From>& aSafeFrom)
    : pT_(aSafeFrom.template cast_get<T>())
{
    HID(typeid(From).name() << " to " << typeid(T).name());
    if (! pT_)
    {
        HID("pT_ == nullptr");  // HID: ut debug only
        return;
    }
    realType_ = aSafeFrom.realType();

    preVoidType_ = aSafeFrom.preVoidType();
    if (! is_same<T, void>::value)
    {
        HID("valid cast to non-void");
        return;
    }
    HID("cast non-void to void (void-to-void is covered by copy constructor)");
    preVoidType_ = &typeid(From);
}

// ***********************************************************************************************
template<typename T>
shared_ptr<T> SafePtr<T>::cast_get() const  // most common usage, standalone is faster
{
    HID("(SafePtr) to self");
    return pT_;
}
template<typename T>
template<typename To>
shared_ptr<To> SafePtr<T>::cast_get() const
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
    if (&typeid(To) == preVoidType_)
    {
        HID("(SafePtr) any to type-before-cast-to-void");
        return static_pointer_cast<To>(pT_);
    }
    if (is_same<To, void>::value)
    {
        HID("(SafePtr) any to void (for container to store diff types)");
        return static_pointer_cast<To>(pT_);
    }
    HID("(SafePtr) unsupported cast");
    return nullptr;
}



// ***********************************************************************************************
template<typename U, typename... Args>
SafePtr<U> make_safe(Args&&... aArgs)
{
    SafePtr<U> sptr;
    sptr.pT_ = make_shared<U>(forward<Args>(aArgs)...);
    return sptr;  // !!! valgrind failed when HID() here
}

// ***********************************************************************************************
template<typename To, typename From>
auto static_pointer_cast(const SafePtr<From>& aFromPtr) noexcept
{
    return SafePtr<To>(aFromPtr);
}

}  // namespace
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2024-01-30  CSZ       1)create
// 2024-02-13  CSZ       2)usage in dom lib
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
