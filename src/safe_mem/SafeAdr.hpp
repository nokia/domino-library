/**
 * Copyright 2024 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - ISSUE:
//   . c++ mem bugs are 1 of the most challenge (eg US gov suggest to replace c++ by Rust)
// - REQ: this class is to enhance safety of shared_ptr:
//   . safe create   : null / make_safe only (not allow unsafe create eg via raw ptr)
//   . safe ship     : cp/mv/assign only among self, base & void
//   . safe lifecycle: by shared_ptr (auto mem-mgmt, no use-after-free)
//   . safe ptr array: TODO
// - DUTY-BOUND:
//   . ensure ptr address is safe: legal created, not freed, not wild, etc
//   . ensure ptr type is valid: origin*, or base*, or void*
//   . not SafeAdr but T to ensure T's inner safety (eg no exception within T's constructor)
//   . hope cooperate with tool to ensure/track SafeAdr, all T, all code's mem safe
// - suggest:
//   . any class ensure mem-safe (like MT safe)
//   . struct ptr/ref member shall be SafeAdr
//
// - VALUE:
//   . way#1: Rust is language-based mem ctrl (heavy)
//   . way#2: tool (dynamic eg valdrind, or static eg coverity)
//     . keep legacy code/invest
//     . but coverage is less than Rust
//   . way#3: eg SafeAdr
//     * if each class ensures itself mem-safe, then whole program is mem-safe (more coverage than tools)
//     . more lightweight than Rust
//     . keep legacy code/invest
//     * keep c++'s full freedom while has a choice to limit partial freedom for mem-safe
//
// - MT safe: NO
//   . so eg after MtInQueue.mt_push(), shall NOT touch pushed SafeAdr
//   . only HID is MT safe that can be used here
// - mem safe: yes
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
class SafeAdr
{
public:
    // safe-only creation (vs shared_ptr, eg shared_ptr<U>(U*) is not safe)
    template<typename U, typename... Args> friend SafeAdr<U> make_safe(Args&&... aArgs);  // U::U(Args) SHALL mem-safe
    constexpr SafeAdr() = default;  // must explicit since below converter constructor
    constexpr SafeAdr(nullptr_t) noexcept : SafeAdr() {}  // implicit nullptr -> SafeAdr()

    // safe-only cast (vs shared_ptr, eg static_pointer_cast<any> is not safe)
    template<typename From> SafeAdr(const SafeAdr<From>&) noexcept;
    shared_ptr<T> cast_get() const noexcept;
    template<typename To> shared_ptr<To> cast_get() const noexcept;

    // safe usage: convenient, equivalent & min (vs shared_ptr)
    shared_ptr<T> get()        const noexcept { return pT_; }
    shared_ptr<T> operator->() const noexcept { return pT_; }  // convenient
    auto          use_count()  const noexcept { return pT_.use_count();  }

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
SafeAdr<T>::SafeAdr(const SafeAdr<From>& aSafeFrom)
    : pT_(aSafeFrom.template cast_get<T>())
{
    // validate
    if (pT_ == nullptr)
    {
        HID("pT_ == nullptr");  // only HID() is multi-thread safe
        return;
    }

    // continue cp
    realType_ = aSafeFrom.realType();
    preVoidType_ = is_same<T, void>::value
        ? preVoidType_ = &typeid(From)  // cast-to-void (impossible void->void covered by another cp constructor)
        : aSafeFrom.preVoidType();      // cast-to-nonVoid
}

// ***********************************************************************************************
template<typename T>
shared_ptr<T> SafeAdr<T>::cast_get() const  // most common usage, standalone is faster
{
    //HID("(SafeAdr) to self");  // comment-out since too many
    return pT_;
}
template<typename T>
template<typename To>
shared_ptr<To> SafeAdr<T>::cast_get() const
{
    if (is_base_of<To, T>::value)  // is_convertible is not safe to static_pointer_cast eg int* to float*
    {
        //HID("(SafeAdr) Derive to Base (include to self)");
        return static_pointer_cast<To>(pT_);
    }
    if (&typeid(To) == realType_)
    {
        //HID("(SafeAdr) any to origin");
        return static_pointer_cast<To>(pT_);
    }
    if (&typeid(To) == preVoidType_)
    {
        //HID("(SafeAdr) any to type-before-castToVoid");
        return static_pointer_cast<To>(pT_);
    }
    if (is_same<To, void>::value)
    {
        //HID("(SafeAdr) any to void (for container to store diff types)");
        return static_pointer_cast<To>(pT_);
    }
    HID("(SafeAdr) unsupported cast");  // ERR() may not be multi-thread safe
    return nullptr;
}



// ***********************************************************************************************
template<typename U, typename... Args>
SafeAdr<U> make_safe(Args&&... aArgs)
{
    SafeAdr<U> sptr;
    sptr.pT_ = make_shared<U>(forward<Args>(aArgs)...);
    //HID("new ptr=" << (void*)(sptr.pT_.get()));  // too many print; void* print addr rather than content(dangeous)
    return sptr;
}

// ***********************************************************************************************
template<typename To, typename From>
SafeAdr<To> static_pointer_cast(const SafeAdr<From>& aFromPtr) noexcept
{
    return SafeAdr<To>(aFromPtr);
}

// ***********************************************************************************************
// - SafeAdr can be key of map & unordered_map (like shared_ptr)
// - convenient usage
template<typename T, typename U>
bool operator==(SafeAdr<T> lhs, SafeAdr<U> rhs)
{
    return lhs.get() == rhs.get();
}
template<typename T, typename U>
bool operator!=(SafeAdr<T> lhs, SafeAdr<U> rhs)
{
    return !(lhs == rhs);
}
template<typename T, typename U>
bool operator<(SafeAdr<T> lhs, SafeAdr<U> rhs)
{
    return lhs.get() < rhs.get();
}
}  // namespace
template<typename T>
struct std::hash<RLib::SafeAdr<T>>
{
    auto operator()(const RLib::SafeAdr<T>& aSafeAdr) const { return hash<shared_ptr<T>>()(aSafeAdr.get()); }
};

// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2024-01-30  CSZ       1)create
// 2024-02-13  CSZ       2)usage in dom lib
// ***********************************************************************************************
// - Q&A
//   . must replace shared_ptr in DatDom, ObjAnywhere?
//     . SafeAdr is simple
//     . freq cast void-any is dangeous
//     . so worth
//
//   . SafeAdr for array:
//     . shared_ptr: c++11 start support T[], c++17 enhance, c++20 full support
//     . shared_ptr[out-bound] is NOT safe, so still need SafeAdr to support array
//     . g++12 full support T[]
//
//   . T not ref/ptr/const?
//   . SafeRef? or like this?
//   . how about class' this?
