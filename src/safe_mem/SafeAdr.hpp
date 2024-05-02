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
//   . safe cast     : only among self, base & void; compile-err is safer than ret-null
//   . safe lifecycle: by shared_ptr (auto mem-mgmt, no use-after-free)
//   . safe ptr array: no need since std::array
// - DUTY-BOUND:
//   . ensure ptr address is safe: legal created, not freed, not wild, etc
//   . ensure ptr type is valid: origin*, or base*, or void*
//   . not SafeAdr but T to ensure T's inner safety (eg no exception within T's constructor)
//   . hope cooperate with tool to ensure/track SafeAdr, all T, all code's mem safe
//
// - VALUE:
//   . way#1: Rust is language-based mem ctrl (heavy)
//   . way#2: tool (dynamic eg valdrind, or static eg coverity)
//     . keep legacy code/invest
//     . but less safe than Rust
//   . way#3: eg SafeAdr
//     * if each app class is safe, then whole app is safe (safer than way#2/tool)
//       * like SafeAdr(app class) encapsulates unsafe shared_ptr(legacy)
//       * keep legacy code/invest
//       * inner-freedom + outer-safe
//     . more lightweight than Rust
// - suggest:
//   . any class ensure mem-safe (like MT safe)
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
    // - safe-only creation (eg shared_ptr<U>(U*) is not safe)
    // - can't construct by shared_ptr that maybe unsafe
    constexpr SafeAdr() = default;  // must explicit since below converter constructor
    constexpr SafeAdr(nullptr_t) noexcept : SafeAdr() {}  // implicit nullptr -> SafeAdr()
    template<typename U, typename... Args> friend SafeAdr<U> make_safe(Args&&... aArgs);  // U(Args) SHALL mem-safe

    // safe-only cast (vs shared_ptr, eg static_pointer_cast<any> is not safe)
    template<typename From> SafeAdr(const SafeAdr<From>&) noexcept;          // cp  ok or compile err
    template<typename From> SafeAdr(SafeAdr<From>&&) noexcept;               // mv  ok or compile err
    template<typename To> shared_ptr<To> cast() const noexcept;              // ret ok or null
    template<typename To, typename From>
    friend SafeAdr<To> dynamic_pointer_cast(const SafeAdr<From>&) noexcept;  // ret ok or null

    // safe usage: convenient, equivalent & min (vs shared_ptr)
    shared_ptr<T> get()        const noexcept { return pT_; }
    shared_ptr<T> operator->() const noexcept { return pT_; }  // convenient
    auto          use_count()  const noexcept { return pT_.use_count();  }

    // most for debug
    const type_info* realType() const noexcept { return realType_; }
    const type_info* diffType() const noexcept { return diffType_; }

private:
    template<typename From> void init_(const SafeAdr<From>&) noexcept;

    // -------------------------------------------------------------------------------------------
    shared_ptr<T>    pT_;
    const type_info* realType_ = &typeid(T);  // origin type
    const type_info* diffType_ = nullptr;  // diff valid type than realType_ & void
};

// ***********************************************************************************************
template<typename T>
template<typename From>
SafeAdr<T>::SafeAdr(const SafeAdr<From>& aSafeFrom) noexcept  // cp
    : pT_(aSafeFrom.get())  // to self/base/void only by shared_ptr till 024-04-25
{
    init_(aSafeFrom);
}

// ***********************************************************************************************
template<typename T>
template<typename From>
SafeAdr<T>::SafeAdr(SafeAdr<From>&& aSafeFrom) noexcept  // mv - MtQ need
    : SafeAdr(aSafeFrom)  // cp
{
    if (pT_ != nullptr)  // cp succ, clear src
        aSafeFrom = SafeAdr<From>();
}

// ***********************************************************************************************
template<typename T>
template<typename To>
shared_ptr<To> SafeAdr<T>::cast() const noexcept
{
    if constexpr(is_base_of<To, T>::value)  // safer than is_convertible()
    {
        //HID("(SafeAdr) cast derived->base/self");  // ERR() not MT safe
        return pT_;
    }
    else if constexpr(is_void<To>::value)  // else if for constexpr
    {
        //HID("(SafeAdr) cast any->void (for container to store diff types)");
        return pT_;
    }
    else if constexpr(is_base_of<T, To>::value)
    {
        //HID("(SafeAdr) cast base->derived");
        return dynamic_pointer_cast<To>(pT_);
    }
    else if constexpr(!is_void<T>::value)
    {
        HID("(SafeAdr) casting from=" << typeid(T).name() << " to=" << typeid(To).name());
        return this;  // c++17: force compile-err, safer than ret pT_ or null
        //return nullptr;  // c++14
    }
    else if (&typeid(To) == realType_)
    {
        //HID("(SafeAdr) cast any->origin");
        return static_pointer_cast<To>(pT_);
    }
    else if (&typeid(To) == diffType_)
    {
        //HID("(SafeAdr) cast to last-type-except-void");
        return static_pointer_cast<To>(pT_);
    }
    HID("(SafeAdr) can't cast from=void/" << typeid(T).name() << " to=" << typeid(To).name());
    // realType_ & diffType_ can't compile-check so has to ret null (than eg dyn-cast that always fail compile)
    return nullptr;
}

// ***********************************************************************************************
template<typename T>
template<typename From>
void SafeAdr<T>::init_(const SafeAdr<From>& aSafeFrom) noexcept
{
    // validate
    if (pT_ == nullptr)
    {
        HID("pT_ == nullptr");  // only HID() is multi-thread safe
        return;
    }

    realType_ = aSafeFrom.realType();
    // save another useful type
    if (&typeid(From) != realType_ && !is_same<From, void>::value)
        diffType_ = &typeid(From);
    else
        diffType_ = aSafeFrom.diffType();

    /*HID("cp from=" << typeid(From).name() << " to=" << typeid(T).name()
        << ", diff=" << (diffType_ == nullptr ? "null" : diffType_->name())
        << ", real=" << (realType_ == nullptr ? "null" : realType_->name()));*/
}



// ***********************************************************************************************
template<typename To, typename From>
SafeAdr<To> dynamic_pointer_cast(const SafeAdr<From>& aSafeFrom) noexcept
{
    SafeAdr<To> safeTo;
    safeTo.pT_ = aSafeFrom.template cast<To>();
    safeTo.init_(aSafeFrom);
    return safeTo;
}

// ***********************************************************************************************
template<typename U, typename... Args>
SafeAdr<U> make_safe(Args&&... aArgs)
{
    SafeAdr<U> safeU;
    safeU.pT_ = make_shared<U>(forward<Args>(aArgs)...);
    //HID("new ptr=" << (void*)(safeU.pT_.get()));  // too many print; void* print addr rather than content(dangeous)
    return safeU;
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

// ***********************************************************************************************
template<typename To, typename From>
SafeAdr<To> static_pointer_cast(const SafeAdr<From>& aFromPtr) noexcept
{
    return dynamic_pointer_cast<To>(aFromPtr);
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
// 2024-04-17  CSZ       3)strict constructor - illegal->compile-err (while *cast() can ret null)
// ***********************************************************************************************
// - Q&A
//   . must replace shared_ptr in DatDom, ObjAnywhere?
//     . SafeAdr is simple
//     . freq cast void->any is dangeous
//     . so worth
//
//   . std::any vs SafeAdr
//     . SafeAdr is safe shared_ptr that is lifecycle ptr, std::any is not ptr nor lifecycle mgmt
//
//   . T not ref/ptr/const?
//   . SafeRef? or like this?
//   . how about class' this?
