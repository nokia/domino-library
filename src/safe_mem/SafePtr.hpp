/**
 * Copyright 2024 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// ISSUE: C++ memory bugs (US Gov suggests Rust; MS/Google: ~70% safety defects are mem-related)
//
// REQ/value: 100% safe smart pointer (like shared_ptr but prevents all unsafe operations)
// - Create:
//   . Only allow safe creation via eg make_safe<T>() or nullptr
//   . Forbid unsafe eg via raw pointer/shared_ptr construction
// - Cast (safe_cast<T>()):
//   . Only allow safe cast: eg among self, base & void, especially multi-inherit cast
//   . Unsafe cast: return nullptr, or compile-time error (better)
// - Copy/Move/Assign:
//   . Only allow safe operations eg between same type, to base/void/const
//   . Unsafe operations (eg to derive, const→non-const): return nullptr, or compile-time error (better)
// - Access, eg:
//   . safe get() returns shared_ptr<T> (not raw T*, safer lifecycle management)
//   . Correct destructor call via shared_ptr (even through void*/base*)
// - Additional Features:
//   . SafeWeak: weak reference support (lock()/expired())
//   . Container support: usable as map/unordered_map key
//   . Exception safe: construction exception → nullptr
//
// CONSTRAINTS:
//   - Single-thread-use as shared_ptr (e.g., after MtInQueue.mt_push(), don't touch SafePtr)
//   - T must be internally safe (eg no constructor exceptions expected)
//   * Circular reference: user's responsibility (like shared_ptr)
// ***********************************************************************************************
#pragma once

#include <cassert>
#include <functional>
#include <memory>
#include <type_traits>
#include <typeindex>

#include "UniLog.hpp"

namespace rlib
{
template<typename T> class SafeWeak;

// EBO: type_index only exists for SafePtr<void>, zero overhead for SafePtr<T≠void>
template<bool> struct void_type_index {};
template<>     struct void_type_index<true> { std::type_index lastType_ = typeid(void); };  // type before cast to void

// ***********************************************************************************************
template<typename T = void>
class SafePtr : private void_type_index<std::is_void_v<T>>
{
public:
    // - safe-only creation, eg can't construct by raw ptr/shared_ptr that maybe unsafe
    // - can't create by SafePtr(ConstructArgs...) that confuse cp constructor, make_safe() instead
    // - T(ConstructArgs) SHALL mem-safe
    constexpr SafePtr(std::nullptr_t = nullptr) noexcept {}
    template<typename U, typename... ConstructArgs> friend SafePtr<U> make_safe(ConstructArgs&&...) noexcept;

    // clarity: forbid unsafe create from raw ptr / shared_ptr
    template<typename U> SafePtr(U*) = delete;
    template<typename U> SafePtr(const std::shared_ptr<U>&) = delete;

    template<typename From> friend class SafePtr;  // let cp/mv access private
    // safe-only cast (vs shared_ptr, eg static_pointer_cast<any> is not safe)
    template<typename From> SafePtr(const SafePtr<From>&) noexcept;   // cp, always ok (or compile err)
    template<typename From> SafePtr(SafePtr<From>&&) noexcept;        // mv, always ok (or compile err)
    // no assignment, compiler will gen it & enough
    template<typename To> [[nodiscard]] std::shared_ptr<To> cast() const noexcept;  // ret ok/null; safe cast
    template<typename To, typename From> friend SafePtr<To> safe_cast(const SafePtr<From>&) noexcept;  // ret ok/null

    // safe usage: convenient(compatible shared_ptr), equivalent & min
    // . ret shared_ptr is safer than T* (but not safest since to call T's func easily)
    // . no operator*() since T& is unsafe
    [[nodiscard]] std::shared_ptr<T> get() const noexcept { return pT_; }
    const std::shared_ptr<T>& operator->() const noexcept { assert(pT_); return pT_; }  // convenient, zero-copy
    explicit operator bool() const noexcept { return pT_ != nullptr; }
    [[nodiscard]] auto use_count() const noexcept { return pT_.use_count(); }

    // most for debug
    [[nodiscard]] auto lastType() const noexcept;

private:
    constexpr SafePtr(std::shared_ptr<T>&&, const std::type_index&) noexcept;

    // -------------------------------------------------------------------------------------------
    std::shared_ptr<T> pT_;  // core
    // (void_type_index::lastType_) only exists for T=void, via EBO (0B for T≠void)

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
{
    if constexpr(std::is_void_v<T>)
        if (pT_) this->lastType_ = aSafeFrom.lastType();
}

// ***********************************************************************************************
// - safe mv (self/base/void), otherwise compile-err (by shared_ptr's mv which is safe)
template<typename T>
template<typename From>
SafePtr<T>::SafePtr(SafePtr<From>&& aSafeFrom) noexcept  // mv - MtQ need
    : pT_(std::move(aSafeFrom.pT_))  // mv is faster than cp
{
    if constexpr(std::is_void_v<T>)
        if (pT_) this->lastType_ = aSafeFrom.lastType();
}

// ***********************************************************************************************
// - for safe_cast; constructor is faster; private is safe
template<typename T>
constexpr SafePtr<T>::SafePtr(std::shared_ptr<T>&& aPtr, const std::type_index& aLast) noexcept
    : pT_(std::move(aPtr))  // mv
{
    if constexpr(std::is_void_v<T>) {
        this->lastType_ = pT_ ? aLast : typeid(void);
    }
    else (void)aLast;  // suppress unused warning for T≠void
}

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
        // compile-err (safer than ret nullptr)
        static_assert(std::is_same_v<T, To>, "(SafePtr) unsafe cast not allowed: unrelated nonVoid-to-nonVoid.");
        return pT_;  // unreachable, but satisfies return type
    }

