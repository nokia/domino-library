/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include <gtest/gtest.h>

#include "CellLog.hpp"
#include "CppLog.hpp"

using namespace testing;

namespace RLib
{
struct CellLogTest : public Test, public CellLog
{
};

TEST_F(CellLogTest, req)
{
    try { SL_DBG("hello world"); }
    catch(...) { FAIL() << "!!!exception"; }
}
}  // namespace
