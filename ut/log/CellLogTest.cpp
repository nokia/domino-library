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
    // req: can log
    Cell(const CellName aCellName = CELL_NAME_DEFAULT) : CellLog(aCellName) { DBG("hello world, I'm a cell"); }
    ~Cell() { DBG("bye world, I'm a cell"); }                             // req: can log
};

struct CellMember : public CellLog
{
    // req: can log
    CellMember(CellName aCellName = CELL_NAME_DEFAULT) : CellLog(aCellName) { DBG("hello world, I'm a cell member"); }
    ~CellMember() { DBG("bye world, I'm a cell member"); }                // req: can log
};

void cellParticipant(CellLog& ssLog = CellLog::defaultCellLog())
{
    static size_t nCalled = 0;
    DBG("hello world, I'm a cell's participant, nCalled=" << ++nCalled);  // req: can log, same API
}

// ***********************************************************************************************
TEST(CellLogTest, GOLD_cell_member_participant)
{
    {
        Cell cell("CELL");
        EXPECT_TRUE(cell.isCell());
        const auto nLog = CellLog::nCellLog();  // req: cell

        CellMember member("CELL");
        EXPECT_FALSE(member.isCell());
        EXPECT_EQ(nLog, CellLog::nCellLog());   // req: cell member

        cellParticipant(cell);                  // req: cell can call func & log into same smartlog
        cellParticipant(member);                // req: cell member can call func & log into same smartlog

        member.needLog();                       // req: shall output log to screen
    }
    EXPECT_EQ(0, CellLog::nCellLog());          // req: del log when no user
}

// ***********************************************************************************************
struct NonCell
{
    NonCell() { DBG("hello world, I'm NOT a cell"); }  // req: can log
    ~NonCell() { DBG("bye world, I'm NOT a cell"); }   // req: can log
};

void nonCellFunc()
{
    static size_t nCalled = 0;
    DBG("hello world, I'm NOT a cell's participant, nCalled=" << ++nCalled);  // req: can log, same API
}

// ***********************************************************************************************
TEST(CellLogTest, no_explicit_CellLog_like_legacy)
{
    {
        Cell cell;                      // req: no explicit CellLog
        CellMember member;              // req: no explicit CellLog
        cellParticipant();              // req: no explicit CellLog

        member.needLog();               // req: shall output log to screen

        NonCell nonCell;                // req: class not based on CellLog
        nonCellFunc();                  // req: func w/o CellLog para
    }
    EXPECT_EQ(0, CellLog::nCellLog());  // req: del log when no user
}

}  // namespace
