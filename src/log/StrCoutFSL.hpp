/**
 * Copyright 2006 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// CONSTITUTION : 1 solution of smart log,
// - tmp log is stringstream (dyn buf)
// - permanent log is cout (screen)
// . shortcoming: multiple smart logs will mix output on screen
// - formatted log (FSL = Formatted Smart Log)
// . slower than unformatted smart log, but more readable
// - derive from BaseSL is to reuse common func
//
// CORE: _tmpBuf (func around it)
//
// MT safe: false
//
// mem safe: yes if stringstream is
// ***********************************************************************************************
#pragma once

#include <iostream>
#include <sstream>  // stringstream

#include "BaseSL.hpp"

namespace rlib
{
// ***********************************************************************************************
class StrCoutFSL  // FSL = Formatted Smart Log
    : public BaseSL
    , public std::stringstream   // for operator<<(); must not ostringstream since dump need read it!!!
{
public :
    ~StrCoutFSL() noexcept;

    // for gtest/etc that failure maybe after testcase destruction
    // void forceDel() { stringstream().swap(*this); }
    void forceSave() const noexcept;
};

// ***********************************************************************************************
inline
StrCoutFSL::~StrCoutFSL() noexcept
{
    if (canDelLog())
        return;

    forceSave();
}

// ***********************************************************************************************
inline
void StrCoutFSL::forceSave() const noexcept
{
    std::cout << rdbuf() << std::endl;  // internet says rdbuf() is faster than str()
}

}  // namespace
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2006-       CSZ       - create
// 2022-01-01  PJ & CSZ  - formal log & naming
// ***********************************************************************************************
