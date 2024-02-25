/**
 * Copyright 2024 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// - why:
//   . replace shared_ptr<string> that is not full mem-safe (eg shared_ptr<string>(ptr))
//   * can use SafeAdr<string>, but will cross-include SafeAdr.hpp & UniLog.hpp
//   . simplify SafeAdr<string> - eg no cast
//   . convenient:
//     . print content & nullptr
//     . sensible string compare
//
// - mem safe: yes
// - MT safe : NO
// ***********************************************************************************************
#pragma once

#include <memory>
#include <string>

using namespace std;

namespace RLib
{
// ***********************************************************************************************
class SafeStr
{
public:
    SafeStr(const char*   aContent) : str_(make_shared<string>(aContent)) {}
    SafeStr(const string& aContent) : str_(make_shared<string>(aContent)) {}
    SafeStr() = default;

    shared_ptr<string> operator*() const { return str_; }  // for compare etc
    string operator()() const { return str_ ? *str_ : "null shared_ptr<>"; }  // for print
private:
    shared_ptr<string> str_;
};

// ***********************************************************************************************
inline
bool operator==(const SafeStr& lhs, const SafeStr& rhs)  // for convenient compare
{
    if (*lhs == *rhs)  // self
        return true;
    if (*lhs == nullptr)
        return false;
    if (*rhs == nullptr)
        return false;
    return **lhs == **rhs;  // same content
}

// ***********************************************************************************************
inline
bool operator!=(const SafeStr& lhs, const SafeStr& rhs)
{
    return ! (lhs == rhs);
}

}  // namespace
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2024-02-24  CSZ       1)create
// ***********************************************************************************************
