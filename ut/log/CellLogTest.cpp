/**
 * Copyright 2022 Nokia
 * Licensed under the BSD 3 Clause license
 * SPDX-License-Identifier: BSD-3-Clause
 */
// ***********************************************************************************************
#include <gtest/gtest.h>

#include "CellLog.hpp"

using namespace testing;

namespace RLib
{
// ***********************************************************************************************
struct Cell : public CellLog
{
    Cell() : CellLog("Cell") { SL_DBG("hello world, I'm a cell"); }               // req: can log
    ~Cell() { SL_DBG("bye world, I'm a cell"); }                                  // req: can log
};

struct CellMember : public CellLog
{
    CellMember() : CellLog("Cell") { SL_DBG("hello world, I'm a cell member"); }  // req: can log
    ~CellMember() { SL_DBG("bye world, I'm a cell member"); }                     // req: can log
};

void cellParticipant(CellLog& log)
{
    static size_t nCalled = 0;
    SL_DBG("hello world, I'm a cell's participant, nCalled=" << ++nCalled);       // req: can log, same API
}

// ***********************************************************************************************
TEST(CellLogTest, GOLD_cell__member_participant)
{
    {
        Cell cell;
        EXPECT_TRUE(cell.isCell());
        const auto nLog = CellLog::nCellLog();  // req: cell

        CellMember member;
        EXPECT_FALSE(member.isCell());
        EXPECT_EQ(nLog, CellLog::nCellLog());   // req: cell member

        cellParticipant(cell);                  // req: cell can call func & log into same smartlog
        cellParticipant(member);                // req: cell member can call func & log into same smartlog

        member().needLog();                     // req: shall output log to screen
    }
    EXPECT_EQ(0, CellLog::nCellLog());          // req: del log when no user
}
}  // namespace
