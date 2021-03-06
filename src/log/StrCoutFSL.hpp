/**
 * Copyright 2006-2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
// CONSTITUTION : 1 solution of smart log,
// - tmp log is std::stringstream (dyn buf)
// - permanent log is std::cout (screen)
// . shortcoming: multiple smart logs will mix output on screen
// - formatted log (FSL = Formatted Smart Log)
// . slower than unformatted smart log, but more readable
// - derive from BaseSL is to reuse common func
//
// CORE: _tmpBuf (func around it)
// ***********************************************************************************************
#ifndef STREAM2COUT_FORMATTED_SMART_LOG_HPP_
#define STREAM2COUT_FORMATTED_SMART_LOG_HPP_

#include <iostream>
#include <sstream>  //std::stringstream

#include "BaseSL.hpp"

namespace RLib
{
// ***********************************************************************************************
class StrCoutFSL               // FSL = Formatted Smart Log
   : public BaseSL
   , public std::stringstream  // for operator<<()
{
public :
    ~StrCoutFSL();

    // for gtest/etc that failure maybe after testcase destruction
    void forceDel() { std::stringstream().swap(*this); }
    void forceSave();
};

// ***********************************************************************************************
inline
StrCoutFSL::~StrCoutFSL()
{
    if( everythingOK() ) return;

    forceSave();
}

// ***********************************************************************************************
inline
void StrCoutFSL::forceSave()
{
    if(rdbuf()->in_avail())  // otherwise cout << rdbuf() will abnormal cout
    {
        std::cout << rdbuf() << std::endl;  // internet says rdbuf() is faster than str()
    }
}
}  // namespace
#endif//STREAM2COUT_FORMATTED_SMART_LOG_HPP_
// ***********************************************************************************************
// YYYY-MM-DD  Who       v)Modification Description
// ..........  .........   .......................................................................
// 2006-       CSZ       - create
// 2022-01-01  PJ & CSZ  - formal log & naming
// ***********************************************************************************************