    else if (this->lastType_ == typeid(To))
    {
        // HID("(SafePtr) cast void->lastType (MI safe round-trip)");
        return std::static_pointer_cast<To>(pT_);  // void stored lastType's address, static_cast restores it
    }
    HID("(SafePtr) can't cast from=void/" << typeid(T).name() << " to=" << typeid(To).name());
    return nullptr;  // non-constexpr branch: dynamic_pointer_cast<To>(pT_) may fail compile
}

// ***********************************************************************************************
template<typename T>
auto SafePtr<T>::lastType() const noexcept
{
    if constexpr(std::is_void_v<T>) return this->lastType_;
    else return std::type_index(typeid(T));
}



// ***********************************************************************************************
template<typename U, typename... ConstructArgs>
[[nodiscard]] SafePtr<U> make_safe(ConstructArgs&&... aArgs) noexcept
{
    SafePtr<U> safeU;
    try {  // bad_alloc, or except from U's constructor
        safeU.pT_ = std::make_shared<U>(std::forward<ConstructArgs>(aArgs)...);  // std::make_shared, not boost's
        // HID("new ptr=" << (void*)(safeU.pT_.get()));  // too many print; void* print addr rather than content(dangeous)
    } catch(...) {
        HID("(make_safe) except=" << mt_exceptInfo());  // only HID is MT safe
    }
    return safeU;
}

// ***********************************************************************************************
// - SafePtr can be key of map & unordered_map (like shared_ptr)
// - convenient usage
template<typename T, typename U>
bool operator==(const SafePtr<T>& lhs, const SafePtr<U>& rhs) noexcept
{
    return lhs.get() == rhs.get();
}
template<typename T, typename U>
bool operator!=(const SafePtr<T>& lhs, const SafePtr<U>& rhs) noexcept
{
    return lhs.get() != rhs.get();
}
template<typename T, typename U>
bool operator<(const SafePtr<T>& lhs, const SafePtr<U>& rhs) noexcept
{
    return std::less<>()(lhs.get(), rhs.get());  // less() is safer to cmp any ptr than <()
}

// ***********************************************************************************************
// - safe cast all possible (self/base/dderive/void/back, more than cp constructor)
// - explicit cast so ok or nullptr (cp/mv constructor is implicit & ok/compile-err)
// - unified-ret is predictable & simple
// - std not allow overload dynamic_pointer_cast so safe_cast & DYN_PTR_CAST
template<typename To, typename From>
[[nodiscard]] SafePtr<To> safe_cast(const SafePtr<From>& aSafeFrom) noexcept
{
    if constexpr(std::is_void_v<To>)
        return SafePtr<To>(aSafeFrom.template cast<To>(), aSafeFrom.lastType());
    else
        return SafePtr<To>(aSafeFrom.template cast<To>(), std::type_index(typeid(To)));
}



// ***********************************************************************************************
template<typename T = void>
class SafeWeak : private void_type_index<std::is_void_v<T>>
{
public:
    SafeWeak(const SafePtr<T>& aSafeFrom) noexcept;
    [[nodiscard]] SafePtr<T> lock() const noexcept;
    [[nodiscard]] bool expired() const noexcept { return pT_.expired(); }

private:
    // -------------------------------------------------------------------------------------------
    std::weak_ptr<T> pT_;  // core
    // (void_type_index::lastType_) only exists for T=void, via EBO (0B for T≠void)
};

// ***********************************************************************************************
template<typename T>
SafeWeak<T>::SafeWeak(const SafePtr<T>& aSafeFrom) noexcept
    : pT_(aSafeFrom.pT_)
{
    if constexpr(std::is_void_v<T>)
        this->lastType_ = aSafeFrom.lastType();
}

// ***********************************************************************************************
template<typename T>
SafePtr<T> SafeWeak<T>::lock() const noexcept
{
    auto sp = pT_.lock();
    if (!sp) return SafePtr<T>();
    if constexpr(std::is_void_v<T>)
        return SafePtr<T>(std::move(sp), this->lastType_);
    else
        return SafePtr<T>(std::move(sp), std::type_index(typeid(T)));
}
}  // namespace



// ***********************************************************************************************
template<typename T>
struct std::hash<rlib::SafePtr<T>>
{
    size_t operator()(const rlib::SafePtr<T>& aSafePtr) const noexcept
    {
        return std::hash<std::shared_ptr<T>>{}(aSafePtr.get());
    }
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
// 2026-04-11  CSZ       6)fix D->B->void->B; only lastType_ for SafePtr&WeakPtr<void> via EBO
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
