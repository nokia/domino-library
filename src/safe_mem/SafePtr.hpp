/**
 * Copyright 2024 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - ISSUE:
//   . c++ mem bugs are 1 of the most challenge (eg US gov suggest to replace c++ by Rust)
//   . ms & google: ~70% safety defects caused by mem safe issue
// - REQ/value: this class is to be same as shared_ptr but 100% safe:
//   . safe create   : null or make_safe only (not allow unsafe create eg via raw ptr)
//   * safe cast     : only among self, base & void; ret-null is safe when invalid cast, comile-err is safer
//   . safe lifespan : by shared_ptr (auto mem-mgmt, no use-after-free)
//   * safe get      : ret shared_ptr than T* (no duty for usr's abuse to get & use T*)
//   . safe del      : not support self-deletor that maybe unsafe; call correct destructor via shared_ptr
//   . SafeWeak      : to&fro with SafePtr, eg SharedMsgCB<->WeakMsgCB
//   . other behavors/expectation are same as shared_ptr
// - DUTY-BOUND:
//   . under single thread
//     . so eg after MtInQueue.mt_push(), shall NOT touch pushed SafePtr
//     . only HID is MT safe that can be used here
//   . T's duty to ensure it's inner safety (eg no exception from T's constructor)
//   . loop-ref: out-duty, usr issue than SafePtr
//   . safe ptr array: no need since std::array
// ***********************************************************************************************
#pragma once

#include <memory>
#include <type_traits>
#include <typeindex>

#include "UniLog.hpp"

namespace rlib
{
template<typename T> class SafeWeak;
// ***********************************************************************************************
template<typename T = void>
class SafePtr
{
public:
    // - safe-only creation, eg can't construct by raw ptr/shared_ptr that maybe unsafe
    // - can't create by SafePtr(ConstructArgs...) that confuse cp constructor, make_safe() instead
    // - T(ConstructArgs) SHALL mem-safe
    constexpr SafePtr(std::nullptr_t = nullptr) noexcept {}
    template<typename U, typename... ConstructArgs> friend SafePtr<U> make_safe(ConstructArgs&&...) noexcept;

    template<typename From> friend class SafePtr;  // let cp/mv access private
    // safe-only cast (vs shared_ptr, eg static_pointer_cast<any> is not safe)
    template<typename From> SafePtr(const SafePtr<From>&) noexcept;   // cp, always ok (or compile err)
    template<typename From> SafePtr(SafePtr<From>&&) noexcept;        // mv, always ok (or compile err)
    // no assignment, compiler will gen it & enough
    template<typename To> std::shared_ptr<To> cast() const noexcept;  // ret ok/null
    template<typename To, typename From> friend SafePtr<To> safe_cast(const SafePtr<From>&) noexcept;  // ret ok/null

    // safe usage: convenient(compatible shared_ptr), equivalent & min
    // . ret shared_ptr is safer than T* (but not safest since to call T's func easily)
    // . no operator*() since T& is unsafe
    std::shared_ptr<T> get() const noexcept { return pT_; }
    std::shared_ptr<T> operator->() const noexcept { return pT_; }  // convenient
    explicit operator bool() const noexcept { return pT_ != nullptr; }
    auto use_count() const noexcept { return pT_.use_count(); }

    // most for debug
    auto realType() const noexcept { return realType_; }  // ret cp is safer than ref
    auto lastType() const noexcept { return lastType_; }

private:
    template<typename To> std::type_index genLastType_() const noexcept;
    constexpr SafePtr(std::shared_ptr<T>&&, const std::type_index&, const std::type_index&) noexcept;

    // -------------------------------------------------------------------------------------------
    std::shared_ptr<T> pT_;  // core
    std::type_index realType_ = typeid(T);  // origin type
    std::type_index lastType_ = typeid(T);  // maybe last valid type than realType_ & void

    // -------------------------------------------------------------------------------------------
public:
    friend class SafeWeak<T>;  // so SafeWeak.lock() can construct SafePtr
    operator SafeWeak<T>() const noexcept { return SafeWeak<T>(*this); }
};

// ***********************************************************************************************
// - safe cp (self/base/void/const), otherwise compile-err(safer than nullptr, like cp shared_ptr)
//   . no choice but assume shared_ptr cp is safe
//   . cp/implicit-converter is useful & convenient
// - void->T: compile-err, can safe_cast() if need
template<typename T>
template<typename From>
SafePtr<T>::SafePtr(const SafePtr<From>& aSafeFrom) noexcept  // cp
    : pT_(aSafeFrom.pT_)  // faster than get()
    , realType_(pT_ ? aSafeFrom.realType_ : typeid(T))  // init list is faster
    , lastType_(pT_ ? aSafeFrom.template genLastType_<T>() : typeid(T))  // init list is faster
{}

// ***********************************************************************************************
// - safe mv (self/base/void), otherwise compile-err (by shared_ptr's mv which is safe)
template<typename T>
template<typename From>
SafePtr<T>::SafePtr(SafePtr<From>&& aSafeFrom) noexcept  // mv - MtQ need
    : pT_(std::move(aSafeFrom.pT_))  // mv faster than cp
    , realType_(pT_ ? aSafeFrom.realType_ : typeid(T))
    , lastType_(pT_ ? aSafeFrom.template genLastType_<T>() : typeid(T))
{
    // reset aSafeFrom:
    // - impossible mv fail (compile err)
    // - no need reset aSafeFrom.pT_, done by mv
    aSafeFrom.realType_ = typeid(From);
    aSafeFrom.lastType_ = typeid(From);
}

// ***********************************************************************************************
// - for safe_cast; constructor is faster; private is safe
template<typename T>
constexpr SafePtr<T>::SafePtr(std::shared_ptr<T>&& aPtr, const std::type_index& aReal, const std::type_index& aLast) noexcept
    : pT_(aPtr)
    , realType_(pT_ ? aReal : typeid(T))
    , lastType_(pT_ ? aLast : typeid(T))
{}

// ***********************************************************************************************
template<typename T>
template<typename To>
std::shared_ptr<To> SafePtr<T>::cast() const noexcept
{
    if constexpr(std::is_base_of_v<To, T>)  // safer than is_convertible()
    {
        // HID("(SafePtr) cast to base/self");  // ERR() not MT safe
        return pT_;  // private/protected inherit will compile err
    }
    else if constexpr(std::is_base_of_v<T, To>)  // else if for constexpr
    {
        // HID("(SafePtr) cast to derived");
        return std::dynamic_pointer_cast<To>(pT_);
    }
    else if constexpr(std::is_void_v<To>)
    {
        // HID("(SafePtr) cast to void (for container to store diff types)");
        return pT_;
    }
    else if constexpr(!std::is_void_v<T>)
    {
        HID("(SafePtr) nonVoid-to-nonVoid, from=" << typeid(T).name() << " to=" << typeid(To).name());
        return pT_;  // compile err is safer than ret nullptr
    }

    else if (realType_ == typeid(To))
    {
        // HID("(SafePtr) cast void->origin");
        return std::static_pointer_cast<To>(pT_);
    }
    else if (lastType_  == typeid(To))
    {
        // HID("(SafePtr) cast void to last-type-except-void");
        return std::static_pointer_cast<To>(pT_);
    }
    HID("(SafePtr) can't cast from=void/" << typeid(T).name() << " to=" << typeid(To).name());
    return nullptr;  // non-constexpr branch: dynamic_pointer_cast<To>(pT_) may fail compile
}

// ***********************************************************************************************
template<typename T>
template<typename To>
std::type_index SafePtr<T>::genLastType_() const noexcept
{
    if constexpr(std::is_same_v<To, void>) {  // compile opt
        HID("T->void, recursive T.lastType_ is the most diff type before To=void");
        return lastType_;  // eg Derive->Base->void = Base
    }
    else if (realType_ == typeid(To)) {
        HID("To->T->To, T.lastType is the most diff type");
        return lastType_;  // eg Derive->Base->Derive = Base
    }
    else {
        HID("most diff type=" << typeid(To).name());
        return typeid(To);  // eg Derive->Base = Base
    }
}



// ***********************************************************************************************
template<typename U, typename... ConstructArgs>
SafePtr<U> make_safe(ConstructArgs&&... aArgs) noexcept
{
    SafePtr<U> safeU;
    try {  // bad_alloc, or except from U's constructor
        safeU.pT_ = std::make_shared<U>(std::forward<ConstructArgs>(aArgs)...);  // std::make_shared, not boost's
        // HID("new ptr=" << (void*)(safeU.pT_.get()));  // too many print; void* print addr rather than content(dangeous)
    } catch(...) {
        HID("failed since construct exception!!!");
    }
    return safeU;
}

// ***********************************************************************************************
// - SafePtr can be key of map & unordered_map (like shared_ptr)
// - convenient usage
template<typename T, typename U>
bool operator==(SafePtr<T> lhs, SafePtr<U> rhs) noexcept
{
    return lhs.get() == rhs.get();
}
template<typename T, typename U>
bool operator!=(SafePtr<T> lhs, SafePtr<U> rhs) noexcept
{
    return !(lhs == rhs);
}
template<typename T, typename U>
bool operator<(SafePtr<T> lhs, SafePtr<U> rhs) noexcept
{
    return lhs.get() < rhs.get();
}

// ***********************************************************************************************
// - safe cast all possible (self/base/dderive/void/back, more than cp constructor)
// - explicit cast so ok or nullptr (cp/mv constructor is implicit & ok/compile-err)
// - unified-ret is predictable & simple
// - std not allow overload dynamic_pointer_cast so safe_cast & DYN_PTR_CAST
template<typename To, typename From>
SafePtr<To> safe_cast(const SafePtr<From>& aSafeFrom) noexcept
{
    return SafePtr<To>(  // constructor is faster
        aSafeFrom.template cast<To>(),  // mv
        aSafeFrom.realType_,
        aSafeFrom.template genLastType_<To>()
    );
}



// ***********************************************************************************************
template<typename T = void>
class SafeWeak
{
public:
    SafeWeak(const SafePtr<T>& aSafeFrom) noexcept;
    SafePtr<T> lock() const noexcept;
    bool expired() const noexcept { return pT_.expired(); }

private:
    // -------------------------------------------------------------------------------------------
    std::weak_ptr<T> pT_;  // core
    std::type_index realType_;
    std::type_index lastType_;
};

// ***********************************************************************************************
template<typename T>
SafeWeak<T>::SafeWeak(const SafePtr<T>& aSafeFrom) noexcept
    : pT_(aSafeFrom.get())
    , realType_(aSafeFrom.realType())
    , lastType_(aSafeFrom.lastType())
{
}

// ***********************************************************************************************
template<typename T>
SafePtr<T> SafeWeak<T>::lock() const noexcept
{
    return (pT_.expired())
        ? nullptr
        : SafePtr<T>(pT_.lock(), realType_, lastType_);  // constructor is faster
}
}  // namespace



// ***********************************************************************************************
template<typename T>
struct std::hash<rlib::SafePtr<T>>
{
    auto operator()(const rlib::SafePtr<T>& aSafePtr) const { return hash<shared_ptr<T>>()(aSafePtr.get()); }
};

// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2024-01-30  CSZ       1)create
// 2024-02-13  CSZ       2)usage in dom lib
// 2024-04-17  CSZ       3)strict constructor - illegal->compile-err (while *cast() can ret null)
// 2024-05-06  CSZ       - AI-gen-code
// 2024-06-28  CSZ       - dynamic_pointer_cast ok or ret null
// 2024-10-16  CSZ       - dynamic_pointer_cast to safe_cast since std not allowed
// 2025-02-13  CSZ       4)SafeWeak
// 2025-03-24  CSZ       5)tolerate exception
// ***********************************************************************************************
// - Q&A
//   * noexcept & constexpr (for whole lib)
//     * noexcept is for safe commitment/long-term to usr
//       . so as many as possible
//       . unless unrecover (eg bad_alloc)->terminate
//       . or eg interrupt constructing (not terminate)
//     . constexpr is for performance (so not suitable as a long-term commitment)
//   * std::function can't + noexcept (since it's a template class, not a func)
//
//   . How to solve safety issue:
//     . way#1: Rust is language-based mem ctrl (heavy)
//     . way#2: tool (dynamic eg valdrind, or static eg coverity)
//       . keep legacy code/invest
//       . but less safe than Rust
//     . way#3: eg SafePtr
//       * if each app class is safe, then whole app is safe (safer than way#2/tool)
//         * like SafePtr(app class) encapsulates unsafe shared_ptr(legacy)
//         * keep legacy code/invest
//         * inner-freedom + outer-safe
//       . more lightweight than Rust
//   . suggest:
//     . any class ensure mem-safe (like MT safe)
//
//   . must replace shared_ptr in DatDom, ObjAnywhere?
//     . SafePtr is simple
//     . freq cast void->any is dangeous
//     . so worth
//   . get()/etc ret shared_ptr<T>, safe?
//     . safer than ret T* since still under lifecycle-protect
//     . but not perfect as T* still available
//     . this is the simplest way to call T's func/member - compromise convenient & safe
//   . still worth? better than shared_ptr?
//     . not perfect, but SafePtr inc safe than shared_ptr - eg create(minor), cast(major) - so worth
//     . it's possible but rare to use T* unsafely - SafePtr has this risk to exchange convenient
//     . shared_ptr reduces mem-lifecycle-mgmt(1 major-possible-bug) - so worth
//
//   . std::any vs SafePtr
//     . SafePtr is safe shared_ptr that is lifecycle ptr, std::any is not ptr nor lifecycle mgmt
//
//   * realType_ & lastType_ shall be in shared_ptr's ctrl blk - less mem
//   * like shared_ptr's deleter to cast pT_ to original type?
//     . then can dyn cast to target type, better than realType_ & lastType_
//     . need the "caster" be same para & ret - impossible
//   . encapulate cast related? better in shared_ptr's ctrl blk
//
//   . T not ref/ptr/const?
//   . SafeRef? or like this?
//   . how about class' this?
